
#include <iostream>
#include <algorithm>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/Renderer.h"
#include "rendering/Model.h"
#include "gameplay/PuzzleSystem.h"

// ==========================================
// WINDOW RESIZE CALLBACK
// ==========================================

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ==========================================
// MAIN FUNCTION
// ==========================================

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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "Reflections of the Tomb",
        nullptr,
        nullptr
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
        window,
        framebufferSizeCallback
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
        std::cout << "Mirror model loaded successfully!\n";
    else
        std::cerr << "Failed to load mirror model!\n";

    glm::mat4 mirrorTransform(1.0f);

    mirrorTransform = glm::translate(
        mirrorTransform,
        glm::vec3(0.5f, 0.0f, 0.0f)
    );

    mirrorTransform = glm::scale(
        mirrorTransform,
        glm::vec3(0.5f)
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
        std::cout << "Dungeon floor loaded successfully!\n";
    else
        std::cerr << "Failed to load dungeon floor!\n";

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
        std::cout << "Temple walls loaded successfully!\n";
    else
        std::cerr << "Failed to load temple walls!\n";

    // ==========================================
    // LOAD DECORATED TEMPLE PILLARS
    // ==========================================

    Model decoratedPillarModel;

    bool decoratedPillarsLoaded = decoratedPillarModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/pillar_decorated.gltf"
    );

    if (decoratedPillarsLoaded)
        std::cout << "Decorated pillars loaded successfully!\n";
    else
        std::cerr << "Failed to load decorated pillars!\n";

    // ==========================================
    // LOAD TEMPLE DOORWAY
    // ==========================================

    Model doorwayModel;

    bool doorwayLoaded = doorwayModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/wall_doorway.gltf"
    );

    if (doorwayLoaded)
    {
        std::cout << "Temple doorway loaded successfully!\n";
        std::cout << "Doorway mesh count: "
                  << doorwayModel.getMeshCount() << '\n';

        if (doorwayModel.getMeshCount() < 2)
        {
            std::cerr
                << "WARNING: Expected separate door and frame meshes.\n";
        }
    }
    else
    {
        std::cerr << "Failed to load temple doorway!\n";
    }

    // ==========================================
    // TEMPLE DOORWAY TRANSFORM
    // ==========================================

    glm::mat4 doorwayTransform(1.0f);

    doorwayTransform = glm::translate(
        doorwayTransform,
        glm::vec3(0.0f, -0.5f, 6.5f)
    );

    // ==========================================
    // DOOR ANIMATION SETTINGS
    // ==========================================

    float doorCurrentAngle = 0.0f;

    const float doorOpenAngle =
        glm::radians(90.0f);

    const float doorAnimationSpeed =
        glm::radians(90.0f);

    float previousFrameTime =
        static_cast<float>(glfwGetTime());

    // ==========================================
    // GEMSTONE TRANSFORMS
    // ==========================================

    // Real ruby.
    glm::mat4 realRubyTransform(1.0f);

    realRubyTransform = glm::translate(
        realRubyTransform,
        glm::vec3(-2.5f, -0.5f, 0.0f)
    );

    // False ruby.
    glm::mat4 falseRubyTransform(1.0f);

    falseRubyTransform = glm::translate(
        falseRubyTransform,
        glm::vec3(2.5f, -0.5f, 0.0f)
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

        // ======================================
        // CALCULATE FRAME TIME
        // ======================================

        float currentFrameTime =
            static_cast<float>(glfwGetTime());

        float deltaTime =
            currentFrameTime - previousFrameTime;

        previousFrameTime = currentFrameTime;

        // Prevent large animation jumps if the window pauses.
        deltaTime = std::min(deltaTime, 0.05f);

        // ======================================
        // HANDLE EXIT INPUT
        // ======================================

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        bool key1 =
            glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;

        bool key2 =
            glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;

        bool keyR =
            glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

        // ======================================
        // HANDLE PUZZLE INPUT
        // ======================================

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
        // UPDATE DOOR ANIMATION
        // ======================================

        float targetDoorAngle =
            puzzle.isSolved() ? doorOpenAngle : 0.0f;

        float angleStep =
            doorAnimationSpeed * deltaTime;

        if (doorCurrentAngle < targetDoorAngle)
        {
            doorCurrentAngle = std::min(
                doorCurrentAngle + angleStep,
                targetDoorAngle
            );
        }
        else if (doorCurrentAngle > targetDoorAngle)
        {
            doorCurrentAngle = std::max(
                doorCurrentAngle - angleStep,
                targetDoorAngle
            );
        }

        // ======================================
        // CALCULATE ANIMATED DOOR TRANSFORM
        // ======================================

        glm::mat4 animatedDoorTransform =
            doorwayTransform;

        // Move the door's local hinge to the origin.
        animatedDoorTransform = glm::translate(
            animatedDoorTransform,
            glm::vec3(-1.0f, 0.0f, 0.0f)
        );

        // Rotate the door around its vertical hinge.
        animatedDoorTransform = glm::rotate(
            animatedDoorTransform,
            doorCurrentAngle,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Move the door back to its original position.
        animatedDoorTransform = glm::translate(
            animatedDoorTransform,
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

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
                        glm::vec3(1.0f)
                    );
                }
            }
        }

        // ======================================
        // RENDER TEMPLE WALLS
        // ======================================

        if (wallsLoaded)
        {
            // Back wall.
            for (int i = -1; i <= 1; i++)
            {
                glm::mat4 backWall(1.0f);

                backWall = glm::translate(
                    backWall,
                    glm::vec3(i * 4.0f, -0.5f, -6.5f)
                );

                renderer.drawModel(
                    wallModel,
                    backWall,
                    glm::vec3(1.0f)
                );
            }

            // Left and right walls.
            for (int i = -1; i <= 1; i++)
            {
                glm::mat4 leftWall(1.0f);

                leftWall = glm::translate(
                    leftWall,
                    glm::vec3(-6.5f, -0.5f, i * 4.0f)
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

                glm::mat4 rightWall(1.0f);

                rightWall = glm::translate(
                    rightWall,
                    glm::vec3(6.5f, -0.5f, i * 4.0f)
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

            // Front wall sections.
            // Centre reserved for the doorway.
            for (int i : {-1, 1})
            {
                glm::mat4 frontWall(1.0f);

                frontWall = glm::translate(
                    frontWall,
                    glm::vec3(i * 4.0f, -0.5f, 6.5f)
                );

                renderer.drawModel(
                    wallModel,
                    frontWall,
                    glm::vec3(1.0f)
                );
            }
        }

        // ======================================
        // RENDER ANIMATED TEMPLE DOORWAY
        // ======================================

        if (doorwayLoaded)
        {
            if (doorwayModel.getMeshCount() >= 2)
            {
                // Mesh 1: stationary doorway frame.
                renderer.drawModelMesh(
                    doorwayModel,
                    1,
                    doorwayTransform,
                    glm::vec3(1.0f)
                );

                // Mesh 0: independently animated door panel.
                renderer.drawModelMesh(
                    doorwayModel,
                    0,
                    animatedDoorTransform,
                    glm::vec3(1.0f)
                );
            }
            else
            {
                // Fallback if the model does not contain
                // separate frame and door meshes.
                renderer.drawModel(
                    doorwayModel,
                    doorwayTransform,
                    glm::vec3(1.0f)
                );
            }
        }

        // ======================================
        // RENDER FOUR DECORATED TEMPLE PILLARS
        // ======================================

        if (decoratedPillarsLoaded)
        {
            const glm::vec3 decoratedPositions[] =
            {
                // Front pair.
                glm::vec3(-4.2f, -0.5f, 3.0f),
                glm::vec3(4.2f, -0.5f, 3.0f),

                // Back pair.
                glm::vec3(-4.2f, -0.5f, -4.0f),
                glm::vec3(4.2f, -0.5f, -4.0f)
            };

            for (const glm::vec3& position : decoratedPositions)
            {
                glm::mat4 pillarTransform(1.0f);

                pillarTransform = glm::translate(
                    pillarTransform,
                    position
                );

                renderer.drawModel(
                    decoratedPillarModel,
                    pillarTransform,
                    glm::vec3(1.0f)
                );
            }
        }

        // ======================================
        // RENDER REAL RUBY
        // ======================================

        if (puzzle.isSolved())
        {
            renderer.drawGemstone(
                realRubyTransform,
                glm::vec3(0.1f, 1.0f, 0.2f)
            );
        }
        else
        {
            renderer.drawGemstone(
                realRubyTransform,
                glm::vec3(0.9f, 0.05f, 0.12f)
            );
        }

        // ======================================
        // RENDER FALSE RUBY
        // ======================================

        renderer.drawGemstone(
            falseRubyTransform,
            glm::vec3(0.6f, 0.1f, 0.9f)
        );

        // ======================================
        // RENDER 3D MIRROR
        // ======================================

        if (mirrorLoaded)
        {
            renderer.drawModel(
                mirrorModel,
                mirrorTransform,
                glm::vec3(0.8f)
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
    decoratedPillarModel.cleanup();
    doorwayModel.cleanup();
    mirrorModel.cleanup();

    renderer.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
