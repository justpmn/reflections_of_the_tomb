
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
    // ==========================================
    // INITIALIZE GLFW
    // ==========================================

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
        "Reflections of the Tomb",
        nullptr, nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // ==========================================
    // INITIALIZE GLAD
    // ==========================================

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

    // ==========================================
    // INITIALIZE RENDERER
    // ==========================================

    Renderer renderer;

    if (!renderer.initialize())
    {
        std::cerr << "Renderer initialization failed\n";

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    // ==========================================
    // LOAD MIRROR MODEL
    // ==========================================

    Model mirrorModel;

    bool mirrorLoaded = mirrorModel.load(
        "assets/models/mirrors/mirror_complete.glb"
    );

    if (mirrorLoaded)
    {
        std::cout << "Mirror model loaded successfully!\n";
    }
    else
    {
        std::cerr << "Failed to load mirror model!\n";
    }

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
    // LOAD DUNGEON FLOOR
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

    // ==========================================
    // LOAD TEMPLE WALLS
    // ==========================================

    Model wallModel;

    bool wallsLoaded = wallModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/wall.gltf"
    );

    if (wallsLoaded)
    {
        std::cout << "Temple wall model loaded successfully!\n";
    }
    else
    {
        std::cerr << "Failed to load temple wall model!\n";
    }

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

        // Handle puzzle input.
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
        // BEGIN FRAME
        // ======================================

        renderer.beginFrame();

        // ======================================
        // RENDER 3 X 3 DUNGEON FLOOR
        // ======================================

        if (floorLoaded)
        {
            for (int x = -1; x <= 1; x++)
            {
                for (int z = -1; z <= 1; z++)
                {
                    glm::mat4 tileTransform(1.0f);

                    tileTransform = glm::translate(
                        tileTransform,
                        glm::vec3(
                            x * 4.0f,
                            -0.5f,
                            z * 4.0f
                        )
                    );

                    renderer.drawModel(
                        floorModel,
                        tileTransform,
                        glm::vec3(1.0f, 1.0f, 1.0f)
                    );
                }
            }
        }

        // ======================================
        // RENDER BACK WALL
        // ======================================

        if (wallsLoaded)
        {
            for (int i = -1; i <= 1; i++)
            {
                glm::mat4 backWall(1.0f);

                backWall = glm::translate(
                    backWall,
                    glm::vec3(
                        i * 4.0f,
                        -0.5f,
                        -6.5f
                    )
                );

                renderer.drawModel(
                    wallModel,
                    backWall,
                    glm::vec3(1.0f)
                );
            }

            // ==================================
            // RENDER LEFT AND RIGHT WALLS
            // ==================================

            for (int i = -1; i <= 1; i++)
            {
                // Left wall.
                glm::mat4 leftWall(1.0f);

                leftWall = glm::translate(
                    leftWall,
                    glm::vec3(
                        -6.5f,
                        -0.5f,
                        i * 4.0f
                    )
                );

                leftWall = glm::rotate(
                    leftWall,
                    glm::radians(90.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                renderer.drawModel(
                    wallModel,
                    leftWall,
                    glm::vec3(1.0f)
                );

                // Right wall.
                glm::mat4 rightWall(1.0f);

                rightWall = glm::translate(
                    rightWall,
                    glm::vec3(
                        6.5f,
                        -0.5f,
                        i * 4.0f
                    )
                );

                rightWall = glm::rotate(
                    rightWall,
                    glm::radians(90.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                renderer.drawModel(
                    wallModel,
                    rightWall,
                    glm::vec3(1.0f)
                );
            }

            // ==================================
            // FRONT WALL
            // Leave the centre open for entry.
            // ==================================

            for (int i : {-1, 1})
            {
                glm::mat4 frontWall(1.0f);

                frontWall = glm::translate(
                    frontWall,
                    glm::vec3(
                        i * 4.0f,
                        -0.5f,
                        6.5f
                    )
                );

                renderer.drawModel(
                    wallModel,
                    frontWall,
                    glm::vec3(1.0f)
                );
            }
        }

        // ======================================
        // RENDER EXISTING PUZZLE PLACEHOLDERS
        // ======================================

        // Real ruby.
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

        // False ruby.
        renderer.drawRectangle(
            -0.2f, 0.2f, 0.3f, 0.3f,
            0.6f, 0.1f, 0.8f
        );

        // Door placeholder.
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

        // Trap placeholder.
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

        // ======================================
        // RENDER 3D MIRROR
        // ======================================

        if (mirrorLoaded)
        {
            renderer.drawModel(
                mirrorModel,
                mirrorTransform,
                glm::vec3(0.8f, 0.8f, 0.8f)
            );
        }

        // ======================================
        // END FRAME
        // ======================================

        glfwSwapBuffers(window);
    }

    // ==========================================
    // CLEANUP OPENGL RESOURCES
    // ==========================================

    floorModel.cleanup();
    wallModel.cleanup();
    mirrorModel.cleanup();

    renderer.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
