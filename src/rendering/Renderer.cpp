
#include "Renderer.h"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Renderer::~Renderer()
{
    shutdown();
}

bool Renderer::initialize()
{
    if (initialized)
    {
        return true;
    }

    // Load the existing 2D shaders.
    if (!shader.loadFromFiles(
        "shaders/basic.vert",
        "shaders/basic.frag"))
    {
        std::cerr << "Failed to load 2D shaders.\n";
        return false;
    }

    // Load the new 3D model shaders.
    if (!modelShader.loadFromFiles(
        "shaders/model.vert",
        "shaders/model.frag"))
    {
        std::cerr << "Failed to load 3D model shaders.\n";
        shader.destroy();
        return false;
    }

    // Create the 2D rectangle buffers.
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    if (VAO == 0 || VBO == 0)
    {
        std::cerr << "Failed to create rendering buffers.\n";
        shutdown();
        return false;
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        18 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Configure the 3D camera.
    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 2.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        1280.0f / 720.0f,
        0.1f,
        100.0f
    );

    initialized = true;

    std::cout << "Renderer initialized successfully.\n";

    return true;
}

void Renderer::beginFrame()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.05f, 0.04f, 0.08f, 1.0f);

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );
}

void Renderer::drawRectangle(
    float x,
    float y,
    float width,
    float height,
    float red,
    float green,
    float blue)
{
    if (!initialized)
    {
        return;
    }

    const float vertices[] = {
        x,          y,           0.0f,
        x + width,  y,           0.0f,
        x + width,  y + height,  0.0f,

        x,          y,           0.0f,
        x + width,  y + height,  0.0f,
        x,          y + height,  0.0f
    };

    shader.use();

    GLint colorLocation =
        glGetUniformLocation(shader.ID, "uColor");

    if (colorLocation != -1)
    {
        glUniform4f(
            colorLocation,
            red, green, blue, 1.0f
        );
    }

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

void Renderer::drawModel(
    const Model& model,
    const glm::mat4& transform,
    const glm::vec3& color)
{
    if (!initialized)
    {
        return;
    }

    modelShader.use();

    GLint modelLocation =
        glGetUniformLocation(modelShader.ID, "model");

    GLint viewLocation =
        glGetUniformLocation(modelShader.ID, "view");

    GLint projectionLocation =
        glGetUniformLocation(modelShader.ID, "projection");

    GLint colorLocation =
        glGetUniformLocation(modelShader.ID, "uColor");

    if (modelLocation != -1)
    {
        glUniformMatrix4fv(
            modelLocation,
            1,
            GL_FALSE,
            glm::value_ptr(transform)
        );
    }

    if (viewLocation != -1)
    {
        glUniformMatrix4fv(
            viewLocation,
            1,
            GL_FALSE,
            glm::value_ptr(viewMatrix)
        );
    }

    if (projectionLocation != -1)
    {
        glUniformMatrix4fv(
            projectionLocation,
            1,
            GL_FALSE,
            glm::value_ptr(projectionMatrix)
        );
    }

    if (colorLocation != -1)
    {
        glUniform3f(
            colorLocation,
            color.r,
            color.g,
            color.b
        );
    }

    model.draw();
}

void Renderer::shutdown()
{
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    shader.destroy();
    modelShader.destroy();

    initialized = false;
}
