
#version 330 core

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform sampler2D uTexture;
uniform bool uHasTexture;

void main()
{
    vec3 baseColor = uColor;

    if (uHasTexture)
    {
        baseColor = texture(uTexture, TexCoords).rgb;
    }

    vec3 lightDirection = normalize(
        vec3(-0.5, 1.0, 0.5)
    );

    float diffuse = max(
        dot(normalize(Normal), lightDirection),
        0.25
    );

    vec3 ambient = baseColor * 0.35;
    vec3 lighting = baseColor * diffuse * 0.65;

    FragColor = vec4(ambient + lighting, 1.0);
}
