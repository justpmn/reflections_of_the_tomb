#include "GameApplication.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "gameplay/GameSession.h"
#include "rendering/TempleScene.h"
#include "rendering/GameUI.h"
#include "Audio.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace {
enum class Screen { Start, Play, Pause, Controls, Victory };
void locateAssets(const char* executable) {
    namespace fs=std::filesystem;
    if(fs::exists("assets/levels/level01.txt") && fs::exists("shaders/model.vert")) return;
    auto p=fs::absolute(executable).parent_path();
    for(int i=0;i<5;++i,p=p.parent_path()) if(fs::exists(p/"assets/levels/level01.txt")) {
        fs::current_path(p); return;
    }
#ifdef GAME_SOURCE_DIR
    if(fs::exists(fs::path(GAME_SOURCE_DIR)/"assets/levels/level01.txt")) fs::current_path(GAME_SOURCE_DIR);
#endif
}
void capture(int width,int height,const std::string& file) {
    std::vector<unsigned char> pixels(width*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream out(file,std::ios::binary); out<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y) out.write(reinterpret_cast<char*>(pixels.data()+y*width*3),width*3);
}
int play(GLFWwindow* window,bool smoke,int preview,const std::string& screenshot,Screen initialScreen,bool solvedPreview,const std::string& view) {
    Renderer renderer;
    if(!renderer.initialize()) return 2;
    TempleScene scene;
    if(!scene.load()) { std::cerr<<"Required scene asset failed to load\n"; return 3; }
    GameSession game("assets/levels");
    if(!game.restart()) {
        for(const auto& e:game.levels.errors()) std::cerr<<e<<'\n';
        return 4;
    }
    if(preview>0) { if(!game.levels.loadIndex(preview) || !game.reset()) return 4; }
    if(smoke&&solvedPreview) {
        const auto& level=game.levels.current();
        for(size_t i=0;i<game.mirrors.size();++i) game.mirrors[i].setAngle(level.mirrors[i].solutionDeg);
        game.trace();
        for(size_t i=0;i<game.mirrors.size();++i) if(level.mirrors[i].hasAlternateSolution) {
            game.mirrors[i].setAngle(level.mirrors[i].alternateSolutionDeg);game.trace();
        }
        for(int i=0;i<25;++i) game.update(0,0,.05f);
        if(!game.solved()||game.doorLift<.95f) return 6;
    }
    Audio audio; audio.music();
    GameUI ui(renderer);
    if(!ui.ready()) {std::cerr<<"UI atlas/shader load failed\n";return 7;}
    Screen screen=initialScreen, back=Screen::Start;
    std::array<bool,GLFW_KEY_LAST+1> previous{};
    bool previousMouse=false;
    double last=glfwGetTime(),lastSound=0,lastStep=0;
    int frames=0;
    float animationTime=0;
    float cameraZoom=1.f;
    glfwSetWindowUserPointer(window,&cameraZoom);
    glfwSetScrollCallback(window,[](GLFWwindow* w,double,double y) {
        auto* zoom=static_cast<float*>(glfwGetWindowUserPointer(w));
        *zoom=std::clamp(*zoom*float(std::pow(.9,y)),.55f,1.35f);
    });
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now=glfwGetTime(); const float dt=std::min(.05f,float(now-last)); last=now;
        auto down=[&](int key){return glfwGetKey(window,key)==GLFW_PRESS;};
        auto pressed=[&](int key){return down(key)&&!previous[key];};
        int width,height,ww,wh; glfwGetFramebufferSize(window,&width,&height); glfwGetWindowSize(window,&ww,&wh);
        if(width==0 || height==0) { glfwWaitEventsTimeout(.05); last=glfwGetTime(); continue; }
        glViewport(0,0,width,height);
        ui.viewport(width,height);
        double mx,my; glfwGetCursorPos(window,&mx,&my);
        mx=2*mx/std::max(1,ww)-1; my=1-2*my/std::max(1,wh);
        const bool mouse=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
        const bool click=mouse&&!previousMouse;
        if(pressed(GLFW_KEY_M)) audio.toggle();
        if(pressed(GLFW_KEY_ESCAPE)) {
            if(screen==Screen::Play) screen=Screen::Pause;
            else if(screen==Screen::Pause) screen=Screen::Play;
            else if(screen==Screen::Controls) screen=back;
            audio.effect("Menu and interface sounds/pause_or_resume.wav");
        }
        if(screen==Screen::Play) {
            if(pressed(GLFW_KEY_R)) { game.reset(); audio.effect("Player/respawn.wav"); }
            const bool wasSolved=game.solved(); const int deaths=game.deaths,level=game.levels.index();
            if(down(GLFW_KEY_Q) || down(GLFW_KEY_E)) {
                const float direction=float(down(GLFW_KEY_E))-float(down(GLFW_KEY_Q));
                const float speed=down(GLFW_KEY_LEFT_SHIFT)?10.f:45.f;
                if(game.rotate(direction*speed*dt) && now-lastSound>.4) {
                    audio.effect("Mirror/rotate_mirror.wav"); lastSound=now;
                }
            }
            game.update(float(down(GLFW_KEY_D))-float(down(GLFW_KEY_A)),
                        float(down(GLFW_KEY_S))-float(down(GLFW_KEY_W)),dt);
            if(!wasSolved && game.solved()) {
                audio.effect("Ruby and Pedestal Sounds/permanent_ruby_sealing.wav");
                audio.effect("Temple doors and traps/stone_door_opening_and_closing.ogg");
            }
            if(game.player.isMoving() && now-lastStep>.45) {
                audio.effect("Player/stone_footsteps.wav");lastStep=now;
            }
            if(deaths!=game.deaths) audio.effect("Player/respawn.wav");
            if(level!=game.levels.index()) audio.effect("Menu and interface sounds/confirmation_or_start game.wav");
            if(game.victory) screen=Screen::Victory;
        }
        const auto& l=game.levels.current();
        glm::vec3 center(l.width*l.cellSize*.5f,-.35f,l.height*l.cellSize*.51f);
        const float span=std::max(l.width*l.cellSize,l.height*l.cellSize*1.25f);
        float framing=std::max(1.f,(1280.f/720.f)/(float(width)/height));
        if(screen!=Screen::Play) center.x-=l.width*l.cellSize*.15f;
        auto eye=center+glm::vec3(span*.06f,span*.68f,span*.82f)*framing;
        if(smoke&&view=="ruby") {center=l.cellCenter(l.rubies[0].cell,1.f);eye=center+glm::vec3(2.5f,2.6f,4.f);}
        if(smoke&&view=="door") {center=l.cellCenter(l.doors[0].cell,1.8f);eye=center+glm::vec3(4.f,3.f,7.f);}
        if(view.empty()) {
            // Fit the projected temple (including the door arch) within the area
            // between the HUD bands, for the current framebuffer aspect ratio.
            const auto direction=glm::normalize(eye-center);
            auto fits=[&](float distance) {
                eye=center+direction*distance;
                const auto vp=glm::perspective(glm::radians(45.f),float(width)/height,.1f,180.f)*glm::lookAt(eye,center,glm::vec3(0,1,0));
                float ratio=1;
                for(float x:{l.cellSize*.5f,(l.width-.5f)*l.cellSize})
                    for(float z:{l.cellSize*.5f,(l.height-.5f)*l.cellSize}) for(float y:{0.f,l.wallHeight}) {
                        if(z>(l.height-1)*l.cellSize && y>0) continue;
                        const auto clip=vp*glm::vec4(x,y,z,1);if(clip.w<=.1f)return false;const auto ndc=glm::vec2(clip)/clip.w;
                        ratio=std::max(ratio,std::max(std::abs(ndc.x)/.94f,std::abs(ndc.y)/.70f));
                    }
                for(const auto& door:l.doors) for(float x:{-1.65f,1.65f}) for(float y:{0.f,3.55f}) {
                    const auto p=l.cellCenter(door.cell)+glm::vec3(x,y,0);
                    const auto clip=vp*glm::vec4(p,1);const auto ndc=glm::vec2(clip)/clip.w;
                    ratio=std::max(ratio,std::max(std::abs(ndc.x)/.94f,std::abs(ndc.y)/.75f));
                }
                return ratio<=1.002f;
            };
            float near=4.f,far=120.f;
            for(int iteration=0;iteration<20;++iteration) {
                const float distance=(near+far)*.5f;
                if(fits(distance))far=distance;else near=distance;
            }
            eye=center+direction*far*cameraZoom;
        }
        renderer.setCamera(eye,center,float(width)/height);
        if(screen==Screen::Play) animationTime+=dt;
        renderer.beginFrame(); scene.draw(renderer,game,smoke?frames*.12f:animationTime,smoke&&frames>=4);
        glDisable(GL_DEPTH_TEST);
        if(screen==Screen::Play) {
            ui.panel(-1,.80f,2,.20f);
            ui.text("REFLECTIONS OF THE TOMB",-.95f,.96f,1.4f);
            ui.text("LEVEL "+std::to_string(game.levels.index()+1)+" / 3  -  "+l.name+"   RUBIES "+std::to_string(std::count_if(game.rubies.begin(),game.rubies.end(),[](const Ruby& r){return r.isActivated();}))+" / "+std::to_string(game.rubies.size()),-.95f,.87f,1.f);
            ui.panel(-1,-1,2,.20f);
            if(game.solved()) ui.text("RUBIES SEALED - WALK THROUGH THE OPEN DOOR",-.95f,-.81f,1.2f,.4f,1,.6f);
            else if(game.nearbyMirror()>=0) {
                const int selected=game.nearbyMirror();std::ostringstream hint;
                hint<<"MIRROR "<<selected+1<<"   "<<std::fixed<<std::setprecision(1)<<game.mirrors[selected].getAngle()<<" DEG   Q / E ROTATE   SHIFT FINE";
                ui.text(hint.str(),-.95f,-.81f,1.15f);
            }
            else ui.text("MOVE NEAR A MIRROR TO ROTATE IT - LIGHT EVERY RUBY",-.95f,-.81f,1.1f);
            ui.text("WASD MOVE   Q / E ROTATE   R RESET   ESC PAUSE   M SOUND",-.95f,-.91f,1.f);
        } else {
            renderer.drawRectangle(-1,-1,2,2,.015f,.025f,.045f,.22f);
            ui.panel(-.94f,-.86f,.92f,1.72f);
            renderer.drawRectangle(-.94f,-.86f,.006f,1.72f,.68f,.52f,.26f);
            ui.text("REFLECTIONS",-.85f,.72f,2.8f);
            ui.text("OF THE TOMB",-.85f,.58f,2.5f);
            auto button=[&](const std::string& label,float y) {
                const bool result=ui.button(label,y,mx,my,click);
                if(result) audio.effect("Menu and interface sounds/menu_selection.wav");
                return result;
            };
            if(screen==Screen::Start) {
                ui.text("SELECT AN OPTION",-.85f,.31f,1.f,.55f,.64f,.73f);
                if(button("START GAME  /  ENTER",.10f)||pressed(GLFW_KEY_ENTER)) {
                    game.restart();screen=Screen::Play;
                    audio.effect("Menu and interface sounds/confirmation_or_start game.wav");
                }
                if(button("CONTROLS",-.08f)) {back=screen;screen=Screen::Controls;}
                if(button("QUIT",-.26f)) glfwSetWindowShouldClose(window,1);
                ui.text("WASD MOVE   Q / E ROTATE",-.85f,-.63f,1.f,.59f,.66f,.73f);
            } else if(screen==Screen::Pause) {
                ui.text("PAUSED",-.85f,.33f,1.6f);
                if(button("RESUME  /  ESC",.10f)) screen=Screen::Play;
                if(button("RESET THIS CHAMBER",-.06f)) {game.reset();screen=Screen::Play;}
                if(button("CONTROLS",-.22f)) {back=screen;screen=Screen::Controls;}
                if(button("RESTART CAMPAIGN",-.38f)) {game.restart();screen=Screen::Play;}
                if(button("MAIN MENU",-.54f)) screen=Screen::Start;
            } else if(screen==Screen::Controls) {
                ui.text("WASD - MOVE RANGER",-.85f,.30f,1.05f);
                ui.text("Q / E - ROTATE NEARBY MIRROR",-.85f,.19f,1.05f);
                ui.text("SHIFT - FINE ROTATION",-.85f,.08f,1.05f);
                ui.text("MOUSE WHEEL - ZOOM",-.85f,-.03f,1.05f);
                ui.text("R - RESET THIS CHAMBER",-.85f,-.14f,1.05f);
                ui.text("ESC - PAUSE   M - SOUND",-.85f,-.25f,1.05f);
                ui.text("LIGHT RUBIES. AVOID TRAPS AND PITS.",-.85f,-.40f,.96f);
                ui.text("RUBIES STAY SEALED UNTIL RESET.",-.85f,-.50f,.96f);
                if(button("BACK",-.69f)) screen=back;
            } else if(screen==Screen::Victory) {
                ui.text("CAMPAIGN COMPLETE",-.85f,.30f,1.45f,.5f,1,.7f);
                ui.text("THREE CHAMBERS SOLVED",-.85f,.13f,1.1f);
                ui.text("RESPAWNS: "+std::to_string(game.deaths),-.85f,-.04f,1.1f);
                if(button("PLAY AGAIN",-.28f)) {game.restart();screen=Screen::Play;}
                if(button("MAIN MENU",-.46f)) screen=Screen::Start;
            }
        }
        if(smoke && ++frames>=8) {
            if(!screenshot.empty()) capture(width,height,screenshot);
            const auto error=glGetError();
            std::cout<<"SMOKE level="<<game.levels.index()+1<<" GL="<<error<<'\n';
            return error==GL_NO_ERROR?0:5;
        }
        glfwSwapBuffers(window);
        for(int i=0;i<=GLFW_KEY_LAST;++i) previous[i]=down(i);
        previousMouse=mouse;
    }
    return 0;
}
}
int runGame(int argc,char** argv) {
    try {
        locateAssets(argv[0]);
        bool smoke=false,solvedPreview=false;int preview=0;std::string screenshot,screenName,view;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--smoke") smoke=true;
            else if(arg=="--solved") solvedPreview=true;
            else if(arg=="--view" && i+1<argc) view=argv[++i];
            else if(arg=="--level" && i+1<argc) preview=std::stoi(argv[++i])-1;
            else if(arg=="--capture" && i+1<argc) screenshot=argv[++i];
            else if(arg=="--screen" && i+1<argc) screenName=argv[++i];
        }
        if(!glfwInit()) {std::cerr<<"GLFW initialization failed\n";return 1;}
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        if(smoke) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
        auto* window=glfwCreateWindow(1280,720,"Reflections of the Tomb",nullptr,nullptr);
        if(!window) {glfwTerminate();return 1;}
        glfwMakeContextCurrent(window);glfwSwapInterval(1);
        if(!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {glfwDestroyWindow(window);glfwTerminate();return 1;}
        // All GL owners are destroyed inside play(), while the context is still alive.
        Screen initial=smoke?Screen::Play:Screen::Start;
        if(screenName=="start") initial=Screen::Start;
        else if(screenName=="pause") initial=Screen::Pause;
        else if(screenName=="controls") initial=Screen::Controls;
        else if(screenName=="victory") initial=Screen::Victory;
        const int result=play(window,smoke,preview,screenshot,initial,solvedPreview,view);
        glfwDestroyWindow(window);glfwTerminate();return result;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 10;}
}
