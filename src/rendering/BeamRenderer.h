#pragma once

#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

#include "rendering/Shader.h"
#include "beam/BeamSegment.h"
#include "mirror/Mirror.h"

class BeamRenderer
{
public:
    bool initialize();

    void draw(
        const std::vector<BeamSegment> &path);
    void drawMirrors(
        const std::vector<Mirror> &mirrors,
        std::size_t selectedMirror);
    void shutdown();

private:
    void drawLine(
        const glm::vec2 &start,
        const glm::vec2 &end,
        float thickness,
        const glm::vec4 &color);

    glm::vec2 toClip(
        const glm::vec2 &position) const;

    Shader shader;

    GLuint VAO = 0;
    GLuint VBO = 0;
};