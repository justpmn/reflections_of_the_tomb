#include "ControlReplay.h"
#include "gameplay/GameSession.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <set>
#include <stdexcept>

struct ControlReplay::State {
    int tick=0,level=-1,cursor=0,boot=0,fineFrames=0;
    bool done=false,walkingExit=false;
    bool resetRequested=false,resetExercised=false;
    bool countdownProbeStarted=false,countdownInputDone=false;
    int heldGoFrames=0;
    player3d::PositionXZ countdownPosition{};
    float countdownAngle=0,countdownPauseTime=0;
    int timedPauseTick=0;
    bool timedPauseDone=false,timedPauseStarted=false;
    std::vector<double> pausedPhases;
    std::set<int> held;
    std::vector<int> order;
    std::vector<glm::ivec2> route;
    size_t waypoint=0;
    std::vector<bool> sealed;
    std::string capture;
    player3d::PositionXZ pausePosition{};
    static void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
    void keys(GLFWwindow* window,const std::set<int>& wanted) {
#ifdef _WIN32
        auto message=[&](int key,bool down) {
            const UINT vk=key==GLFW_KEY_ENTER?VK_RETURN:key==GLFW_KEY_ESCAPE?VK_ESCAPE:key==GLFW_KEY_LEFT_SHIFT?VK_SHIFT:UINT(key);
            const UINT scan=key==GLFW_KEY_LEFT_SHIFT?0x2a:MapVirtualKeyW(vk,MAPVK_VK_TO_VSC);
            LPARAM parameter=1|(LPARAM(scan)<<16);
            if(!down)parameter|=LPARAM(3)<<30;
            require(PostMessageW(glfwGetWin32Window(window),down?WM_KEYDOWN:WM_KEYUP,vk,parameter)!=0,"native key message failed");
        };
        for(int key:held)if(!wanted.count(key))message(key,false);
        for(int key:wanted)if(!held.count(key))message(key,true);
        held=wanted;
#else
        (void)window;(void)wanted;
        throw std::runtime_error("native control replay currently requires Windows");
#endif
    }
    bool plan(const GameSession& game,glm::ivec2 goal) {
        const auto& l=game.levels.current();const auto p=game.player.position();
        const auto start=l.worldToCell({p.x,0,p.z});
        auto index=[&](glm::ivec2 c){return c.y*l.width+c.x;};
        std::vector<int> parent(l.width*l.height,-1);std::queue<glm::ivec2> queue;
        queue.push(start);parent[index(start)]=index(start);
        const glm::ivec2 directions[]={{-1,0},{1,0},{0,-1},{0,1}};
        while(!queue.empty()) {
            const auto c=queue.front();queue.pop();
            for(auto direction:directions) {
                const auto n=c+direction;
                if(!l.inBounds(n)||!l.isWalkable(n)||parent[index(n)]>=0)continue;
                const int trap=l.trapAt(n);
                const int previousTrap=l.trapAt(c);
                if(trap>=0&&previousTrap>=0&&game.traps[trap].isTimed()&&game.traps[previousTrap].isTimed())continue;if(trap>=0&&!game.traps[trap].isTimed()&&game.traps[trap].isActive())continue;
                parent[index(n)]=index(c);queue.push(n);
            }
        }
        if(!l.inBounds(goal)||parent[index(goal)]<0)return false;
        route.clear();waypoint=0;
        for(int i=index(goal);i!=index(start);i=parent[i])route.push_back({i%l.width,i/l.width});
        std::reverse(route.begin(),route.end());
        route.push_back(goal);return true;
    }
    bool walk(const GameSession& game,std::set<int>& wanted) {
        const auto& l=game.levels.current();const auto p=game.player.position();
        while(waypoint<route.size()) {
            const auto delta=l.cellCenterXZ(route[waypoint])-glm::vec2(p.x,p.z);
            if(glm::length(delta)<.07f){++waypoint;continue;}
            const int trap=l.trapAt(route[waypoint]);
            const auto cell=l.worldToCell({p.x,0,p.z});
            if(trap>=0&&game.traps[trap].isTimed()&&cell!=route[waypoint]&&!game.traps[trap].safeToCross(1.15))return false;
            if(std::abs(delta.x)>std::abs(delta.y))wanted.insert(delta.x>0?GLFW_KEY_D:GLFW_KEY_A);
            else wanted.insert(delta.y>0?GLFW_KEY_S:GLFW_KEY_W);
            return false;
        }
        return true;
    }
};
ControlReplay::ControlReplay():state(std::make_unique<State>()){}
ControlReplay::~ControlReplay()=default;
void ControlReplay::beforePoll(GLFWwindow* window,const GameSession& game,GameScreen screen) {
    auto& s=*state;State::require(++s.tick<100000,"native control replay exceeded frame limit");
    std::set<int> wanted;
    if(s.boot<8) {
        if(s.boot==0)wanted.insert(GLFW_KEY_ENTER);
        if(s.boot==2||s.boot==5)wanted.insert(GLFW_KEY_ESCAPE);
        if(s.boot==3){wanted.insert(GLFW_KEY_W);s.pausePosition=game.player.position();s.countdownPauseTime=game.phaseTime;}
        if(s.boot==6)wanted.insert(GLFW_KEY_R);
        s.keys(window,wanted);return;
    }
    if(!s.countdownInputDone){
        if(!s.countdownProbeStarted){s.countdownProbeStarted=true;s.countdownPosition=game.player.position();s.countdownAngle=game.mirrors[0].getAngle();}
        if(game.phase==GameSession::Phase::Playing&&s.heldGoFrames>=12){s.countdownInputDone=true;s.keys(window,{});return;}
        s.keys(window,{GLFW_KEY_W,GLFW_KEY_E});return;
    }
    if(game.victory){s.keys(window,{});return;}
    if(game.phase!=GameSession::Phase::Playing){
        s.keys(window,{});return;
    }
    if(game.levels.index()==3&&!s.timedPauseDone) {
        if(s.timedPauseTick==0){s.timedPauseStarted=true;s.pausedPhases.clear();for(const auto& t:game.traps)s.pausedPhases.push_back(t.cyclePosition());}
        if(s.timedPauseTick==0||s.timedPauseTick==12)wanted.insert(GLFW_KEY_ESCAPE);
        s.keys(window,wanted);return;
    }
    if(s.resetRequested&&!s.resetExercised){s.keys(window,{GLFW_KEY_R});return;}
    State::require(screen==GameScreen::Play,"unexpected screen during native WASD/QE replay");
    if(s.level!=game.levels.index()) {
        s.level=game.levels.index();s.cursor=0;s.walkingExit=false;s.route.clear();s.waypoint=0;
        s.order.clear();for(size_t i=0;i<game.mirrors.size();++i)s.order.push_back(int(i));
        if(game.mirrors.size()==4)s.order={0,2,1,3};
        if(game.mirrors.size()==6)s.order={0,3,1,4,2,5};
        s.sealed.assign(game.rubies.size(),false);s.capture="control-level"+std::to_string(s.level+1)+"-initial";
        std::cout<<"NATIVE ENTER LEVEL "<<s.level+1<<'\n';
    }
    const auto& l=game.levels.current();
    if(s.cursor<int(s.order.size())) {
        const int mirror=s.order[s.cursor];
        if(s.route.empty()) {
            const glm::ivec2 directions[]={{-1,0},{1,0},{0,-1},{0,1}};bool found=false;
            for(auto d:directions)if(l.isWalkable(l.mirrors[mirror].cell+d)&&s.plan(game,l.mirrors[mirror].cell+d)){found=true;break;}
            State::require(found,"no safe native-WASD route to required mirror");
        }
        if(s.walk(game,wanted)) {
            State::require(game.nearbyMirror()==mirror,"native WASD did not reach intended interaction range");
            const float difference=l.mirrors[mirror].solutionDeg-game.mirrors[mirror].getAngle();
            if(std::abs(difference)<=.085f) {
                std::cout<<"NATIVE MIRROR "<<mirror+1<<" reached "<<game.mirrors[mirror].getAngle()<<" degrees via Q/E\n";
                ++s.cursor;s.route.clear();
            }else {
                wanted.insert(difference>0?GLFW_KEY_E:GLFW_KEY_Q);
                if(std::abs(difference)<1.0f)wanted.insert(GLFW_KEY_LEFT_SHIFT);
            }
        }
    }else {
        State::require(game.solved(),"actual Q/E positions failed to seal all required rubies");
        if(game.doorLift>=.99f) {
            if(!s.walkingExit){State::require(s.plan(game,l.doors[0].cell),"no physical route to final exit");s.walkingExit=true;s.capture="control-level"+std::to_string(s.level+1)+"-open";}
            s.walk(game,wanted);
        }
    }
    s.keys(window,wanted);
}
void ControlReplay::afterInput(GLFWwindow* window,const GameSession& game,GameScreen screen) {
    auto& s=*state;
    for(int key:s.held)State::require(glfwGetKey(window,key)==GLFW_PRESS,"GLFW did not receive a native replay key");
    if(s.held.count(GLFW_KEY_LEFT_SHIFT))++s.fineFrames;
    if(s.boot<8) {
        if(s.boot==0)State::require(screen==GameScreen::Play,"native Enter did not start game");
        if(s.boot==2||s.boot==3)State::require(screen==GameScreen::Pause,"native Escape did not pause");
        if(s.boot==3)State::require(game.player.position().x==s.pausePosition.x&&game.player.position().z==s.pausePosition.z,"paused native WASD moved the player");
        if(s.boot==3)State::require(game.phaseTime==s.countdownPauseTime,"native pause advanced countdown");
        if(s.boot==5)State::require(screen==GameScreen::Play,"native Escape did not resume");
        if(s.boot==6)for(const auto& ruby:game.rubies)State::require(!ruby.isActivated(),"native R did not reset chamber");
        ++s.boot;return;
    }
    if(s.countdownProbeStarted&&!s.countdownInputDone){
        const auto p=game.player.position();
        State::require(p.x==s.countdownPosition.x&&p.z==s.countdownPosition.z&&game.mirrors[0].getAngle()==s.countdownAngle,"held countdown input leaked through GO");
        if(game.phase==GameSession::Phase::Playing){++s.heldGoFrames;if(s.heldGoFrames==12)std::cout<<"NATIVE COUNTDOWN HELD INPUT AND PAUSE VERIFIED\n";}
        return;
    }
    if(game.levels.index()==3&&s.timedPauseStarted&&!s.timedPauseDone) {
        if(s.timedPauseTick<12) {
            State::require(screen==GameScreen::Pause,"timed chamber did not pause");
            for(size_t i=0;i<game.traps.size();++i)State::require(game.traps[i].cyclePosition()==s.pausedPhases[i],"pause advanced a timed trap");
        }else {State::require(screen==GameScreen::Play,"timed chamber did not resume");s.timedPauseDone=true;std::cout<<"NATIVE TIMED TRAP PAUSE FREEZE VERIFIED\n";}
        ++s.timedPauseTick;return;
    }
    if(s.resetRequested&&!s.resetExercised) {
        State::require(!game.solved(),"native R failed to close the door after activation");
        for(const auto& ruby:game.rubies)State::require(!ruby.isActivated()&&ruby.sealingProgress()==0,"native R failed to clear a sealed ruby");
        const auto& level=game.levels.current();
        for(size_t i=0;i<game.mirrors.size();++i)State::require(game.mirrors[i].getAngle()==level.mirrors[i].angleDeg,"native R failed to restore mirror starting angles");
        s.resetExercised=true;s.resetRequested=false;s.level=-1;s.route.clear();s.cursor=0;
        std::cout<<"NATIVE R RESET AFTER SEALED RUBY\n";return;
    }
    if(s.level==game.levels.index())for(size_t i=0;i<game.rubies.size();++i)if(game.rubies[i].isActivated()&&!s.sealed[i]) {
        s.sealed[i]=true;s.capture="control-level"+std::to_string(s.level+1)+"-ruby"+std::to_string(i+1);
        std::cout<<"NATIVE RUBY "<<i+1<<" SEALED IN LEVEL "<<s.level+1<<'\n';
        if(s.level==2&&i==0&&!s.resetExercised)s.resetRequested=true;
    }
    if(game.victory) {
        State::require(screen==GameScreen::Victory,"native campaign exit did not reach victory screen");
        State::require(game.levels.index()==4&&game.deaths==0,"five-level native campaign must finish without unavoidable hazard deaths");
        State::require(s.fineFrames>0,"native Shift precision input was not exercised");
        State::require(s.resetExercised,"native reset after ruby activation was not exercised");
        std::cout<<"NATIVE SHIFT PRECISION FRAMES "<<s.fineFrames<<'\n';
        s.done=true;s.capture="control-victory";
    }
}
bool ControlReplay::shouldRender()const{return state->tick%30==0||!state->capture.empty()||state->done;}
bool ControlReplay::finished()const{return state->done;}
std::string ControlReplay::takeCapture(){auto out=std::move(state->capture);state->capture.clear();return out;}
