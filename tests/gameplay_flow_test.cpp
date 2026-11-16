#include "gameplay/GameSession.h"
#include "gameplay/GameplayInputGate.h"
#include "SessionTestHelpers.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void frames(GameSession& g,int n){for(int i=0;i<n;++i)g.update(0,0,.05f);}
void until(GameSession& g,GameSession::Phase target){for(int i=0;i<160&&g.phase!=target;++i)g.update(1,1,.05f);check(g.phase==target,"phase reached within finite time");}
int main(){try{
    using Phase=GameSession::Phase;using Character=GameSession::CharacterState;
    GameplayInputGate gate;
    check(!gate.allow(false,true)&&!gate.allow(true,true),"held countdown key cannot leak through GO");
    check(gate.allow(true,false)&&gate.allow(true,true),"release then fresh input works");
    GameSession g(LEVELS_DIR);check(g.restart(),"start campaign");
    check(g.phase==Phase::Countdown&&g.countdownLabel()=="3","entry starts at 3");
    const auto p=g.player.position();const auto angle=g.mirrors[0].getAngle();
    for(int i=0;i<18;++i){check(!g.rotate(10),"countdown rejects mirror input");g.update(1,1,.05f);}
    check(g.player.position().x==p.x&&g.player.position().z==p.z&&g.mirrors[0].getAngle()==angle&&!g.solved(),"countdown freezes player and puzzle");
    g.paused=true;const float clock=g.phaseTime;frames(g,40);check(g.phaseTime==clock,"countdown pause");g.paused=false;
    frames(g,3);check(g.countdownLabel()=="2","second number");frames(g,20);check(g.countdownLabel()=="1","third number");frames(g,20);check(g.countdownLabel()=="GO!","short GO display");finishEntry(g);check(g.controlsEnabled(),"controls enabled after GO");
    check(g.levels.loadIndex(3)&&g.reset(),"timed chamber reset");g.phase=Phase::Countdown;
    const auto initial=g.traps[0].cyclePosition();frames(g,30);check(g.traps[0].cyclePosition()==initial,"timed hazards frozen throughout countdown");
    g.paused=true;frames(g,20);check(g.traps[0].cyclePosition()==initial,"timed hazards frozen during pause");g.paused=false;finishEntry(g);
    // Freeze a solved chamber's progress through a pit death and respawn.
    check(g.levels.loadIndex(2)&&g.reset(),"death chamber");
    for(size_t i=0;i<g.mirrors.size();++i)g.mirrors[i].setAngle(g.levels.current().mirrors[i].solutionDeg);g.trace();check(g.solved(),"seal objectives before death");
    const float savedAngle=g.mirrors[0].getAngle();glm::vec2 pit(0);
    const auto& l=g.levels.current();for(int y=0;y<l.height;++y)for(int x=0;x<l.width;++x)if(l.isPit({x,y}))pit=l.cellCenterXZ({x,y});
    g.player=player3d::PlayerMovement3D({pit.x,pit.y});g.update(0,0,.016f);
    check(g.characterState==Character::Dying&&g.deaths==1,"pit begins one death event");
    const auto deathPosition=g.player.position();g.update(1,1,.05f);
    check(!g.controlsEnabled()&&!g.rotate(15)&&g.player.position().x==deathPosition.x&&g.player.position().z==deathPosition.z,"death locks movement and interaction");
    g.paused=true;const float deathTime=g.characterTime;frames(g,30);check(g.characterTime==deathTime,"pause freezes death animation");g.paused=false;
    for(int i=0;i<25&&g.characterState==Character::Dying;++i)g.update(1,1,.05f);
    check(g.characterState==Character::Respawning,"death reaches distinct respawning state");
    const auto spawn=l.cellCenterXZ(l.playerSpawn);check(glm::length(glm::vec2(g.player.position().x,g.player.position().z)-spawn)<.001f,"correct respawn position");
    check(g.solved()&&g.mirrors[0].getAngle()==savedAngle&&g.deaths==1,"respawn preserves puzzle and counter");
    g.update(0,0,.016f);check(g.characterState==Character::Invulnerable&&g.controlsEnabled(),"protection starts after respawn");
    g.update(0,0,.05f);const float bright=g.characterOpacity();frames(g,2);check(g.characterOpacity()!=bright,"time based opacity flicker");
    g.paused=true;const float immunity=g.characterTime;frames(g,40);check(g.characterTime==immunity,"pause freezes immunity");g.paused=false;
    g.player=player3d::PlayerMovement3D({pit.x,pit.y});frames(g,10);check(g.deaths==1,"pit cannot retrigger during immunity");
    // Interaction remains available during immunity.
    const auto near=l.cellCenterXZ(l.mirrors[0].cell+glm::ivec2(-1,0));g.player=player3d::PlayerMovement3D({near.x,near.y});check(g.rotate(1),"protected Ranger can interact");
    g.player=player3d::PlayerMovement3D({spawn.x,spawn.y});frames(g,40);check(g.characterState==Character::Alive&&g.characterOpacity()==1,"protection and flicker expire");
    g.player=player3d::PlayerMovement3D({pit.x,pit.y});g.update(0,0,.016f);check(g.deaths==2,"hazards dangerous again after expiry");
    check(g.reset()&&g.characterState==Character::Alive&&g.phase==Phase::Playing,"reset clears death without replaying countdown");
    check(g.restart()&&g.deaths==0&&g.characterState==Character::Alive&&g.phase==Phase::Countdown,"restart clears all lifecycle state");finishEntry(g);
    for(auto& ruby:g.rubies)ruby.hitByLight();g.trace();check(g.advance()&&g.phase==Phase::FadeOut&&g.levels.index()==0,"old chamber retained during fade out");
    until(g,Phase::Title);check(g.levels.index()==1&&g.levels.current().name=="Hall of Spikes","title comes from newly loaded metadata");
    const auto nextSpawn=g.player.position();g.paused=true;const float titleClock=g.phaseTime;frames(g,40);check(g.phaseTime==titleClock,"transition pauses");g.paused=false;
    until(g,Phase::FadeIn);check(g.player.position().x==nextSpawn.x,"transition cannot move player");until(g,Phase::Countdown);check(g.countdownLabel()=="3","reveal followed by new countdown");finishEntry(g);
    check(g.levels.loadIndex(4)&&g.reset(),"final chamber");for(auto& ruby:g.rubies)ruby.hitByLight();g.trace();check(g.advance(),"final exit begins fade");until(g,Phase::Victory);
    check(g.victory&&g.levels.index()==4,"fifth exit reaches victory, never sixth level");
    check(g.reset()&&!g.victory&&g.characterState==Character::Alive,"reset completed chamber clears victory state");
    std::cout<<"PASS countdown gates, pause, death/respawn/protection, retained puzzles, transition ordering and fifth victory\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
