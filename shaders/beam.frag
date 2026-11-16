
#version 330 core

out vec4 FragColor;

uniform vec3 uBeamColor;
uniform float uIntensity;

void main()
{
    vec3 glowingColor = uBeamColor * uIntensity;

    FragColor = vec4(glowingColor, 1.0);
}
