#pragma once

#include "Shader.h"
#include "Model.h"

#include <glm/glm.hpp>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool initialize();

    void beginFrame();

    void drawRectangle(
        float x,
        float y,
        float width,
        float height,
        float red,
        float green,
        float blue
    );

    void drawModel(
        const Model& model,
        const glm::mat4& modelMatrix,
        const glm::vec3& color
    );

    // Draw one selected mesh using its own transform.
    void drawModelMesh(
        const Model& model,
        unsigned int meshIndex,
        const glm::mat4& modelMatrix,
        const glm::vec3& color
    );

    // Draw a procedural gemstone and its stone pedestal.
    void drawGemstone(
        const glm::mat4& transform,
        const glm::vec3& crystalColor
    );

    void shutdown();

private:
    Shader shader;
    Shader modelShader;

    unsigned int VAO = 0;
    unsigned int VBO = 0;

    unsigned int pedestalVAO = 0;
    unsigned int pedestalVBO = 0;
    int pedestalVertexCount = 0;

    unsigned int crystalVAO = 0;
    unsigned int crystalVBO = 0;
    int crystalVertexCount = 0;

    bool initialized = false;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};