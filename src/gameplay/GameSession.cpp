#include "GameSession.h"
#include "world/TempleMesh.h"
#include <algorithm>
#include <cmath>

GameSession::GameSession(const std::string& dir)
    : levels({dir + "/level01.txt", dir + "/level02.txt", dir + "/level03.txt"}) {}

bool GameSession::restart() {
    victory = false; deaths = 0;
    return levels.loadFirst() && initializeLevel();
}
bool GameSession::reset() { return levels.reload() && initializeLevel(); }

bool GameSession::initializeLevel() {
    const auto& l = levels.current();
    const auto spawn = l.cellCenterXZ(l.playerSpawn);
    player = player3d::PlayerMovement3D({spawn.x, spawn.y}, 4.0f, .28f);
    mirrors.clear(); rubies.clear(); doors.assign(l.doors.size(), Door{});
    traps.assign(l.traps.size(), Trap{}); walls.clear(); obstacles.clear(); beams.clear();
    doorLift = 0;
    for (const auto& m : l.mirrors)
        mirrors.emplace_back(l.cellCenterXZ(m.cell), 1.3f, m.angleDeg, -80.f, 80.f);
    for (const auto& r : l.rubies) { (void)r; rubies.emplace_back(true); }
    for (const auto& b : world::buildStaticColliders(l)) {
        obstacles.push_back({b.min.x, b.max.x, b.min.z, b.max.z});
        walls.push_back({{b.min.x,b.min.z},{b.max.x,b.min.z}});
        walls.push_back({{b.max.x,b.min.z},{b.max.x,b.max.z}});
        walls.push_back({{b.max.x,b.max.z},{b.min.x,b.max.z}});
        walls.push_back({{b.min.x,b.max.z},{b.min.x,b.min.z}});
    }
    // Pedestals are solid to the player, but only mirror surfaces reflect beams.
    auto pedestal = [&](glm::ivec2 cell) {
        const auto p = l.cellCenterXZ(cell);
        obstacles.push_back({p.x-.4f,p.x+.4f,p.y-.4f,p.y+.4f});
    };
    for (const auto& m : l.mirrors) pedestal(m.cell);
    for (const auto& r : l.rubies) pedestal(r.cell);
    for (const auto& e : l.lights) pedestal(e.cell);
    trace();
    return true;
}

void GameSession::trace() {
    const auto& l = levels.current();
    beams.clear();
    std::vector<RubyTarget> targets;
    for (const auto& r : l.rubies) targets.push_back({l.cellCenter(r.cell,1.3f), .38f, true});
    auto blockers = walls;
    for (size_t i=0; i<doors.size(); ++i) if (!doors[i].isOpen()) {
        const auto p = l.cellCenterXZ(l.doors[i].cell);
        blockers.push_back({{p.x-l.cellSize*.5f,p.y},{p.x+l.cellSize*.5f,p.y}});
    }
    for (const auto& light : l.lights) {
        const auto d = world::dirToVec(light.dir);
        const auto hit = BeamTracer::traceWithRubies(l.cellCenterXZ(light.cell),
            {d.x,d.z}, mirrors, targets, 12, 200, 1.3f, blockers);
        beams.insert(beams.end(),hit.path.begin(),hit.path.end());
        if (hit.hitRubyIndex >= 0) rubies.at(hit.hitRubyIndex).hitByLight();
    }
    for (size_t i=0;i<doors.size();++i) {
        bool ready = true;
        for (int r : l.doors[i].needsRubies) ready &= rubies.at(r).isActivated();
        if (ready) doors[i].open();
    }
    for (size_t i=0;i<traps.size();++i) {
        const int r = l.traps[i].disabledByRuby;
        if (r >= 0 && rubies.at(r).isActivated()) traps[i].deactivate();
    }
}
int GameSession::nearbyMirror() const {
    int best = -1; float distance = 2.2f;
    const auto p = player.position();
    for (size_t i=0;i<mirrors.size();++i) {
        if (!levels.current().mirrors[i].rotatable) continue;
        const float d = glm::length(mirrors[i].getCenter()-glm::vec2(p.x,p.z));
        if (d < distance) { distance=d; best=static_cast<int>(i); }
    }
    return best;
}
bool GameSession::rotate(float degrees) {
    const int i = nearbyMirror();
    if (i < 0) return false;
    mirrors[i].setAngle(mirrors[i].getAngle()+degrees);
    trace(); return true;
}
bool GameSession::solved() const {
    return !doors.empty() && std::all_of(doors.begin(),doors.end(),[](const Door& d){return d.isOpen();});
}
bool GameSession::advance() {
    if (!solved()) return false;
    if (levels.isLastLevel()) { victory = true; return true; }
    return levels.next() && initializeLevel();
}
void GameSession::update(float x,float z,float dt) {
    if (victory) return;
    dt = std::clamp(dt,0.f,.05f);
    trace();
    const auto& l = levels.current();
    auto solid = obstacles;
    for (size_t i=0;i<doors.size();++i) if (!doors[i].isOpen() || doorLift<.95f) {
        const auto p = l.cellCenterXZ(l.doors[i].cell);
        const float q = l.cellSize*.5f;
        solid.push_back({p.x-q,p.x+q,p.y-q,p.y+q});
    }
    player.update(x,z,dt,{0,l.width*l.cellSize,0,l.height*l.cellSize},solid);
    doorLift = std::min(1.f,doorLift+(solved()?dt:0.f));
    const auto p=player.position();
    const auto cell=l.worldToCell({p.x,0,p.z});
    const int t=l.trapAt(cell);
    if (l.isPit(cell) || (t>=0 && traps[t].isActive())) {
        ++deaths; player.reset(); return;
    }
    const int d=l.doorAt(cell);
    if (d>=0 && doors[d].isOpen() && doorLift>=.95f) advance();
}
