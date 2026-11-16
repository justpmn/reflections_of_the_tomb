
#pragma once

#include "Shader.h"
#include "Model.h"

#include <glm/glm.hpp>
#include <vector>
#include "../beam/BeamSegment.h"

class Renderer
{
public:
    Renderer() = default;
    ~Renderer();

    bool initialize();

    void beginFrame();
    void setCamera(const glm::vec3& eye, const glm::vec3& target, float aspect);
    void setLights(const std::vector<glm::vec3>& positions,const std::vector<glm::vec3>& colors);
    void drawShadow(const glm::vec3& position,float radius);
    void drawBox(const glm::vec3& center,const glm::vec3& size,const glm::vec3& color);

    void drawRectangle(
        float x,
        float y,
        float width,
        float height,
        float red,
        float green,
        float blue, float alpha=1);

    void drawModel(
        const Model& model,
        const glm::mat4& modelMatrix,
        const glm::vec3& color, float glow=0);

    void drawModelMesh(
        const Model& model,
        unsigned int meshIndex,
        const glm::mat4& modelMatrix,
        const glm::vec3& color, float glow=0, float clipFloor=-1000);

    void drawGemstone(
        const glm::mat4& transform,
        const glm::vec3& crystalColor, float crystalOffset=0);

    // Draw one glowing 3D beam segment.
    void drawBeam(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color,
        float thickness, float intensity=.28f);

    // Draw multiple connected beam segments.
    void drawBeamSegments(
        const std::vector<BeamSegment>& segments,
        const glm::vec3& color,
        float thickness, float intensity=.28f);

    void shutdown();

private:
    Shader shader;
    Shader modelShader;

    // Dedicated magical beam shader.
    Shader beamShader;

    // 2D rectangle buffers.
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    // Gemstone pedestal buffers.
    unsigned int pedestalVAO = 0;
    unsigned int pedestalVBO = 0;
    int pedestalVertexCount = 0;

    // Gemstone crystal buffers.
    unsigned int crystalVAO = 0;
    unsigned int crystalVBO = 0;
    int crystalVertexCount = 0;

    // Light beam buffers.
    unsigned int beamVAO = 0;
    unsigned int beamVBO = 0;
    int beamVertexCount = 0;

    bool initialized = false;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    std::vector<glm::vec3> lightPositions,lightColors;
    void prepareMaterial(float glow);
};
