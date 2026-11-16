
#version 330 core

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform sampler2D uTexture;
uniform bool uHasTexture;

uniform vec3 uLightDirection;
uniform vec3 uViewPos;

uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform float uSpecularStrength;
uniform float uShininess;

void main()
{
    // Get the base colour from the texture or uniform.
    vec3 baseColor = uColor;

    if (uHasTexture)
    {
        baseColor = texture(uTexture, TexCoords).rgb;
    }

    // Normalize the surface normal.
    vec3 normal = normalize(Normal);

    // Direction towards the light.
    vec3 lightDir = normalize(uLightDirection);

    // Ambient lighting.
    vec3 ambient = uAmbientStrength * baseColor;

    // Diffuse lighting.
    float diffuseFactor = max(
        dot(normal, lightDir),
        0.0
    );

    vec3 diffuse =
        uDiffuseStrength * diffuseFactor * baseColor;

    // Blinn-Phong specular lighting.
    vec3 viewDir = normalize(uViewPos - FragPos);

    vec3 halfwayDir = normalize(lightDir + viewDir);

    float specularFactor = pow(
        max(dot(normal, halfwayDir), 0.0),
        uShininess
    );

    vec3 specular =
        uSpecularStrength * specularFactor * vec3(1.0);

    // Combine lighting components.
    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}
