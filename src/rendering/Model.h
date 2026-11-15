
#pragma once

#include <string>
#include <vector>
#include <glad/gl.h>

class Model
{
public:
    Model() = default;
    ~Model();

    bool load(const std::string& filePath);

    void draw(unsigned int shaderProgram) const;

    void cleanup();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

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
