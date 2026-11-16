#include "GameUI.h"
#include "third_party/stb_image.h"
#include <fstream>
#include <vector>
#include <cctype>
#include <algorithm>
bool GameUI::load(Font& font,const std::string& name) {
    int width,height,channels;
    auto* pixels=stbi_load(("assets/ui/"+name+".png").c_str(),&width,&height,&channels,4);
    if(!pixels) return false;
    glGenTextures(1,&font.texture);glBindTexture(GL_TEXTURE_2D,font.texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);
    std::ifstream in("assets/ui/"+name+".txt");int code;Glyph glyph;
    while(in>>code>>glyph.x>>glyph.y>>glyph.advance) font.glyphs[code]=glyph;
    return font.glyphs.size()==95;
}
GameUI::GameUI(Renderer& renderer):r(renderer) {
    loaded=shader.loadFromFiles("shaders/ui_text.vert","shaders/ui_text.frag")&&load(sans,"sans")&&load(roman,"roman");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),reinterpret_cast<void*>(2*sizeof(float)));glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
GameUI::~GameUI() {
    if(vao)glDeleteVertexArrays(1,&vao);if(vbo)glDeleteBuffers(1,&vbo);
    if(sans.texture)glDeleteTextures(1,&sans.texture);if(roman.texture)glDeleteTextures(1,&roman.texture);
}
void GameUI::panel(float x,float y,float w,float h) {
    r.drawRectangle(x,y,w,h,.035f,.047f,.066f,.95f);
}
void GameUI::text(const std::string& value,float x,float y,float size,float red,float green,float blue) {
    if(!loaded) return;
    const Font& font=size>=2.4f?roman:sans;
    const float scale=16.f*size/46.f;
    const float sx=scale*2.f/screenWidth,sy=scale*2.f/screenHeight;
    std::vector<float> vertices;
    for(unsigned char ch:value) {
        const int c=std::toupper(ch);const auto it=font.glyphs.find(c);if(it==font.glyphs.end()) continue;
        const auto& g=it->second;
        const float right=x+64*sx,bottom=y-80*sy;
        const float u=g.x/1024,v=g.y/480,ur=(g.x+64)/1024,vb=(g.y+80)/480;
        vertices.insert(vertices.end(),{x,y,u,v,x,bottom,u,vb,right,bottom,ur,vb,x,y,u,v,right,bottom,ur,vb,right,y,ur,v});
        x+=g.advance*sx;
    }
    shader.use();glUniform3f(glGetUniformLocation(shader.ID,"uColor"),red,green,blue);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,font.texture);
    glUniform1i(glGetUniformLocation(shader.ID,"uAtlas"),0);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,static_cast<int>(vertices.size()/4));glBindVertexArray(0);glDisable(GL_BLEND);
}
void GameUI::centeredText(const std::string& value,float y,float size,float red,float green,float blue) {
    const auto& font=size>=2.4f?roman:sans;
    float advance=0;
    for(unsigned char ch:value){const auto it=font.glyphs.find(std::toupper(ch));if(it!=font.glyphs.end())advance+=it->second.advance;}
    const float width=advance*(16.f*size/46.f)*2.f/screenWidth;
    text(value,-width*.5f,y,size,red,green,blue);
}
bool GameUI::button(const std::string& label,float y,double mx,double my,bool click) {
    const bool hover=mx>-.85&&mx<-.07&&my>y-.045&&my<y+.065;
    r.drawRectangle(-.85f,y-.045f,.78f,.11f,hover?.25f:.09f,hover?.21f:.12f,hover?.14f:.16f);
    r.drawRectangle(-.85f,y-.045f,.004f,.11f,.75f,.58f,.25f);
    text(label,-.81f,y+.042f,1.25f,hover?1.f:.84f,hover?.90f:.84f,hover?.68f:.82f);
    return hover&&click;
}
