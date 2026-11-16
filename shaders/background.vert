#version 330 core
out vec2 UV;
void main() {
    vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);
    UV=p*.5+.5;
    gl_Position=vec4(p,0,1);
}
