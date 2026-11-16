#version 330 core
in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPos;
in vec4 VertexColor;
out vec4 FragColor;
uniform bool uHasVertexColor;
uniform float uShadowRadius;
uniform vec2 uShadowCenter;
uniform float uClipFloor;
uniform vec3 uColor;
uniform vec4 uMaterialColor;
uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform vec3 uMaterialEmissive;
uniform float uRoughness;
uniform float uMetallic;
uniform float uGlow;
uniform int uAlphaMode;
uniform float uAlphaCutoff;
uniform vec3 uLightDirection;
uniform vec3 uViewPos;
uniform int uPointCount;
uniform vec3 uPointPosition[8];
uniform vec3 uPointColor[8];
void main() {
    if(FragPos.y<uClipFloor) discard;
    if(uShadowRadius>0) {
        float d=length(FragPos.xz-uShadowCenter)/uShadowRadius;
        FragColor=vec4(.015,.022,.035,(1.0-smoothstep(.0,1.0,d))*.28);return;
    }
    vec4 base=vec4(uColor,1)*uMaterialColor;
    if(uHasVertexColor) base*=VertexColor;
    if(uHasTexture) base*=texture(uTexture,TexCoords);
    if(uAlphaMode==1 && base.a<uAlphaCutoff) discard;
    vec3 n=normalize(Normal);
    if(!gl_FrontFacing) n=-n;
    vec3 v=normalize(uViewPos-FragPos);
    vec3 key=normalize(uLightDirection);
    float rough=clamp(uRoughness,.08,1.0);
    vec3 f0=mix(vec3(.04),base.rgb,uMetallic);
    float spec=pow(max(dot(n,normalize(key+v)),0),mix(160.0,8.0,rough));
    float hemi=n.y*.5+.5;
    vec3 ambient=mix(vec3(.10,.13,.19),vec3(.30,.34,.40),hemi);
    vec3 light=base.rgb*(ambient+vec3(.93,.81,.64)*max(dot(n,key),0)*.9);
    light+=f0*spec*(1.0-rough)*2.0;
    // A cool fill keeps the backs of imported models readable.
    light+=base.rgb*vec3(.12,.19,.28)*max(dot(n,normalize(vec3(-.6,.5,-.7))),0);
    for(int i=0;i<uPointCount;++i) {
        vec3 d=uPointPosition[i]-FragPos;
        float attenuation=1.0/(1.0+dot(d,d)*.65);
        light+=base.rgb*uPointColor[i]*max(dot(n,normalize(d)),.12)*attenuation;
    }
    light+=uMaterialEmissive+base.rgb*uGlow;
    // Linear light and sRGB textures, followed by a restrained display transform.
    light=light/(vec3(1)+light*.35);
    vec3 display=pow(max(light,vec3(0)),vec3(1.0/2.2));
    float atmosphere=clamp(1.0-exp(-length(uViewPos-FragPos)*.0025),0.0,.18);
    display=mix(display,vec3(.12,.16,.25),atmosphere);
    FragColor=vec4(display,uAlphaMode==2?base.a:1.0);
}
