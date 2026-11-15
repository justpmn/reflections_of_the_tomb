
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "beam/BeamTracer.h"
#include "rendering/Shader.h"

namespace
{
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;

    // Visible horizontal world area:
    // X: -4 to 12, Z: -2 to 7
    // Both axes have 80 pixels per world unit.
    glm::vec2 toClip(const glm::vec2& p)
    {
        return {
            (p.x + 4.0f) / 8.0f - 1.0f,
            (p.y + 2.0f) * (2.0f / 9.0f) - 1.0f
        };
    }

    // Draw thick line segments as two OpenGL triangles.
    // This avoids driver-dependent GL line widths.
    struct LineCanvas
    {
        Shader shader;
        GLuint vao = 0;
        GLuint vbo = 0;

        bool initialize()
        {
            if (!shader.loadFromFiles(
                "shaders/beam_debug.vert",
                "shaders/beam_debug.frag"))
            {
                return false;
            }

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            if (!vao || !vbo)
                return false;

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            glBufferData(
                GL_ARRAY_BUFFER,
                sizeof(float) * 12,
                nullptr,
                GL_DYNAMIC_DRAW
            );

            glVertexAttribPointer(
                0, 2, GL_FLOAT, GL_FALSE,
                2 * sizeof(float), nullptr
            );

            glEnableVertexAttribArray(0);
            glBindVertexArray(0);

            return true;
        }

        void line(
            glm::vec2 a,
            glm::vec2 b,
            float thickness,
            const glm::vec4& color)
        {
            glm::vec2 direction = b - a;

            if (glm::dot(direction, direction) < 1e-12f)
                return;

            glm::vec2 perpendicular =
                glm::normalize(glm::vec2(
                    -direction.y,
                    direction.x
                )) * (thickness * 0.5f);

            glm::vec2 p[6] = {
                toClip(a + perpendicular),
                toClip(a - perpendicular),
                toClip(b + perpendicular),

                toClip(b + perpendicular),
                toClip(a - perpendicular),
                toClip(b - perpendicular)
            };

            float vertices[12];

            for (int i = 0; i < 6; ++i)
            {
                vertices[i * 2] = p[i].x;
                vertices[i * 2 + 1] = p[i].y;
            }

            shader.use();

            GLint location = glGetUniformLocation(
                shader.ID, "uColor"
            );

            glUniform4f(
                location,
                color.r, color.g, color.b, color.a
            );

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                sizeof(vertices),
                vertices
            );

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        void shutdown()
        {
            if (vbo)
            {
                glDeleteBuffers(1, &vbo);
                vbo = 0;
            }

            if (vao)
            {
                glDeleteVertexArrays(1, &vao);
                vao = 0;
            }

            shader.destroy();
        }
    };
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    // Fixed size preserves the debug coordinate scale.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "Reflections of the Tomb - Beam Debug",
        nullptr,
        nullptr
    );

    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(
        reinterpret_cast<GLADloadfunc>(
            glfwGetProcAddress)))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    LineCanvas canvas;

    if (!canvas.initialize())
    {
        canvas.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Two mirrors:
    // +X -> +Z -> -X
    std::vector<Mirror> mirrors;

    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f, 45.0f, -180.0f, 180.0f
    );

    mirrors.emplace_back(
        glm::vec2(5.0f, 5.0f),
        4.0f, 135.0f, -180.0f, 180.0f
    );

    // Rectangle surrounding the puzzle area.
    std::vector<WallSegment> walls = {
        {{-3.0f, -1.5f}, {9.0f, -1.5f}},
        {{ 9.0f, -1.5f}, {9.0f,  6.5f}},
        {{ 9.0f,  6.5f}, {-3.0f, 6.5f}},
        {{-3.0f,  6.5f}, {-3.0f, -1.5f}}
    };

    const glm::vec2 emitter(0.0f, 0.0f);
    const glm::vec2 ruby(-2.0f, 5.0f);

    const glm::vec3 rubyPosition(
        ruby.x, 1.0f, ruby.y
    );
    const std::vector<RubyTarget> rubies = {
        {rubyPosition, 0.3f, true}
    };

    std::size_t selectedMirror = 0;
    double previousTime = glfwGetTime();

    std::cout << "\n=== LIVE BEAM DEBUG DEMO ===\n";
    std::cout << "1 / 2: Select mirror\n";
    std::cout << "Q / E: Rotate selected mirror\n";
    std::cout << "R: Reset mirror angles\n";
    std::cout << "ESC: Exit\n\n";

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        double now = glfwGetTime();

        float dt = std::min(
            static_cast<float>(now - previousTime),
            0.05f
        );

        previousTime = now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE)
            == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_1)
            == GLFW_PRESS)
        {
            selectedMirror = 0;
        }

        if (glfwGetKey(window, GLFW_KEY_2)
            == GLFW_PRESS)
        {
            selectedMirror = 1;
        }

        const float rotationSpeed = 50.0f;

        if (glfwGetKey(window, GLFW_KEY_Q)
            == GLFW_PRESS)
        {
            mirrors[selectedMirror].rotateLeft(
                rotationSpeed * dt
            );
        }

        if (glfwGetKey(window, GLFW_KEY_E)
            == GLFW_PRESS)
        {
            mirrors[selectedMirror].rotateRight(
                rotationSpeed * dt
            );
        }

        if (glfwGetKey(window, GLFW_KEY_R)
            == GLFW_PRESS)
        {
            mirrors[0].setAngle(45.0f);
            mirrors[1].setAngle(135.0f);
        }

        // Recalculate the entire beam path each frame.
        // Trace mirrors, walls and absorbing ruby targets.
