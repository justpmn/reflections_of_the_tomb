
#pragma once

#include "Shader.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Initialize the renderer and its OpenGL resources.
    bool initialize();

    // Clear the screen for a new frame.
    void beginFrame();

    // Draw a colored rectangle using normalized coordinates.
    void drawRectangle(
        float x,
        float y,
        float width,
        float height,
        float red,
        float green,
        float blue
    );

    // Release OpenGL resources.
    void shutdown();

private:
    Shader shader;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    bool initialized = false;
};
