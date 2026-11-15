
#include "Renderer.h"

#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    using VertexList = std::vector<float>;

    // Add one triangle with its calculated surface normal.
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

    // Add a box using six faces.
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

        // Position attribute.
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            8 * sizeof(float), nullptr
        );
        glEnableVertexAttribArray(0);

        // Normal attribute.
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            8 * sizeof(float),
            reinterpret_cast<void*>(3 * sizeof(float))
        );
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute.
        glVertexAttribPointer(
            2, 2, GL_FLOAT, GL_FALSE,
            8 * sizeof(float),
            reinterpret_cast<void*>(6 * sizeof(float))
        );
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
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

    // Load 2D shaders.
    if (!shader.loadFromFiles(
        "shaders/basic.vert",
        "shaders/basic.frag"))
    {
        std::cerr << "Failed to load 2D shaders.\n";
        return false;
    }

    // Load 3D model shaders.
    if (!modelShader.loadFromFiles(
        "shaders/model.vert",
        "shaders/model.frag"))
    {
        std::cerr << "Failed to load 3D model shaders.\n";
        shader.destroy();
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

    if (pedestalVAO == 0 || crystalVAO == 0)
    {
        std::cerr << "Failed to create gemstone geometry.\n";
        shutdown();
        return false;
    }

    // Configure camera.
    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 7.0f, 12.0f),
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
            modelLocation, 1, GL_FALSE,
            glm::value_ptr(transform)
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

    model.draw(modelShader.ID);
}

// Draw only one selected mesh from an imported model.
void Renderer::drawModelMesh(
    const Model& model,
    unsigned int meshIndex,
    const glm::mat4& transform,
    const glm::vec3& color)
{
    if (!initialized)
        return;

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
            modelLocation, 1, GL_FALSE,
            glm::value_ptr(transform)
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

    model.drawMesh(meshIndex, modelShader.ID);
}

// Draw a gemstone and its pedestal.
void Renderer::drawGemstone(
    const glm::mat4& transform,
    const glm::vec3& crystalColor)
{
    if (!initialized)
        return;

    modelShader.use();

    GLint modelLocation =
        glGetUniformLocation(modelShader.ID, "model");

    GLint viewLocation =
        glGetUniformLocation(modelShader.ID, "view");

    GLint projectionLocation =
        glGetUniformLocation(modelShader.ID, "projection");

    GLint colorLocation =
        glGetUniformLocation(modelShader.ID, "uColor");

    GLint hasTextureLocation =
        glGetUniformLocation(modelShader.ID, "uHasTexture");

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

    // These procedural objects do not use textures.
    if (hasTextureLocation != -1)
        glUniform1i(hasTextureLocation, GL_FALSE);

    // Draw stone pedestal.
    if (modelLocation != -1)
    {
        glUniformMatrix4fv(
            modelLocation, 1, GL_FALSE,
            glm::value_ptr(transform)
        );
    }

    if (colorLocation != -1)
    {
        glUniform3f(
            colorLocation,
            0.38f,
            0.34f,
            0.29f
        );
    }

    glBindVertexArray(pedestalVAO);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        pedestalVertexCount
    );

    // Draw crystal.
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

// Release OpenGL resources.
void Renderer::shutdown()
{
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

    pedestalVertexCount = 0;
    crystalVertexCount = 0;

    initialized = false;
}
