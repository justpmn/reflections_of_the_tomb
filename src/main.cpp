#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/Renderer.h"
#include "rendering/Model.h"
#include "gameplay/PuzzleSystem.h"

void framebufferSizeCallback(
    GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    GLFWwindow* window = glfwCreateWindow(
        1280, 720,
        "Reflections of the Tomb - Puzzle Demo",
        nullptr, nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(
        reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(
        window, framebufferSizeCallback
    );

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Renderer renderer;

    if (!renderer.initialize())
    {
        std::cerr << "Renderer initialization failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Load the 3D mirror model.
    Model mirrorModel;
    bool mirrorLoaded = mirrorModel.load(
        "assets/models/mirrors/mirror_complete.glb"
    );

    if (mirrorLoaded)
    {
        std::cout << "3D mirror model loaded successfully!\n";
    }
    else
    {
        std::cerr << "Failed to load 3D mirror model\n";
    }

    // Initial mirror position and scale.
    glm::mat4 mirrorTransform = glm::mat4(1.0f);

    mirrorTransform = glm::translate(
        mirrorTransform,
        glm::vec3(0.5f, 0.0f, 0.0f)
    );

    mirrorTransform = glm::scale(
        mirrorTransform,
        glm::vec3(0.5f, 0.5f, 0.5f)
    );

    PuzzleSystem puzzle;

    std::cout << "=== Reflections of the Tomb ===\n";
    std::cout << "1 - Hit false ruby\n";
    std::cout << "2 - Hit real ruby\n";
    std::cout << "R - Reset puzzle\n";
    std::cout << "ESC - Exit\n";

    bool previous1 = false;
    bool previous2 = false;
    bool previousR = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE)
            == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        bool key1 = glfwGetKey(
            window, GLFW_KEY_1) == GLFW_PRESS;

        bool key2 = glfwGetKey(
            window, GLFW_KEY_2) == GLFW_PRESS;

        bool keyR = glfwGetKey(
            window, GLFW_KEY_R) == GLFW_PRESS;

        // Respond once per key press.
        if (key1 && !previous1)
        {
            puzzle.hitRuby(false);
            std::cout << "False ruby hit!\n";
        }

        if (key2 && !previous2)
        {
            puzzle.hitRuby(true);
            std::cout << "Real ruby hit!\n";
        }

        if (keyR && !previousR)
        {
            puzzle.reset();
            std::cout << "Puzzle reset!\n";
        }

        previous1 = key1;
        previous2 = key2;
        previousR = keyR;

        renderer.beginFrame();

        // Real ruby: red until puzzle solved.
        if (puzzle.isSolved())
        {
            renderer.drawRectangle(
                -0.7f, 0.2f, 0.3f, 0.3f,
                0.1f, 1.0f, 0.2f
            );
        }
        else
        {
            renderer.drawRectangle(
                -0.7f, 0.2f, 0.3f, 0.3f,
                0.9f, 0.1f, 0.2f
            );
        }

        // False ruby: purple.
        renderer.drawRectangle(
            -0.2f, 0.2f, 0.3f, 0.3f,
            0.6f, 0.1f, 0.8f
        );

        // Door: brown when closed, green when open.
        if (puzzle.isDoorOpen())
        {
            renderer.drawRectangle(
                0.5f, -0.2f, 0.25f, 0.7f,
                0.1f, 0.8f, 0.2f
            );
        }
        else
        {
            renderer.drawRectangle(
                0.5f, -0.2f, 0.25f, 0.7f,
                0.5f, 0.3f, 0.1f
            );
        }

        // Trap: red when active, gray when safe.
        if (puzzle.areTrapsActive())
        {
            renderer.drawRectangle(
                -0.7f, -0.7f, 0.8f, 0.2f,
                1.0f, 0.1f, 0.1f
            );
        }
        else
        {
            renderer.drawRectangle(
                -0.7f, -0.7f, 0.8f, 0.2f,
                0.4f, 0.4f, 0.4f
            );
        }

        // Render the 3D mirror.
        if (mirrorLoaded)
        {
            renderer.drawModel(
                mirrorModel,
                mirrorTransform,
                glm::vec3(0.8f, 0.8f, 0.8f)
            );
        }

        glfwSwapBuffers(window);
    }

    // Clean up while the OpenGL context is still active.
    mirrorModel.cleanup();
    renderer.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}