#pragma once

#include <string>
#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

class Model
{
public:
    Model() = default;
    ~Model();

    bool load(const std::string& filePath, bool retainSkin = false);
    bool loadAnimations(const std::string& idleFile, const std::string& walkFile);
    bool animate(float seconds, bool walking);

    // Draw every mesh in the model.
    void draw(unsigned int shaderProgram) const;

    // Draw only one mesh by its index.
    void drawMesh(
        unsigned int meshIndex,
        unsigned int shaderProgram
    ) const;

    void cleanup();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Number of meshes loaded.
    unsigned int getMeshCount() const;
    glm::vec3 meshCenter(unsigned int index) const;
    glm::vec3 meshSize(unsigned int index) const;
    const std::string& meshName(unsigned int index) const;
    int findMesh(const std::string& name) const;
    glm::vec3 meshNormal(unsigned int index) const;
    glm::vec3 minBounds{1e20f};
    glm::vec3 maxBounds{-1e20f};

private:
    struct AnimationState;
    AnimationState* animation = nullptr;
    struct Mesh
    {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int colorVBO = 0;
        unsigned int EBO = 0;
        unsigned int textureID = 0;
        unsigned int indexCount = 0;
        bool hasTexture = false;
        glm::vec4 baseColor{1};
        bool hasVertexColor = false;
        glm::vec3 minBounds{1e20f},maxBounds{-1e20f};
        std::string name;
        glm::vec3 emissive{0};
        float roughness=.7f,metallic=0,alphaCutoff=.5f;
        int alphaMode=0;
        bool doubleSided=false;
        glm::vec3 normal{0,0,1};
    };

    std::vector<Mesh> meshes;
    bool loaded = false;
};
