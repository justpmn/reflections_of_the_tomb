
#include "Renderer.h"

#include <iostream>
#include <vector>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

namespace
{
    using VertexList = std::vector<float>;

    // Add a triangle with its calculated surface normal.
    void addTriangle(
        VertexList& vertices,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c)
    {
        glm::vec3 normal = glm::normalize(
            glm::cross(b - a, c - a)
        );

        const glm::vec3 points[] = { a, b, c };

        for (const glm::vec3& point : points)
        {
            vertices.push_back(point.x);
            vertices.push_back(point.y);
            vertices.push_back(point.z);

            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    // Add a quad using two triangles.
    void addQuad(
        VertexList& vertices,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d)
    {
        addTriangle(vertices, a, b, c);
        addTriangle(vertices, a, c, d);
    }

    // Add a box using six faces.
    void addBox(
        VertexList& vertices,
        const glm::vec3& center,
        const glm::vec3& size)
    {
        glm::vec3 h = size * 0.5f;

        glm::vec3 p000 = center + glm::vec3(-h.x, -h.y, -h.z);
        glm::vec3 p001 = center + glm::vec3(-h.x, -h.y,  h.z);
        glm::vec3 p010 = center + glm::vec3(-h.x,  h.y, -h.z);
        glm::vec3 p011 = center + glm::vec3(-h.x,  h.y,  h.z);
        glm::vec3 p100 = center + glm::vec3( h.x, -h.y, -h.z);
        glm::vec3 p101 = center + glm::vec3( h.x, -h.y,  h.z);
        glm::vec3 p110 = center + glm::vec3( h.x,  h.y, -h.z);
        glm::vec3 p111 = center + glm::vec3( h.x,  h.y,  h.z);

        addQuad(vertices, p001, p101, p111, p011);
        addQuad(vertices, p100, p000, p010, p110);
        addQuad(vertices, p000, p001, p011, p010);
        addQuad(vertices, p101, p100, p110, p111);
        addQuad(vertices, p010, p011, p111, p110);
        addQuad(vertices, p000, p100, p101, p001);
    }

    // Create the stone pedestal geometry.
    VertexList createPedestal()
    {
        VertexList vertices;

        addBox(
            vertices,
            glm::vec3(0.0f, 0.18f, 0.0f),
            glm::vec3(1.2f, 0.36f, 1.2f)
        );

        addBox(
            vertices,
            glm::vec3(0.0f, 0.65f, 0.0f),
            glm::vec3(0.65f, 0.62f, 0.65f)
        );

        addBox(
            vertices,
            glm::vec3(0.0f, 1.02f, 0.0f),
            glm::vec3(0.95f, 0.18f, 0.95f)
        );

        return vertices;
    }

    // Create a faceted crystal geometry.
    VertexList createCrystal()
    {
        VertexList vertices;

        const float radius = 0.43f;
        const float centerY = 1.75f;

        glm::vec3 top(0.0f, 2.35f, 0.0f);
        glm::vec3 bottom(0.0f, 1.20f, 0.0f);

        glm::vec3 ring[] = {
            {-radius, centerY, 0.0f},
            {0.0f, centerY, radius},
            {radius, centerY, 0.0f},
            {0.0f, centerY, -radius}
        };

        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;

            addTriangle(vertices, top, ring[i], ring[next]);
            addTriangle(vertices, bottom, ring[next], ring[i]);
        }

        return vertices;
    }

    // Create unit beam geometry.
    VertexList createBeam()
    {
        VertexList vertices;

        addBox(
            vertices,
            glm::vec3(0.0f),
            glm::vec3(1.0f)
        );

        return vertices;
    }

    // Upload procedural geometry to OpenGL.
    void createMesh(
        const VertexList& vertices,
        unsigned int& vao,
        unsigned int& vbo,
        int& vertexCount)
    {
        vertexCount = static_cast<int>(vertices.size() / 8);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(float),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            8 * sizeof(float), nullptr
        );
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            8 * sizeof(float),
            reinterpret_cast<void*>(3 * sizeof(float))
        );
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2, 2, GL_FLOAT, GL_FALSE,
            8 * sizeof(float),
            reinterpret_cast<void*>(6 * sizeof(float))
        );
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Configure Blinn-Phong lighting uniforms.
    void setLightingUniforms(const Shader& shader)
    {
        GLint lightDirectionLocation =
            glGetUniformLocation(shader.ID, "uLightDirection");

        GLint viewPositionLocation =
            glGetUniformLocation(shader.ID, "uViewPos");

        GLint ambientLocation =
            glGetUniformLocation(shader.ID, "uAmbientStrength");

        GLint diffuseLocation =
            glGetUniformLocation(shader.ID, "uDiffuseStrength");

        GLint specularLocation =
            glGetUniformLocation(shader.ID, "uSpecularStrength");

        GLint shininessLocation =
            glGetUniformLocation(shader.ID, "uShininess");

        if (lightDirectionLocation != -1)
            glUniform3f(lightDirectionLocation, -0.5f, 1.0f, 0.5f);

        if (viewPositionLocation != -1)
            glUniform3f(viewPositionLocation, 0.0f, 6.0f, 10.0f);

        if (ambientLocation != -1)
            glUniform1f(ambientLocation, 0.35f);

        if (diffuseLocation != -1)
            glUniform1f(diffuseLocation, 0.75f);

        if (specularLocation != -1)
            glUniform1f(specularLocation, 0.25f);

        if (shininessLocation != -1)
            glUniform1f(shininessLocation, 32.0f);
    }

    // Set common 3D transformation uniforms.
    void setModelUniforms(
        const Shader& shader,
        const glm::mat4& modelMatrix,
        const glm::mat4& viewMatrix,
        const glm::mat4& projectionMatrix,
        const glm::vec3& color)
    {
        GLint modelLocation =
            glGetUniformLocation(shader.ID, "model");

        GLint viewLocation =
            glGetUniformLocation(shader.ID, "view");

        GLint projectionLocation =
            glGetUniformLocation(shader.ID, "projection");

        GLint colorLocation =
            glGetUniformLocation(shader.ID, "uColor");

        if (modelLocation != -1)
        {
            glUniformMatrix4fv(
                modelLocation, 1, GL_FALSE,
                glm::value_ptr(modelMatrix)
            );
        }

        if (viewLocation != -1)
        {
            glUniformMatrix4fv(
                viewLocation, 1, GL_FALSE,
                glm::value_ptr(viewMatrix)
            );
        }

        if (projectionLocation != -1)
        {
            glUniformMatrix4fv(
                projectionLocation, 1, GL_FALSE,
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
    }
}

// Destructor.
Renderer::~Renderer()
{
    shutdown();
}

// Initialize renderer.
bool Renderer::initialize()
{
    if (initialized)
        return true;

    if (!shader.loadFromFiles(
        "shaders/basic.vert",
        "shaders/basic.frag"))
    {
        std::cerr << "Failed to load 2D shaders.\n";
        return false;
    }

    if (!modelShader.loadFromFiles(
        "shaders/model.vert",
        "shaders/model.frag"))
    {
        std::cerr << "Failed to load 3D model shaders.\n";
        shader.destroy();
        return false;
    }

    if (!beamShader.loadFromFiles(
        "shaders/beam.vert",
        "shaders/beam.frag"))
    {
        std::cerr << "Failed to load beam shaders.\n";
        shader.destroy();
        modelShader.destroy();
        return false;
    }

    // Create 2D rectangle buffers.
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

    // Create gemstone pedestal geometry.
    createMesh(
        createPedestal(),
        pedestalVAO,
        pedestalVBO,
        pedestalVertexCount
    );

    // Create gemstone crystal geometry.
    createMesh(
        createCrystal(),
        crystalVAO,
        crystalVBO,
        crystalVertexCount
    );

    // Create beam geometry.
    createMesh(
        createBeam(),
        beamVAO,
        beamVBO,
        beamVertexCount
    );

    if (pedestalVAO == 0 ||
        crystalVAO == 0 ||
        beamVAO == 0)
    {
        std::cerr << "Failed to create procedural geometry.\n";
        shutdown();
        return false;
    }

    // Configure elevated 3D camera.
    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 6.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    projectionMatrix = glm::perspective(
        glm::radians(40.0f),
        1280.0f / 720.0f,
        0.1f,
        100.0f
    );

    initialized = true;

    std::cout << "Renderer initialized successfully.\n";

    return true;
}

void Renderer::setCamera(const glm::vec3& eye, const glm::vec3& target, float aspect)
{
    viewMatrix = glm::lookAt(eye, target, glm::vec3(0,1,0));
    projectionMatrix = glm::perspective(glm::radians(45.f), aspect, .1f, 180.f);
}

// Clear the screen.
void Renderer::beginFrame()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(
        0.05f,
        0.04f,
        0.08f,
        1.0f
    );

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );
}

