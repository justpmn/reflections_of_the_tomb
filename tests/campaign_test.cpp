#include "gameplay/GameSession.h"
#include <iostream>
#include <stdexcept>
#include <queue>
#include <algorithm>

void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }

// Navigate real movement/collision along a BFS floor route, never teleport to the exit.
void walkTo(GameSession& g, glm::ivec2 goal, bool expectExit=false) {
    const auto l=g.levels.current();
    const auto p=g.player.position();
    const auto start=l.worldToCell({p.x,0,p.z});
    auto index=[&](glm::ivec2 c){return c.y*l.width+c.x;};
    std::vector<int> parent(l.width*l.height,-1); std::queue<glm::ivec2> todo;
    todo.push(start); parent[index(start)]=index(start);
    const glm::ivec2 directions[]={{1,0},{-1,0},{0,1},{0,-1}};
    while(!todo.empty()) {
        auto c=todo.front();todo.pop();
        for(auto d:directions) {
            auto n=c+d;
            if(!l.inBounds(n)||!l.isWalkable(n)||parent[index(n)]>=0) continue;
            const int trap=l.trapAt(n);
            if(trap>=0 && g.traps[trap].isActive()) continue;
            parent[index(n)]=index(c);todo.push(n);
        }
    }
    require(parent[index(goal)]>=0,"Exit must be reachable on foot");
    std::vector<glm::ivec2> route;
    for(int i=index(goal);i!=index(start);i=parent[i]) route.push_back({i%l.width,i/l.width});
    std::reverse(route.begin(),route.end());
    const int level=g.levels.index();
    for(auto cell:route) {
        const auto target=l.cellCenterXZ(cell);
        bool arrived=false;
        for(int step=0;step<400;++step) {
            const auto pos=g.player.position(); auto delta=target-glm::vec2(pos.x,pos.z);
            if(glm::length(delta)<.07f) {arrived=true;break;}
            delta=glm::normalize(delta);
            g.update(delta.x,delta.y,.016f);
            if(g.victory||g.levels.index()!=level) return;
        }
        require(arrived,"Player collision unexpectedly blocks route to exit");
    }
    require(!expectExit,"Entering the unlocked exit must advance campaign");
}

int main() {
    try {
        GameSession g(LEVELS_DIR); require(g.restart(),"Load campaign");
        require(!g.rotate(10),"Remote mirror interaction must be rejected");
        for(int level=0;level<3;++level) {
            require(g.levels.index()==level,"Campaign progression order");
            require(!g.solved(),"Level must start unsolved");
            require(!g.advance(),"Locked door must reject progression");
            const auto l=g.levels.current();
            for(size_t i=0;i<g.mirrors.size();++i) g.mirrors[i].setAngle(l.mirrors[i].solutionDeg);
            g.trace();
            require(g.solved(),"Actual BeamTracer solution must unlock every required door");
            for(const auto& ruby:g.rubies) require(ruby.isActivated(),"Every ruby must be reached by physical beams");
            for(const auto& trap:g.traps) require(!trap.isActive(),"Linked traps deactivate");
            // Solutions must include the intended number of reflections, not a direct shortcut.
            require(g.beams.size()>=l.mirrors.size()+l.lights.size(),"Real reflected path segment count");
            for(const auto& beam:g.beams) require(std::abs(beam.start.y-1.3f)<.001f,"Beam height matches world targets");
            require(g.reset(),"Reset current chamber");
            require(!g.solved(),"Reset closes doors and clears ruby activation");
            for(size_t i=0;i<g.mirrors.size();++i) require(g.mirrors[i].getAngle()==l.mirrors[i].angleDeg,"Reset mirror angle");
            // Solve again using only walking and the proximity-gated rotate API.
            for(size_t i=0;i<g.mirrors.size();++i) {
                const auto cell=l.mirrors[i].cell;
                const glm::ivec2 options[]={{-1,0},{1,0},{0,-1},{0,1}};
                bool reached=false;
                for(auto offset:options) if(l.isWalkable(cell+offset) && !l.isPit(cell+offset)) {
                    walkTo(g,cell+offset);reached=true;break;
                }
                require(reached && g.nearbyMirror()==static_cast<int>(i),"Walking reaches intended interactive mirror");
                const float difference=l.mirrors[i].solutionDeg-g.mirrors[i].getAngle();
                const int steps=static_cast<int>(std::abs(difference));
                for(int step=0;step<steps;++step) require(g.rotate(difference<0?-1.f:1.f),"Nearby mirror must rotate");
            }
            require(g.solved(),"Player-controlled mirror rotations solve the level");
            for(int i=0;i<25;++i) g.update(0,0,.05f);
            walkTo(g,l.doors[0].cell,true);
            std::cout<<"PASS real beams, reset, collision route and exit: "<<l.name<<'\n';
        }
        require(g.victory,"Final exit reaches victory");
        require(g.restart()&&!g.victory&&g.levels.index()==0&&!g.solved(),"Restart clears completed campaign");
        require(g.levels.loadIndex(2)&&g.reset(),"Load hazard chamber");
        const auto l=g.levels.current();
        const auto spike=l.cellCenterXZ(l.traps[0].cell);
        // Test fixture places the player on a hazard; gameplay must respawn at the authored spawn.
        g.player=player3d::PlayerMovement3D({spike.x,spike.y});g.update(0,0,.016f);
        require(g.deaths==1,"Active spikes cause respawn");
        const auto spawn=l.cellCenterXZ(l.playerSpawn);
        require(glm::length(glm::vec2(g.player.position().x,g.player.position().z)-spawn)<.01f,"Hazard respawn uses level spawn");
        const auto pit=l.cellCenterXZ({6,4});
        g.player=player3d::PlayerMovement3D({pit.x,pit.y});g.update(0,0,.016f);
        require(g.deaths==2,"Pits cause respawn");
    } catch(const std::exception& e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
    return 0;
}
