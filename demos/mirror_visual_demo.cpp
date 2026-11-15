
#include <iostream>
#include <vector>
#include <cstddef>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "mirror/Mirror.h"

#include <string>

struct Vertex {
    float x, y;
    float r, g, b;
};

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 vertexColor;
out vec3 color;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    color = vertexColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 color;
out vec4 FragColor;

void main() {
    FragColor = vec4(color, 1.0);
}
)";

GLuint createShaderProgram() {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        900, 600, "Mirror Transformation Demo", nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "GLAD initialization failed\n";
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, 900, 600);

    GLuint shader = createShaderProgram();

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        5 * sizeof(Vertex),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, x)
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, r)
    );
    glEnableVertexAttribArray(1);

    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f,
                  0.0f, -80.0f, 80.0f);

    bool qWasPressed = false;
    bool eWasPressed = false;
    bool rWasPressed = false;

    std::cout << "Q: Rotate left | E: Rotate right | R: Reset\n";

    while (!glfwWindowShouldClose(window)) {
        bool qPressed = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        bool rPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

        if (qPressed && !qWasPressed)
            mirror.rotateLeft();

        if (ePressed && !eWasPressed)
            mirror.rotateRight();

        if (rPressed && !rWasPressed)
            mirror.setAngle(0.0f);

        qWasPressed = qPressed;
        eWasPressed = ePressed;
        rWasPressed = rPressed;

        glm::vec2 a = mirror.getEndpointA();
        glm::vec2 b = mirror.getEndpointB();
        glm::vec2 c = mirror.getCenter();
        glm::vec2 n = mirror.getNormal();

        const float scale = 0.35f;

        std::vector<Vertex> vertices = {
            {a.x * scale, a.y * scale, 0.2f, 0.8f, 1.0f},
            {b.x * scale, b.y * scale, 0.2f, 0.8f, 1.0f},

            {c.x * scale, c.y * scale, 1.0f, 0.8f, 0.1f},
            {(c.x + n.x * 0.8f) * scale,
             (c.y + n.y * 0.8f) * scale, 1.0f, 0.3f, 0.3f},

            {c.x * scale, c.y * scale, 1.0f, 0.8f, 0.1f}
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            vertices.size() * sizeof(Vertex),
            vertices.data()
        );

        glClearColor(0.06f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glBindVertexArray(VAO);

        glLineWidth(6.0f);
        glDrawArrays(GL_LINES, 0, 4);

        glPointSize(14.0f);
        glDrawArrays(GL_POINTS, 4, 1);

        glfwSetWindowTitle(
            window,
            ("Mirror Transformation Demo | Angle: " +
             std::to_string(mirror.getAngle()) +
             " degrees | Q/E rotate, R reset").c_str()
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
