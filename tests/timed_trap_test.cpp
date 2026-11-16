#include "gameplay/GameSession.h"
#include "SessionTestHelpers.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void near(double a,double b,const char* m){check(std::abs(a-b)<.0003,m);}
int main(){try{
    Trap t;t.configureTimed(5,1.2f,2.4f,0);
    check(t.state()==Trap::State::Retracted&&!t.isActive()&&t.extension()==0,"initial safe state");
    t.update(5.01f);check(t.state()==Trap::State::Warning&&!t.isActive()&&t.extension()==0,"warning is visible before any dangerous extension");
    t.update(1.0f);check(t.state()==Trap::State::Warning&&!t.isActive(),"at least one second reaction time");
    t.update(.3f);check(t.state()==Trap::State::Extended&&t.extension()>0&&t.extension()<1,"smooth extension starts");
    t.update(.4f);check(t.isActive()&&t.extension()==1,"fully raised spikes dangerous");
    t.update(1.7f);check(t.extension()>0&&t.extension()<1,"smooth retraction");
    t.update(.3f);check(t.state()==Trap::State::Retracted&&!t.isActive()&&t.extension()==0,"cycle returns safe");
    Trap a,b,c;a.configureTimed(5,1.2f,2.4f,1.7f);b=a;c=a;
    for(int i=0;i<6000;++i)a.update(1.f/60);
    for(int i=0;i<3000;++i)b.update(1.f/30);
    for(int i=0;i<14400;++i)c.update(1.f/144);
    near(a.cyclePosition(),b.cyclePosition(),"30/60 FPS equivalent phase");near(a.cyclePosition(),c.cyclePosition(),"144/60 FPS equivalent phase");near(a.extension(),c.extension(),"frame independent animation");
    a.reset();near(a.cyclePosition(),1.7,"reset restores offset");a.deactivate();a.update(100);check(a.isDisabled()&&!a.isActive()&&a.extension()==0,"explicit disable stops cycle");a.reset();check(!a.isDisabled(),"reset restores timed mechanism");
    check(!world::loadLevelFromString("name: bad\ngrid:\n#####\n#SLR#\n#MTD#\n#####\nend\ntrap 0 timed 0.1 0.1 0.1 0\ndoor 0 needs 0\n").ok,"unfair timings rejected by loader");
    GameSession g(LEVELS_DIR);check(g.restart(),"load session");
    for(int index:{3,4}) {
        check(g.levels.loadIndex(index)&&g.reset(),"new level load");const auto l=g.levels.current();
        check(l.name==(index==3?"The Gauntlet":"The Final Trial"),"new campaign names");
        int timer=-1;std::vector<double> initial;
        for(size_t i=0;i<g.traps.size();++i){initial.push_back(g.traps[i].cyclePosition());if(g.traps[i].isTimed()&&timer<0)timer=int(i);}
        check(timer>=0,"authored timers");const auto p=l.cellCenterXZ(l.traps[timer].cell);
        g.player=player3d::PlayerMovement3D({p.x,p.y});g.update(0,0,.016f);check(g.deaths==0,"retracted overlap safe");
        g.rubies[0].hitByLight();g.trace();check(!g.solved()&&!g.advance(),"partial ruby activation cannot unlock exit");
        // Direct fixture advances trap simulation without moving the player.
        const auto& spec=l.traps[timer];g.traps[timer].update(spec.safeSeconds+spec.warningSeconds+.5f);
        check(g.traps[timer].isActive(),"raised fixture state");
        g.player=player3d::PlayerMovement3D({p.x+.79f,p.y+.79f});g.update(0,0,.016f);
        check(g.deaths==0,"circle outside plate corner does not collide with enclosing square");
        g.player=player3d::PlayerMovement3D({p.x,p.y});g.update(0,0,.016f);
        check(g.deaths==1&&g.rubies[0].isActivated(),"extended contact respawns but preserves seal");
        finishRespawn(g);
        for(int frame=0;frame<20;++frame)g.update(0,0,.016f);
        check(g.deaths==1,"safe spawn cannot repeatedly retrigger hazard");
        check(g.reset(),"timed reset");for(size_t i=0;i<g.traps.size();++i)near(g.traps[i].cyclePosition(),initial[i],"all authored phases restored");
        bool independent=false;for(const auto& trap:g.traps)if(trap.isTimed()&&trap.cyclePosition()!=g.traps[timer].cyclePosition())independent=true;
        check(independent,"independently phased groups");
        for(const auto& spec2:l.traps)if(spec2.timed)check(spec2.safeSeconds>(l.cellSize+2*g.player.radius())/4.f+1.0f,"crossing window exceeds physical crossing time with reaction margin");
        for(auto& ruby:g.rubies)ruby.hitByLight();g.trace();check(g.solved(),"all three or two objectives unlock door");
        bool timedStillRunning=false;for(const auto& trap:g.traps)if(trap.isTimed()&&!trap.isDisabled())timedStillRunning=true;
        check(timedStillRunning,"unlinked timed traps continue after sealing");
        check(g.reset(),"restore for next test");g.deaths=0;
    }
    std::cout<<"PASS states, frame rates, warning margin, safe/hazard contact, permanent seals, independent phases and resets\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