// Draw a 2D rectangle.
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
        return;

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
            red,
            green,
            blue,
            1.0f
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

// Draw an imported 3D model.
void Renderer::drawModel(
    const Model& model,
    const glm::mat4& transform,
    const glm::vec3& color)
{
    if (!initialized)
        return;

    modelShader.use();
    setLightingUniforms(modelShader);

    setModelUniforms(
        modelShader,
        transform,
        viewMatrix,
        projectionMatrix,
        color
    );

    model.draw(modelShader.ID);
}

// Draw one selected mesh.
void Renderer::drawModelMesh(
    const Model& model,
    unsigned int meshIndex,
    const glm::mat4& transform,
    const glm::vec3& color)
{
    if (!initialized)
        return;

    modelShader.use();
    setLightingUniforms(modelShader);

    setModelUniforms(
        modelShader,
        transform,
        viewMatrix,
        projectionMatrix,
        color
    );

    model.drawMesh(meshIndex, modelShader.ID);
}

// Draw gemstone and pedestal.
void Renderer::drawGemstone(
    const glm::mat4& transform,
    const glm::vec3& crystalColor)
{
    if (!initialized)
        return;

    modelShader.use();
    setLightingUniforms(modelShader);

    GLint hasTextureLocation =
        glGetUniformLocation(modelShader.ID, "uHasTexture");

    if (hasTextureLocation != -1)
        glUniform1i(hasTextureLocation, GL_FALSE);

    // Draw pedestal.
    setModelUniforms(
        modelShader,
        transform,
        viewMatrix,
        projectionMatrix,
        glm::vec3(0.38f, 0.34f, 0.29f)
    );

    glBindVertexArray(pedestalVAO);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        pedestalVertexCount
    );

    // Draw crystal.
    GLint colorLocation =
        glGetUniformLocation(modelShader.ID, "uColor");

    if (colorLocation != -1)
    {
        glUniform3f(
            colorLocation,
            crystalColor.r,
            crystalColor.g,
            crystalColor.b
        );
    }

    glBindVertexArray(crystalVAO);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        crystalVertexCount
    );

    glBindVertexArray(0);
}

