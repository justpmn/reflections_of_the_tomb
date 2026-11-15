#pragma once

#include "Shader.h"
#include "Model.h"

#include <glm/glm.hpp>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Initialize the renderer and its OpenGL resources.
    bool initialize();

    // Clear the screen for a new frame.
    void beginFrame();

    // Draw a colored 2D rectangle.
    void drawRectangle(
        float x,
        float y,
        float width,
        float height,
        float red,
        float green,
        float blue
    );

    // Draw a 3D model.
    void drawModel(
        const Model& model,
        const glm::mat4& modelMatrix,
        const glm::vec3& color
    );

    // Release OpenGL resources.
    void shutdown();

private:
    Shader shader;
    Shader modelShader;

    unsigned int VAO = 0;
    unsigned int VBO = 0;

    bool initialized = false;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};
