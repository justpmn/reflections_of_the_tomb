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
int play(GLFWwindow* window,bool smoke,int preview,const std::string& screenshot) {
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
    Audio audio; audio.music();
    GameUI ui(renderer);
    Screen screen=smoke?Screen::Play:Screen::Start, back=Screen::Start;
    std::array<bool,GLFW_KEY_LAST+1> previous{};
    bool previousMouse=false;
    double last=glfwGetTime(),lastSound=0;
    int frames=0;
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now=glfwGetTime(); const float dt=std::min(.05f,float(now-last)); last=now;
        auto down=[&](int key){return glfwGetKey(window,key)==GLFW_PRESS;};
        auto pressed=[&](int key){return down(key)&&!previous[key];};
        int width,height,ww,wh; glfwGetFramebufferSize(window,&width,&height); glfwGetWindowSize(window,&ww,&wh);
        if(width==0 || height==0) { glfwWaitEventsTimeout(.05); last=glfwGetTime(); continue; }
        glViewport(0,0,width,height);
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
            if(!wasSolved && game.solved()) audio.effect("Ruby and Pedestal Sounds/permanent_ruby_sealing.wav");
            if(deaths!=game.deaths) audio.effect("Player/respawn.wav");
            if(level!=game.levels.index()) audio.effect("Menu and interface sounds/confirmation_or_start game.wav");
            if(game.victory) screen=Screen::Victory;
        }
        const auto& l=game.levels.current();
        const glm::vec3 center(l.width*l.cellSize*.5f,0,l.height*l.cellSize*.47f);
        const float span=std::max(l.width*l.cellSize,l.height*l.cellSize*1.35f);
        renderer.setCamera(center+glm::vec3(0,span*.90f,span*.74f),center,float(width)/height);
        renderer.beginFrame(); scene.draw(renderer,game);
        glDisable(GL_DEPTH_TEST);
        if(screen==Screen::Play) {
            ui.panel(-1,.77f,2,.23f);
            ui.text("REFLECTIONS OF THE TOMB",-.95f,.94f,1.6f);
            ui.text("LEVEL "+std::to_string(game.levels.index()+1)+" / 3  -  "+l.name,-.95f,.85f,1.2f);
            ui.panel(-1,-1,2,.20f);
            if(game.solved()) ui.text("RUBIES SEALED - WALK THROUGH THE OPEN DOOR",-.95f,-.85f,1.35f,.4f,1,.6f);
            else if(game.nearbyMirror()>=0) ui.text("Q / E ROTATE MIRROR   SHIFT FOR PRECISION",-.95f,-.85f,1.3f);
            else ui.text("MOVE NEAR A MIRROR TO ROTATE IT - LIGHT EVERY RUBY",-.95f,-.85f,1.2f);
            ui.text("WASD MOVE   R RESET   ESC PAUSE   M SOUND",-.95f,-.94f,1.1f);
        } else {
            ui.panel(-.79f,-.77f,1.58f,1.54f);
            ui.text("REFLECTIONS",-.57f,.65f,3.1f);
            ui.text("OF THE TOMB",-.53f,.50f,2.5f);
            auto button=[&](const std::string& label,float y) {
                const bool result=ui.button(label,y,mx,my,click);
                if(result) audio.effect("Menu and interface sounds/menu_selection.wav");
                return result;
            };
            if(screen==Screen::Start) {
                ui.text("AWAKEN THE LIGHT. UNSEAL THE TEMPLE.",-.60f,.32f,1.35f);
                if(button("START GAME  /  ENTER",.10f)||pressed(GLFW_KEY_ENTER)) {
                    game.restart();screen=Screen::Play;
                    audio.effect("Menu and interface sounds/confirmation_or_start game.wav");
                }
                if(button("CONTROLS",-.08f)) {back=screen;screen=Screen::Controls;}
                if(button("QUIT",-.26f)) glfwSetWindowShouldClose(window,1);
                ui.text("THREE CHAMBERS. ONE ANCIENT SECRET.",-.58f,-.57f,1.3f);
            } else if(screen==Screen::Pause) {
                ui.text("PAUSED",-.18f,.30f,2);
                if(button("RESUME  /  ESC",.10f)) screen=Screen::Play;
                if(button("RESET THIS CHAMBER",-.06f)) {game.reset();screen=Screen::Play;}
                if(button("CONTROLS",-.22f)) {back=screen;screen=Screen::Controls;}
                if(button("RESTART CAMPAIGN",-.38f)) {game.restart();screen=Screen::Play;}
                if(button("MAIN MENU",-.54f)) screen=Screen::Start;
            } else if(screen==Screen::Controls) {
                ui.text("WASD - MOVE THE RANGER",-.62f,.29f,1.45f);
                ui.text("Q / E - ROTATE A NEARBY MIRROR",-.62f,.16f,1.45f);
                ui.text("SHIFT - PRECISE MIRROR ROTATION",-.62f,.03f,1.45f);
                ui.text("R - RESET CHAMBER   M - MUTE",-.62f,-.10f,1.45f);
                ui.text("ESC - PAUSE / RESUME",-.62f,-.23f,1.45f);
                ui.text("LIGHT RUBIES. AVOID SPIKES AND PITS.",-.62f,-.36f,1.35f);
                ui.text("SEALED RUBIES STAY LIT UNTIL RESET.",-.62f,-.46f,1.35f);
                if(button("BACK",-.63f)) screen=back;
            } else if(screen==Screen::Victory) {
                ui.text("THE TEMPLE IS AWAKENED",-.62f,.27f,1.9f,.5f,1,.7f);
                ui.text("ALL THREE CHAMBERS COMPLETE",-.60f,.10f,1.6f);
                ui.text("RESPAWNS: "+std::to_string(game.deaths),-.35f,-.07f,1.5f);
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
        bool smoke=false;int preview=0;std::string screenshot;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--smoke") smoke=true;
            else if(arg=="--level" && i+1<argc) preview=std::stoi(argv[++i])-1;
            else if(arg=="--capture" && i+1<argc) screenshot=argv[++i];
        }
        if(!glfwInit()) {std::cerr<<"GLFW initialization failed\n";return 1;}
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        if(smoke) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
        auto* window=glfwCreateWindow(1280,800,"Reflections of the Tomb",nullptr,nullptr);
        if(!window) {glfwTerminate();return 1;}
        glfwMakeContextCurrent(window);glfwSwapInterval(1);
        if(!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {glfwDestroyWindow(window);glfwTerminate();return 1;}
        // All GL owners are destroyed inside play(), while the context is still alive.
        const int result=play(window,smoke,preview,screenshot);
        glfwDestroyWindow(window);glfwTerminate();return result;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 10;}
}
