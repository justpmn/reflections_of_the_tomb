#pragma once

#include <string>
#include <vector>
#include <glad/gl.h>

class Model {
public:
    Model() = default;
    ~Model();

    // Load a 3D model from a GLB or GLTF file
    bool load(const std::string& filePath);

    // Draw the loaded model
    void draw() const;

    // Release OpenGL resources
    void cleanup();

    // Prevent accidental copying of OpenGL resources
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

private:
    struct Mesh {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;
        unsigned int indexCount = 0;
    };

    std::vector<Mesh> meshes;
    bool loaded = false;
};