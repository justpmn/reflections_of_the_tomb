#include "rendering/Model.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
GLuint stage(GLenum type,const char* source) {
    const GLuint id=glCreateShader(type);glShaderSource(id,1,&source,nullptr);glCompileShader(id);
    int success;glGetShaderiv(id,GL_COMPILE_STATUS,&success);check(success,"probe shader compile");return id;
}
int verify() {
    Model model;check(model.load("tests/fixtures/material_scene.gltf"),"load fixture");
    check(model.getMeshCount()==1,"only selected-scene nodes render; off-scene mesh omitted");
    check(std::abs(model.minBounds.x+.4f)<.0001f&&std::abs(model.maxBounds.x-.6f)<.0001f,"parent and child node transforms compose correctly");
    const char* vertex=R"(#version 330 core
        layout(location=0)in vec3 p;layout(location=1)in vec3 n;layout(location=2)in vec2 uv;layout(location=3)in vec4 color;
        out vec3 N;out vec2 UV;out vec4 C;
        void main(){N=n;UV=uv;C=color;gl_Position=vec4(p,1);})";
    const char* fragment=R"(#version 330 core
        in vec3 N;in vec2 UV;in vec4 C;out vec4 result;
        uniform int probe;uniform sampler2D uTexture;uniform float offsetY;
        uniform vec4 uMaterialColor;uniform int uAlphaMode;uniform float uAlphaCutoff;
        void main(){
            if(probe==0) result=vec4(normalize(N)*.5+.5,1);
            else if(probe==1) result=texture(uTexture,UV+vec2(0,offsetY))*C;
            else if(probe==2) result=uMaterialColor;
            else {if(uAlphaMode==1&&uMaterialColor.a<uAlphaCutoff) discard;result=vec4(1);}
        })";
    const auto v=stage(GL_VERTEX_SHADER,vertex),f=stage(GL_FRAGMENT_SHADER,fragment);
    const GLuint program=glCreateProgram();glAttachShader(program,v);glAttachShader(program,f);glLinkProgram(program);
    int linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);check(linked,"probe shader link");
    glDeleteShader(v);glDeleteShader(f);glUseProgram(program);glViewport(0,0,128,128);
    auto sample=[&](int probe,float offset=0,const Model* selected=nullptr) {
        glClearColor(.07f,.09f,.11f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUniform1i(glGetUniformLocation(program,"probe"),probe);glUniform1f(glGetUniformLocation(program,"offsetY"),offset);
        (selected?*selected:model).draw(program);
        std::array<unsigned char,4> pixel{};glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());return pixel;
    };
    auto normal=sample(0);
    check(std::abs(int(normal[0])-128)<=2&&std::abs(int(normal[1])-198)<=2&&std::abs(int(normal[2])-234)<=2,"inverse transpose corrects nonuniform node-scale normals");
    auto texture=sample(1);
    check(texture[0]>52&&texture[0]<58&&texture[1]<2&&texture[2]<2,"COLOR_0 wins over COLOR_1; sRGB decoding and glTF UV orientation are correct");
    auto clamped=sample(1,1);
    check(clamped[0]<2&&clamped[2]>250,"authored clamp sampler is respected");
    auto factor=sample(2);
    check(std::abs(int(factor[0])-128)<=2&&std::abs(int(factor[1])-179)<=2&&std::abs(int(factor[3])-64)<=2,"RGBA material factor preserved");
    auto masked=sample(3);
    check(std::abs(int(masked[0])-18)<=2&&std::abs(int(masked[2])-28)<=2,"alpha mask mode and cutoff preserved");
    Model blended;check(blended.load("tests/fixtures/material_blend.gltf"),"load blend fixture");
    auto blendedPixel=sample(2,0,&blended);
    check(std::abs(int(blendedPixel[0])-45)<=2&&std::abs(int(blendedPixel[1])-62)<=2,"BLEND material uses source alpha over opaque background");
    GLboolean depthWrite;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    check(depthWrite==GL_TRUE&&glIsEnabled(GL_BLEND)==GL_FALSE,"transparent mesh restores depth writes and blending state");
    check(glGetError()==GL_NO_ERROR,"asset contract probes have no OpenGL error");
    glDeleteProgram(program);
    std::cout<<"PASS selected scene, node transform, inverse-transpose normal, COLOR_0, sRGB UVs, sampler, RGBA and alpha-mask GPU probes\n";
    return 0;
}
int main() {
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    auto* window=glfwCreateWindow(128,128,"Asset contract",nullptr,nullptr);if(!window){glfwTerminate();return 1;}
    glfwMakeContextCurrent(window);
    int result=1;
    if(gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        try {result=verify();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';}
    }
    glfwDestroyWindow(window);glfwTerminate();return result;
}
