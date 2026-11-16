#pragma once
#include "Renderer.h"
#include <map>
#include <string>
class GameUI {
public:
    explicit GameUI(Renderer& renderer);
    ~GameUI();
    bool ready() const {return loaded;}
    void viewport(int width,int height) {screenWidth=width;screenHeight=height;}
    void panel(float x,float y,float w,float h);
    void text(const std::string& value,float x,float y,float size=1,float red=.9f,float green=.84f,float blue=.66f);
    bool button(const std::string& label,float y,double mx,double my,bool click);
private:
    struct Glyph {float x,y,advance;};
    struct Font {unsigned int texture=0;std::map<int,Glyph> glyphs;};
    Renderer& r;
    Shader shader;
    Font sans,roman;
    unsigned int vao=0,vbo=0;
    int screenWidth=1280,screenHeight=720;
    bool loaded=false;
    bool load(Font& font,const std::string& name);
};
