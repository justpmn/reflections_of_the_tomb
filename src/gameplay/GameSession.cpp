#include "GameSession.h"
#include "world/TempleMesh.h"
#include "world/WorldMetrics.h"
#include <algorithm>
#include <cmath>

GameSession::GameSession(const std::string& dir)
    : levels({dir + "/level01.txt", dir + "/level02.txt", dir + "/level03.txt", dir + "/level04.txt", dir + "/level05.txt"}) {}

bool GameSession::restart() {
    victory = false; deaths = 0;
    return levels.loadFirst() && initializeLevel(true);
}
bool GameSession::reset() {
    if(!levels.reload())return false;
    victory=false;return initializeLevel();
}

bool GameSession::initializeLevel(bool countdown) {
    const Phase loadedPhase=countdown?Phase::Countdown:Phase::Playing;
    phase=Phase::Countdown;phaseTime=0;paused=false;
    if(countdown)++entrySerial;
    characterState=CharacterState::Alive;characterTime=0;fellInPit=false;
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
    trace(); // Populate visible beams without activating an objective while loading.
    phase=loadedPhase;
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
        if (controlsEnabled() && hit.hitRubyIndex >= 0) rubies.at(hit.hitRubyIndex).hitByLight();
    }
    if(!controlsEnabled())return;
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
    if(!controlsEnabled())return false;
    const int i = nearbyMirror();
    if (i < 0) return false;
    mirrors[i].setAngle(mirrors[i].getAngle()+degrees);
    trace(); return true;
}
bool GameSession::solved() const {
    return !doors.empty() && std::all_of(doors.begin(),doors.end(),[](const Door& d){return d.isOpen();});
}
bool GameSession::controlsEnabled()const {
    return !paused&&phase==Phase::Playing&&(characterState==CharacterState::Alive||characterState==CharacterState::Invulnerable);
}
float GameSession::characterOpacity()const {
    if(characterState==CharacterState::Respawning)return .25f;
    if(characterState!=CharacterState::Invulnerable)return 1;
    return std::fmod(characterTime,.24f)<.12f?1.f:.32f;
}
std::string GameSession::countdownLabel()const {
    if(phase!=Phase::Countdown)return {};
    return phaseTime<1?"3":phaseTime<2?"2":phaseTime<3?"1":"GO!";
}
bool GameSession::advance() {
    if (!solved()||!controlsEnabled()) return false;
    phase=Phase::FadeOut;phaseTime=0;return true;
}
void GameSession::update(float x,float z,float dt) {
    if (victory||paused||!std::isfinite(dt)) return;
    dt = std::clamp(dt,0.f,.05f);
    if(phase!=Phase::Playing) {
        phaseTime+=dt;
        if(phase==Phase::Countdown&&phaseTime>=3.45f){phase=Phase::Playing;phaseTime=0;}
        else if(phase==Phase::FadeOut&&phaseTime>=.45f) {
            if(levels.isLastLevel()){victory=true;phase=Phase::Victory;}
            else if(levels.next()&&initializeLevel()){phase=Phase::Title;phaseTime=0;}
        }else if(phase==Phase::Title&&phaseTime>=2.f){phase=Phase::FadeIn;phaseTime=0;}
        else if(phase==Phase::FadeIn&&phaseTime>=.45f){phase=Phase::Countdown;phaseTime=0;++entrySerial;}
        return;
    }
    if(characterState==CharacterState::Dying) {
        characterTime+=dt;
        if(characterTime>=.95f){
            const auto spawn=levels.current().cellCenterXZ(levels.current().playerSpawn);
            player=player3d::PlayerMovement3D({spawn.x,spawn.y},4.f,.28f);
            characterState=CharacterState::Respawning;characterTime=0;
        }
        return;
    }
    if(characterState==CharacterState::Respawning){characterState=CharacterState::Invulnerable;characterTime=0;return;}
    if(characterState==CharacterState::Invulnerable){characterTime+=dt;if(characterTime>=2.f){characterState=CharacterState::Alive;characterTime=0;}}
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
    if (characterState==CharacterState::Alive&&(l.isPit(cell) || onSpikes)) {
        ++deaths;characterState=CharacterState::Dying;characterTime=0;fellInPit=l.isPit(cell);
        // Clear the movement flag without moving or rebuilding puzzle entities.
        player.update(0,0,0,{0,l.width*l.cellSize,0,l.height*l.cellSize},solid);
        return;
    }
    const int d=l.doorAt(cell);
    if (d>=0 && doors[d].isOpen() && doorLift>=.95f) advance();
}