BeamTraceResult traced = BeamTracer::traceWithRubies(
    emitter,
    {1.0f, 0.0f},
    mirrors,
    rubies,
    4,
    30.0f,
    1.0f,
    walls
);

const std::vector<BeamSegment>& path = traced.path;

// In this demo, the only ruby has index 0.
bool rubyHit = traced.hitRubyIndex == 0;

        // Detect whether any valid segment reaches the ruby.

        glClearColor(0.035f, 0.04f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw temple boundary walls.
        for (const WallSegment& wall : walls)
        {
            canvas.line(
                wall.start,
                wall.end,
                0.13f,
                {0.40f, 0.43f, 0.50f, 1.0f}
            );
        }

        // Draw the golden light path.
        for (const BeamSegment& segment : path)
        {
            glm::vec2 a(
                segment.start.x,
                segment.start.z
            );

            glm::vec2 b(
                segment.end.x,
                segment.end.z
            );

            // Soft glow.
            canvas.line(
                a, b, 0.29f,
                {1.0f, 0.54f, 0.12f, 0.22f}
            );

            // Golden beam.
            canvas.line(
                a, b, 0.105f,
                {1.0f, 0.76f, 0.30f, 1.0f}
            );

            // Bright center.
            canvas.line(
                a, b, 0.035f,
                {1.0f, 0.98f, 0.77f, 1.0f}
            );
        }

        // Draw mirror surfaces.
        for (std::size_t i = 0; i < mirrors.size(); ++i)
        {
            const Mirror& mirror = mirrors[i];

            glm::vec4 color =
                (i == selectedMirror)
                ? glm::vec4(0.45f, 0.95f, 1.0f, 1.0f)
                : glm::vec4(0.65f, 0.75f, 0.85f, 1.0f);

            canvas.line(
                mirror.getEndpointA(),
                mirror.getEndpointB(),
                0.17f,
                color
            );
        }

        // Draw emitter marker.
        canvas.line(
            emitter + glm::vec2(-0.15f, 0.0f),
            emitter + glm::vec2(0.15f, 0.0f),
            0.30f,
            {1.0f, 0.85f, 0.35f, 1.0f}
        );

        // Draw ruby as a diamond.
        glm::vec4 rubyColor =
            rubyHit
            ? glm::vec4(0.20f, 1.0f, 0.48f, 1.0f)
            : glm::vec4(0.95f, 0.12f, 0.28f, 1.0f);

        glm::vec2 top = ruby + glm::vec2(0.0f, 0.35f);
        glm::vec2 right = ruby + glm::vec2(0.35f, 0.0f);
        glm::vec2 bottom = ruby + glm::vec2(0.0f, -0.35f);
        glm::vec2 left = ruby + glm::vec2(-0.35f, 0.0f);

        canvas.line(top, right, 0.12f, rubyColor);
        canvas.line(right, bottom, 0.12f, rubyColor);
        canvas.line(bottom, left, 0.12f, rubyColor);
        canvas.line(left, top, 0.12f, rubyColor);

        // Display status in the window title.
        std::ostringstream title;

        title << "Beam Debug | Mirror "
              << (selectedMirror + 1)
              << " | Angle: "
              << std::fixed << std::setprecision(1)
              << mirrors[selectedMirror].getAngle()
              << " | Ruby: "
              << (rubyHit ? "ACTIVATED" : "INACTIVE")
              << " | Segments: " << path.size();

        glfwSetWindowTitle(window, title.str().c_str());

        glfwSwapBuffers(window);
    }

    canvas.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
