#version 330 core
in vec2 UV;
out vec4 FragColor;
uniform sampler2D uAtlas;
uniform vec3 uColor;
void main(){FragColor=vec4(uColor,texture(uAtlas,UV).a);}
