#version 330 core
in vec2 UV;
out vec4 FragColor;
void main() {
    vec3 upper=vec3(.026,.035,.086);
    vec3 lower=vec3(.105,.127,.205);
    vec3 color=mix(lower,upper,smoothstep(.0,1.0,UV.y));
    float haze=exp(-length((UV-vec2(.58,.43))*vec2(1.2,1.8))*3.0);
    color+=vec3(.014,.018,.037)*haze;
    // Subpixel dither avoids color bands in the quiet indigo gradient.
    float grain=fract(sin(dot(gl_FragCoord.xy,vec2(12.9898,78.233)))*43758.5453);
    FragColor=vec4(color+(grain-.5)/510.0,1);
}
