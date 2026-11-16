#include "gameplay/GameSession.h"
#include "world/WorldMetrics.h"
#include "physics/Intersections.h"
#include <iostream>
#include <stdexcept>

void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
    try{
        GameSession game(LEVELS_DIR);check(game.levels.loadIndex(2)&&game.reset(),"load Inner Sanctum");
        const auto level=game.levels.current();
        check(level.lights.size()==2&&game.mirrors.size()==4&&game.rubies.size()==2,"authored two-emitter/four-mirror/two-ruby design");
        check(!game.solved(),"initial pose does not complete chamber");
        for(const auto& ruby:game.rubies)check(!ruby.isActivated(),"neither initial source bypasses a mirror");
        std::vector<RubyTarget> targets;
        for(const auto& ruby:level.rubies)targets.push_back({level.cellCenter(ruby.cell,metrics::beamHeight),metrics::rubyRadius,true});
        for(size_t i=0;i<game.mirrors.size();++i){
            check(level.mirrors[i].solutionDeg>=-80&&level.mirrors[i].solutionDeg<=80,"solution is inside actual Q/E angle limits");
            game.mirrors[i].setAngle(level.mirrors[i].solutionDeg);
        }
        const std::vector<std::vector<size_t>> expected={{0,2},{1,3}};
        for(size_t source=0;source<level.lights.size();++source){
            const auto& light=level.lights[source];const auto d=world::dirToVec(light.dir);
            const auto result=BeamTracer::traceWithRubies(level.cellCenterXZ(light.cell),{d.x,d.z},game.mirrors,targets,12,200,metrics::beamHeight,game.walls);
            check(result.hitRubyIndex==int(source),"each source hits its own required ruby");
            check(result.reflectedMirrorIndices==expected[source],"actual source path uses both intended finite mirrors in order");
            check(result.path.size()==3,"each circuit has two real reflections, without an extra unintended bounce");
            for(const auto& segment:result.path){
                const glm::vec2 start(segment.start.x,segment.start.z),end(segment.end.x,segment.end.z);
                for(const auto& wall:game.walls){
                    float distance;glm::vec2 hit;
                    if(Intersections::raySegment2D(start,end-start,wall.start,wall.end,distance,hit))
                        check(distance>=glm::length(end-start)-.001f,"solution ray must not cross any solid wall or pillar");
                }
            }
            for(size_t bounce=0;bounce<2;++bounce){
                const auto incoming=glm::normalize(result.path[bounce].end-result.path[bounce].start);
                const auto outgoing=glm::normalize(result.path[bounce+1].end-result.path[bounce+1].start);
                const auto n=game.mirrors[expected[source][bounce]].getNormal();
                check(glm::dot(glm::reflect(incoming,glm::vec3(n.x,0,n.y)),outgoing)>.99999f,"each reflection obeys the physical surface normal");
            }
        }
        // More than one precision-input frame of tolerance in every joint
        // direction: the puzzle must not require exact designer-only angles.
        for(unsigned mask=0;mask<16;++mask){
            for(size_t i=0;i<4;++i)game.mirrors[i].setAngle(level.mirrors[i].solutionDeg+((mask&(1u<<i))?.2f:-.2f));
            for(size_t source=0;source<level.lights.size();++source){
                const auto& light=level.lights[source];const auto d=world::dirToVec(light.dir);
                const auto result=BeamTracer::traceWithRubies(level.cellCenterXZ(light.cell),{d.x,d.z},game.mirrors,targets,12,200,metrics::beamHeight,game.walls);
                check(result.hitRubyIndex==int(source),"both circuits tolerate all +/-0.2 degree precision-input combinations");
            }
        }
        game.trace();check(game.solved(),"both actual ruby hits unlock the final exit");
        check(game.reset(),"reset chamber");
        for(size_t i=0;i<game.mirrors.size();++i)check(game.mirrors[i].getAngle()==level.mirrors[i].angleDeg,"reset restores authored starting angle");
        for(const auto& ruby:game.rubies)check(!ruby.isActivated()&&ruby.sealingProgress()==0,"reset clears both ruby and animation state");
        std::cout<<"PASS two independent real finite-mirror circuits, solid geometry, physical normals, angle limits, door and reset\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
