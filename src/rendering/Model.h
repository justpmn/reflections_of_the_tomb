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

    bool load(const std::string& filePath);

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
    glm::vec3 minBounds{1e20f};
    glm::vec3 maxBounds{-1e20f};

private:
    struct Mesh
    {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;
        unsigned int textureID = 0;
        unsigned int indexCount = 0;
        bool hasTexture = false;
    };

    std::vector<Mesh> meshes;
    bool loaded = false;
};
