#include "gameplay/GameSession.h"
#include "world/WorldMetrics.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>
#include <stdexcept>

void check(bool ok,const std::string& s) {if(!ok) throw std::runtime_error(s);}
struct Envelope {glm::vec2 lo,hi;};
struct Face {int axis;float plane,lo,hi;};

// Each rotating finite surface is enclosed by an axis-aligned square for EVERY
// real-valued angle. A solid wall spanning the entire projected envelope blocks
// all possible connections, including fractional angles and repeated bounces.
std::vector<Face> mergedFaces(const std::vector<WallSegment>& walls) {
    std::vector<Face> faces;
    for(const auto& w:walls) {
        const int axis=std::abs(w.start.x-w.end.x)<.001f?0:1;
        faces.push_back({axis,w.start[axis],std::min(w.start[1-axis],w.end[1-axis]),std::max(w.start[1-axis],w.end[1-axis])});
    }
    std::sort(faces.begin(),faces.end(),[](const Face& a,const Face& b){
        if(a.axis!=b.axis)return a.axis<b.axis;
        if(a.plane!=b.plane)return a.plane<b.plane;
        return a.lo<b.lo;
    });
    std::vector<Face> out;
    for(const auto& f:faces) {
        if(!out.empty()&&out.back().axis==f.axis&&std::abs(out.back().plane-f.plane)<.001f&&f.lo<=out.back().hi+.001f)
            out.back().hi=std::max(out.back().hi,f.hi);
        else out.push_back(f);
    }
    return out;
}
bool blocked(const Envelope& a,const Envelope& b,const std::vector<Face>& faces) {
    for(const auto& f:faces) {
        const int k=f.axis,t=1-k;
        if(!((a.hi[k]<f.plane&&b.lo[k]>f.plane)||(b.hi[k]<f.plane&&a.lo[k]>f.plane))) continue;
        float low=1e20f,high=-1e20f;
        for(int i=0;i<4;++i) for(int j=0;j<4;++j) {
            const glm::vec2 p((i&1)?a.hi.x:a.lo.x,(i&2)?a.hi.y:a.lo.y);
            const glm::vec2 q((j&1)?b.hi.x:b.lo.x,(j&2)?b.hi.y:b.lo.y);
            const float v=p[t]+(q[t]-p[t])*(f.plane-p[k])/(q[k]-p[k]);
            low=std::min(low,v);high=std::max(high,v);
        }
        // Strict interior margins avoid claiming proof from a grazing wall corner.
        if(low>f.lo+.001f && high<f.hi-.001f) return true;
    }
    return false;
}
std::vector<RubyTarget> targets(const world::Level& l) {
    std::vector<RubyTarget> out;
    for(const auto& r:l.rubies) out.push_back({l.cellCenter(r.cell,metrics::beamHeight),metrics::rubyRadius,true});
    return out;
}
void proveContinuousNecessity(const GameSession& g,int missing) {
    const auto& l=g.levels.current();
    const auto faces=mergedFaces(g.walls);
    std::vector<Envelope> nodes;
    for(size_t i=0;i<g.mirrors.size();++i) if(int(i)!=missing) {
        const auto c=g.mirrors[i].getCenter();const float r=metrics::mirrorLength*.5f;
        nodes.push_back({c-glm::vec2(r),c+glm::vec2(r)});
    }
    const size_t count=nodes.size();
    for(const auto& ruby:l.rubies) {
        const auto c=l.cellCenterXZ(ruby.cell);nodes.push_back({c-glm::vec2(metrics::rubyRadius),c+glm::vec2(metrics::rubyRadius)});
    }
    std::vector<bool> seen(nodes.size(),false);std::queue<size_t> todo;
    for(const auto& e:l.lights) {
        const auto origin=l.cellCenterXZ(e.cell);const auto d3=world::dirToVec(e.dir);const glm::vec2 d(d3.x,d3.z);
        const auto empty=BeamTracer::trace(origin,d,{},12,200,metrics::beamHeight,g.walls);
        const float limit=glm::length(empty.front().end-empty.front().start);
        for(size_t i=0;i<nodes.size();++i) {
            const auto c=(nodes[i].lo+nodes[i].hi)*.5f;
            // Circumscribed circle: conservative, cannot miss a possible first hit.
            const float radius=glm::length(nodes[i].hi-nodes[i].lo)*.5f;
            const float along=glm::dot(c-origin,d);
            if(along+radius>=0 && along-radius<limit && glm::length(c-origin-d*along)<=radius) {seen[i]=true;todo.push(i);}
        }
    }
    while(!todo.empty()) {
        const auto a=todo.front();todo.pop();
        if(a>=count) continue; // Absorbing rubies cannot reflect to other nodes.
        for(size_t b=0;b<nodes.size();++b) if(!seen[b] && !blocked(nodes[a],nodes[b],faces)) {seen[b]=true;todo.push(b);}
    }
    bool impossible=false;
    for(const auto& door:l.doors) for(int ruby:door.needsRubies) if(!seen[count+ruby]) impossible=true;
    check(impossible,"Continuous visibility proof failed: "+l.name+" missing mirror "+std::to_string(missing));
}
int main() {
    try {
        GameSession g(LEVELS_DIR);check(g.restart(),"load campaign");
        for(int level=0;level<3;++level) {
            check(g.levels.loadIndex(level)&&g.reset(),"load chamber");const auto l=g.levels.current();
            check(!g.solved(),"initial puzzle is unsolved");
            std::vector<bool> used(g.mirrors.size(),false),lit(g.rubies.size(),false);
            for(size_t i=0;i<g.mirrors.size();++i) g.mirrors[i].setAngle(l.mirrors[i].solutionDeg);
            auto sample=[&](const std::vector<Mirror>& mirrors) {
                for(const auto& light:l.lights) {
                    const auto d=world::dirToVec(light.dir);
                    const auto result=BeamTracer::traceWithRubies(l.cellCenterXZ(light.cell),{d.x,d.z},mirrors,targets(l),12,200,metrics::beamHeight,g.walls);
                    if(result.hitRubyIndex>=0) lit[result.hitRubyIndex]=true;
                    for(auto index:result.reflectedMirrorIndices) used[index]=true;
                }
            };
            sample(g.mirrors);g.trace();
            for(size_t i=0;i<l.mirrors.size();++i) if(l.mirrors[i].hasAlternateSolution) {
                g.mirrors[i].setAngle(l.mirrors[i].alternateSolutionDeg);sample(g.mirrors);g.trace();
            }
            check(g.solved(),"intended continuous solution sequence seals every ruby");
            for(bool value:used) check(value,"every intended mirror is physically reflected from");
            for(bool value:lit) check(value,"every ruby reached by intended continuous path");
            if(level==1) {
                // Calibration: deleting the baffle really does permit the old
                // one-mirror diagonal shortcut. The negative test is not vacuous.
                auto single=std::vector<Mirror>{g.mirrors[0]};
                const auto delta=l.cellCenterXZ(l.rubies[0].cell)-single[0].getCenter();
                single[0].setAngle(glm::degrees(std::atan2(delta.y,delta.x))*.5f);
                const float left=l.cellSize,right=(l.width-1)*l.cellSize,top=l.cellSize,bottom=(l.height-1)*l.cellSize;
                const std::vector<WallSegment> boundary={{{left,top},{right,top}},{{right,top},{right,bottom}},{{right,bottom},{left,bottom}},{{left,bottom},{left,top}}};
                const auto origin=l.cellCenterXZ(l.lights[0].cell);
                check(BeamTracer::traceWithRubies(origin,{1,0},single,targets(l),12,200,metrics::beamHeight,boundary).hitRubyIndex==0,"baffle-free calibration must expose diagonal bypass");
                check(BeamTracer::traceWithRubies(origin,{1,0},single,targets(l),12,200,metrics::beamHeight,g.walls).hitRubyIndex<0,"authored baffle must block that real diagonal bypass");
            }
            for(size_t missing=0;missing<g.mirrors.size();++missing) {
                proveContinuousNecessity(g,int(missing));
                std::vector<Mirror> remaining;
                for(size_t i=0;i<g.mirrors.size();++i) if(i!=missing) remaining.push_back(g.mirrors[i]);
                std::vector<bool> reachable(l.rubies.size(),false);size_t configurations=0;
                auto disabledWalls=g.walls;
                disabledWalls.push_back({g.mirrors[missing].getEndpointA(),g.mirrors[missing].getEndpointB()});
                std::vector<bool> reachableDisabled(l.rubies.size(),false);
                // Factor independent optical components. The envelope graph is
                // conservative for all angles, so a ray cannot bounce between
                // disconnected components. Removing the other components only
                // enlarges reachable paths; every integer-angle state is covered
                // without an unnecessary 161^3 Cartesian product for two circuits.
                const auto faces=mergedFaces(g.walls);
                std::vector<std::vector<Mirror>> components;
                std::vector<bool> assigned(remaining.size(),false);
                for(size_t seed=0;seed<remaining.size();++seed) if(!assigned[seed]) {
                    std::vector<Mirror> group;std::queue<size_t> queue;queue.push(seed);assigned[seed]=true;
                    while(!queue.empty()) {
                        const auto from=queue.front();queue.pop();group.push_back(remaining[from]);
                        const auto p=remaining[from].getCenter();const glm::vec2 extent(metrics::mirrorLength*.5f);
                        for(size_t to=0;to<remaining.size();++to) if(!assigned[to]) {
                            const auto q=remaining[to].getCenter();
                            if(!blocked({p-extent,p+extent},{q-extent,q+extent},faces)) {assigned[to]=true;queue.push(to);}
                        }
                    }
                    components.push_back(std::move(group));
                }
                components.push_back({}); // Also verify the no-reflection/direct rays.
                for(auto& component:components) {
                    std::function<void(size_t)> search=[&](size_t depth) {
                        if(depth<component.size()) {
                            for(int angle=-80;angle<=80;++angle) {component[depth].setAngle(float(angle));search(depth+1);}return;
                        }
                        ++configurations;
                        for(const auto& light:l.lights) {
                            const auto d=world::dirToVec(light.dir);
                            const auto result=BeamTracer::traceWithRubies(l.cellCenterXZ(light.cell),{d.x,d.z},component,targets(l),12,200,metrics::beamHeight,g.walls);
                            if(result.hitRubyIndex>=0) reachable[result.hitRubyIndex]=true;
                            const auto disabled=BeamTracer::traceWithRubies(l.cellCenterXZ(light.cell),{d.x,d.z},component,targets(l),12,200,metrics::beamHeight,disabledWalls);
                            if(disabled.hitRubyIndex>=0) reachableDisabled[disabled.hitRubyIndex]=true;
                        }
                    };
                    search(0);
                }
                bool impossible=false;
                for(const auto& door:l.doors) for(int ruby:door.needsRubies) if(!reachable[ruby]) impossible=true;
                check(impossible,"angle search found a bypass, even with permanent sealing");
                bool disabledImpossible=false;
                for(const auto& door:l.doors) for(int ruby:door.needsRubies) if(!reachableDisabled[ruby]) disabledImpossible=true;
                check(disabledImpossible,"disabled opaque mirror must not permit campaign completion");
                std::cout<<"PASS "<<l.name<<" mirror "<<missing<<": continuous envelope proof + "<<configurations<<" real-tracer configurations\n";
            }
        }
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