// Draw one 3D beam segment.
void Renderer::drawBeam(
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& color,
    float thickness)
{
    if (!initialized || beamVAO == 0)
        return;

    glm::vec3 difference = end - start;

    float beamLength = glm::length(difference);

    if (beamLength < 0.0001f || thickness <= 0.0f)
        return;

    glm::vec3 direction = difference / beamLength;

    // The beam geometry points along local positive Y.
    glm::vec3 localUp(0.0f, 1.0f, 0.0f);

    float dotProduct = glm::clamp(
        glm::dot(localUp, direction),
        -1.0f,
        1.0f
    );

    glm::vec3 rotationAxis =
        glm::cross(localUp, direction);

    float axisLength = glm::length(rotationAxis);

    glm::mat4 rotation(1.0f);

    if (axisLength > 0.0001f)
    {
        rotationAxis /= axisLength;

        float angle = std::acos(dotProduct);

        rotation = glm::rotate(
            glm::mat4(1.0f),
            angle,
            rotationAxis
        );
    }
    else if (dotProduct < 0.0f)
    {
        rotation = glm::rotate(
            glm::mat4(1.0f),
            glm::pi<float>(),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
    }

    glm::vec3 midpoint = (start + end) * 0.5f;

    glm::mat4 beamTransform(1.0f);

    beamTransform = glm::translate(
        beamTransform,
        midpoint
    );

    beamTransform *= rotation;

    beamTransform = glm::scale(
        beamTransform,
        glm::vec3(thickness, beamLength, thickness)
    );

    beamShader.use();

    GLint modelLocation =
        glGetUniformLocation(beamShader.ID, "model");

    GLint viewLocation =
        glGetUniformLocation(beamShader.ID, "view");

    GLint projectionLocation =
        glGetUniformLocation(beamShader.ID, "projection");

    GLint colorLocation =
        glGetUniformLocation(beamShader.ID, "uBeamColor");

    GLint intensityLocation =
        glGetUniformLocation(beamShader.ID, "uIntensity");

    if (modelLocation != -1)
    {
        glUniformMatrix4fv(
            modelLocation,
            1,
            GL_FALSE,
            glm::value_ptr(beamTransform)
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

    if (intensityLocation != -1)
        glUniform1f(intensityLocation, 2.5f);

    // Additive blending creates a brighter emissive appearance.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArray(beamVAO);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        beamVertexCount
    );

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

// Draw multiple beam segments.
void Renderer::drawBeamSegments(
    const std::vector<BeamSegment>& segments,
    const glm::vec3& color,
    float thickness)
{
    if (!initialized || segments.empty())
        return;

    for (const BeamSegment& segment : segments)
    {
        drawBeam(
            segment.start,
            segment.end,
            color,
            thickness
        );
    }
}

// Release OpenGL resources.
void Renderer::shutdown()
{
    if (beamVBO != 0)
    {
        glDeleteBuffers(1, &beamVBO);
        beamVBO = 0;
    }

    if (beamVAO != 0)
    {
        glDeleteVertexArrays(1, &beamVAO);
        beamVAO = 0;
    }

    if (pedestalVBO != 0)
    {
        glDeleteBuffers(1, &pedestalVBO);
        pedestalVBO = 0;
    }

    if (pedestalVAO != 0)
    {
        glDeleteVertexArrays(1, &pedestalVAO);
        pedestalVAO = 0;
    }

    if (crystalVBO != 0)
    {
        glDeleteBuffers(1, &crystalVBO);
        crystalVBO = 0;
    }

    if (crystalVAO != 0)
    {
        glDeleteVertexArrays(1, &crystalVAO);
        crystalVAO = 0;
    }

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
    beamShader.destroy();

    pedestalVertexCount = 0;
    crystalVertexCount = 0;
    beamVertexCount = 0;

    initialized = false;
}
