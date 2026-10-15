
#include "Renderer.h"

#include <iostream>

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    if (initialized) {
        return true;
    }

    // Load and link the vertex and fragment shaders.
    if (!shader.loadFromFiles(
            "shaders/basic.vert",
            "shaders/basic.frag")) {
        std::cerr << "Failed to load rendering shaders.\n";
        return false;
    }

    // Create the OpenGL vertex array and vertex buffer.
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    if (VAO == 0 || VBO == 0) {
        std::cerr << "Failed to create rendering buffers.\n";
        shutdown();
        return false;
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Reserve space for six vertices each with three coordinates.
    glBufferData(
        GL_ARRAY_BUFFER,
        18 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    // Tell OpenGL how to read each vertex position.
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), nullptr
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized = true;
    std::cout << "Renderer initialized successfully.\n";

    return true;
}

void Renderer::beginFrame() {
    
    glClearColor(0.05f, 0.04f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawRectangle(
    float x,
    float y,
    float width,
    float height,
    float red,
    float green,
    float blue
) {
    if (!initialized) {
        return;
    }

    // Six vertices form two triangles, creating a rectangle.
    const float vertices[] = {
        x,          y,           0.0f,
        x + width,  y,           0.0f,
        x + width,  y + height,  0.0f,

        x,          y,           0.0f,
        x + width,  y + height,  0.0f,
        x,          y + height,  0.0f
    };

    shader.use();

    // Set the rectangle's color.
    GLint colorLocation =
        glGetUniformLocation(shader.ID, "uColor");

    if (colorLocation != -1) {
        glUniform4f(colorLocation, red, green, blue, 1.0f);
    }

    // Upload the rectangle vertices and draw them.
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::shutdown() {
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    shader.destroy();
    initialized = false;
}
