#include "BeamRenderer.h"

#include <glm/geometric.hpp>

bool BeamRenderer::initialize()
{
    if (!shader.loadFromFiles(
            "shaders/beam_debug.vert",
            "shaders/beam_debug.frag"))
    {
        return false;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    if (!VAO || !VBO)
    {
        shutdown();
        return false;
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        12 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW);

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), nullptr);

    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return true;
}

glm::vec2 BeamRenderer::toClip(
    const glm::vec2 &p) const
{
    // Same world bounds as Peter's demo:
    // X: -4 to 12, Z: -2 to 7
    return {
        (p.x + 4.0f) / 8.0f - 1.0f,
        (p.y + 2.0f) * (2.0f / 9.0f) - 1.0f};
}

void BeamRenderer::drawLine(
    const glm::vec2 &a,
    const glm::vec2 &b,
    float thickness,
    const glm::vec4 &color)
{
    glm::vec2 direction = b - a;

    if (glm::dot(direction, direction) < 1e-12f)
        return;

    glm::vec2 perpendicular =
        glm::normalize(glm::vec2(
            -direction.y,
            direction.x)) *
        (thickness * 0.5f);

    glm::vec2 points[6] = {
        toClip(a + perpendicular),
        toClip(a - perpendicular),
        toClip(b + perpendicular),

        toClip(b + perpendicular),
        toClip(a - perpendicular),
        toClip(b - perpendicular)};

    float vertices[12];

    for (int i = 0; i < 6; ++i)
    {
        vertices[i * 2] = points[i].x;
        vertices[i * 2 + 1] = points[i].y;
    }

    shader.use();

    GLint colorLocation =
        glGetUniformLocation(shader.ID, "uColor");

    glUniform4f(
        colorLocation,
        color.r,
        color.g,
        color.b,
        color.a);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void BeamRenderer::drawMirrors(
    const std::vector<Mirror> &mirrors,
    std::size_t selectedMirror)
{
    glDisable(GL_DEPTH_TEST);

    for (std::size_t i = 0; i < mirrors.size(); ++i)
    {
        const Mirror &mirror = mirrors[i];

        glm::vec4 color =
            (i == selectedMirror)
                ? glm::vec4(0.2f, 0.9f, 1.0f, 1.0f)
                : glm::vec4(0.7f, 0.75f, 0.85f, 1.0f);

        drawLine(
            mirror.getEndpointA(),
            mirror.getEndpointB(),
            0.17f,
            color);
    }

    glEnable(GL_DEPTH_TEST);
}

void BeamRenderer::draw(
    const std::vector<BeamSegment> &path)
{
    // Render this 2D debug overlay without depth testing.
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA);

    for (const BeamSegment &segment : path)
    {
        glm::vec2 start(
            segment.start.x,
            segment.start.z);

        glm::vec2 end(
            segment.end.x,
            segment.end.z);

        // Soft glow.
        drawLine(
            start, end, 0.29f,
            {1.0f, 0.54f, 0.12f, 0.22f});

        // Golden beam.
        drawLine(
            start, end, 0.105f,
            {1.0f, 0.76f, 0.30f, 1.0f});

        // Bright center.
        drawLine(
            start, end, 0.035f,
            {1.0f, 0.98f, 0.77f, 1.0f});
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void BeamRenderer::shutdown()
{
    if (VBO)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    shader.destroy();
}