
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

    // Initialize renderer.
    Renderer renderer;

    if (!renderer.initialize())
    {
        std::cerr << "Renderer initialization failed\n";

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    // ==========================================
    // LOAD 3D MIRROR MODEL
    // ==========================================

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

    // Position and scale the mirror.
    glm::mat4 mirrorTransform(1.0f);

    mirrorTransform = glm::translate(
        mirrorTransform,
        glm::vec3(0.5f, 0.0f, 0.0f)
    );

    mirrorTransform = glm::scale(
        mirrorTransform,
        glm::vec3(0.5f, 0.5f, 0.5f)
    );

    // ==========================================
    // LOAD KAYKIT DUNGEON FLOOR
    // ==========================================

    Model floorModel;

    bool floorLoaded = floorModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/floor_tile_large.gltf"
    );

    if (floorLoaded)
    {
        std::cout << "Dungeon floor loaded successfully!\n";
    }
    else
    {
        std::cerr << "Failed to load dungeon floor!\n";
    }

    // Position and scale the dungeon floor.
    glm::mat4 floorTransform(1.0f);

    floorTransform = glm::translate(
        floorTransform,
        glm::vec3(0.0f, -0.5f, 0.0f)
    );

    // ==========================================
    // INITIALIZE PUZZLE
    // ==========================================

    PuzzleSystem puzzle;

    std::cout << "\n=== Reflections of the Tomb ===\n";
    std::cout << "1 - Hit false ruby\n";
    std::cout << "2 - Hit real ruby\n";
    std::cout << "R - Reset puzzle\n";
    std::cout << "ESC - Exit\n";

    bool previous1 = false;
    bool previous2 = false;
    bool previousR = false;

    // ==========================================
    // MAIN GAME LOOP
    // ==========================================

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Exit game.
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

        // ======================================
        // RENDER FRAME
        // ======================================

        renderer.beginFrame();

        // Render the textured dungeon floor first.
        if (floorLoaded)
        {
            renderer.drawModel(
                floorModel,
                floorTransform,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
        }

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

    // ==========================================
    // CLEANUP
    // ==========================================

    // Delete model resources while OpenGL is active.
    floorModel.cleanup();
    mirrorModel.cleanup();

    renderer.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
