#include "GameSession.h"
#include "world/TempleMesh.h"
#include "world/WorldMetrics.h"
#include <algorithm>
#include <cmath>

GameSession::GameSession(const std::string& dir)
    : levels({dir + "/level01.txt", dir + "/level02.txt", dir + "/level03.txt", dir + "/level04.txt", dir + "/level05.txt"}) {}

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
    for(size_t i=0;i<traps.size();++i) {
        const auto& t=l.traps[i];
        if(t.timed)traps[i].configureTimed(t.safeSeconds,t.warningSeconds,t.extendedSeconds,t.phaseOffset);
    }

    for (const auto& m : l.mirrors)
        mirrors.emplace_back(l.cellCenterXZ(m.cell), metrics::mirrorLength, m.angleDeg, -80.f, 80.f);
    for (const auto& r : l.rubies) { (void)r; rubies.emplace_back(true); }
    for (const auto& b : world::buildStaticColliders(l)) {
        obstacles.push_back({b.min.x, b.max.x, b.min.z, b.max.z});
        walls.push_back({{b.min.x,b.min.z},{b.max.x,b.min.z}});
        walls.push_back({{b.max.x,b.min.z},{b.max.x,b.max.z}});
        walls.push_back({{b.max.x,b.max.z},{b.min.x,b.max.z}});
        walls.push_back({{b.min.x,b.max.z},{b.min.x,b.min.z}});
    }
    // Pedestals are solid to the player, but only mirror surfaces reflect beams.
    auto pedestal = [&](glm::ivec2 cell,float radius) {
        const auto p = l.cellCenterXZ(cell);
        obstacles.push_back({p.x-radius,p.x+radius,p.y-radius,p.y+radius});
    };
    for (const auto& m : l.mirrors) pedestal(m.cell,.53f);
    for (const auto& r : l.rubies) pedestal(r.cell,.675f);
    for (const auto& e : l.lights) pedestal(e.cell,.34f);
    trace();
    return true;
}

void GameSession::trace() {
    const auto& l = levels.current();
    beams.clear();
    std::vector<RubyTarget> targets;
    for (const auto& r : l.rubies) targets.push_back({l.cellCenter(r.cell,metrics::beamHeight), metrics::rubyRadius, true});
    auto blockers = walls;
    for (size_t i=0; i<doors.size(); ++i) if (!doors[i].isOpen()) {
        const auto p = l.cellCenterXZ(l.doors[i].cell);
        blockers.push_back({{p.x-l.cellSize*.5f,p.y},{p.x+l.cellSize*.5f,p.y}});
    }
    for (const auto& light : l.lights) {
        const auto d = world::dirToVec(light.dir);
        const auto hit = BeamTracer::traceWithRubies(l.cellCenterXZ(light.cell),
            {d.x,d.z}, mirrors, targets, 12, 200, metrics::beamHeight, blockers);
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
    for(auto& trap:traps) trap.update(dt);
    for(auto& ruby:rubies) ruby.update(dt);
    const auto p=player.position();
    const auto cell=l.worldToCell({p.x,0,p.z});
    bool onSpikes=false;
    // Check every footprint, including overlaps from neighbouring grid cells.
    for(size_t t=0;t<traps.size();++t) if(traps[t].isActive()) {
        const auto centre=l.cellCenterXZ(l.traps[t].cell);
        const float dx=std::max(0.f,std::abs(p.x-centre.x)-.55f);
        const float dz=std::max(0.f,std::abs(p.z-centre.y)-.55f);
        onSpikes |= dx*dx+dz*dz<player.radius()*player.radius();
    }
    if (l.isPit(cell) || onSpikes) {
        ++deaths;
        const auto spawn=l.cellCenterXZ(l.playerSpawn);
        player=player3d::PlayerMovement3D({spawn.x,spawn.y},4.f,.28f); return;
    }
    const int d=l.doorAt(cell);
    if (d>=0 && doors[d].isOpen() && doorLift>=.95f) advance();
}
