#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/Renderer.h"
#include "rendering/Model.h"
#include "gameplay/PuzzleSystem.h"

#include "player/PlayerMovement3D.h"

void framebufferSizeCallback(
    GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
enum class CharacterAnimationState
{
    Idle,
    Walking
};
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
    // Load the Ranger character.
Model rangerModel;

bool rangerLoaded = rangerModel.load(
    "assets/models/characters/ranger/Ranger.glb"
);

if (rangerLoaded)
{
    std::cout << "Ranger model loaded successfully!\n";
}
else
{
    std::cerr << "Failed to load Ranger model.\n";
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

    // 3D Ranger movement
player3d::PlayerMovement3D player(
    {-0.8f, 0.0f}, // Starting position: X, Z
    2.5f,          // Movement speed
    0.25f          // Collision radius
);

// Temporary room boundaries.
// These will later come from Zahra's 3D chamber.
player3d::RoomBoundsXZ roomBounds{
    -4.0f, 4.0f,
    -4.0f, 4.0f
};

// No obstacles yet; we'll add actual chamber collision geometry later.
std::vector<player3d::ObstacleXZ> obstacles;

// Mirror position used for interaction testing.
player3d::PositionXZ mirrorPosition{0.5f, 0.0f};

double previousTime = glfwGetTime();
CharacterAnimationState animationState =
    CharacterAnimationState::Idle;
bool previousE = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Calculate time since the previous frame.
double currentTime = glfwGetTime();

float deltaTime = static_cast<float>(
    currentTime - previousTime
);

previousTime = currentTime;

// Prevent large movement jumps after pauses.
if (deltaTime > 0.05f)
{
    deltaTime = 0.05f;
}

// Read WASD input.
float inputX = 0.0f;
float inputZ = 0.0f;

if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    inputZ -= 1.0f;

if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    inputZ += 1.0f;

if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    inputX -= 1.0f;

if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    inputX += 1.0f;

// Move the Ranger using our tested movement controller.
player.update(
    inputX,
    inputZ,
    deltaTime,
    roomBounds,
    obstacles
);
// Determine which animation the Ranger should play.
CharacterAnimationState newState =
    player.isMoving()
        ? CharacterAnimationState::Walking
        : CharacterAnimationState::Idle;

// Only respond when the animation state changes.
if (newState != animationState)
{
    animationState = newState;

    if (animationState == CharacterAnimationState::Walking)
    {
        std::cout << "Animation: Walking\n";
    }
    else
    {
        std::cout << "Animation: Idle\n";
    }
}
// Test whether the player can interact with the mirror.
bool keyE = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

if (keyE && !previousE)
{
    if (player.canInteract(mirrorPosition, 1.0f))
    {
        std::cout << "Mirror interaction available!\n";
    }
    else
    {
        std::cout << "Move closer to the mirror.\n";
    }
}

previousE = keyE;

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
            player.reset();
            std::cout << "Puzzle and Ranger reset!\n";
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
// Render the Ranger at its updated 3D position.
if (rangerLoaded)
{
    player3d::PositionXZ position = player.position();

    glm::mat4 rangerTransform(1.0f);

    rangerTransform = glm::translate(
        rangerTransform,
        glm::vec3(position.x, 0.0f, position.z)
    );
    // Rotate the Ranger to face its movement direction.
player3d::PositionXZ facing = player.facing();

float angle = std::atan2(facing.x, facing.z);

rangerTransform = glm::rotate(
    rangerTransform,
    angle,
    glm::vec3(0.0f, 1.0f, 0.0f)
);

    rangerTransform = glm::scale(
        rangerTransform,
        glm::vec3(0.5f)
    );

    renderer.drawModel(
        rangerModel,
        rangerTransform,
        glm::vec3(0.75f, 0.85f, 0.65f)
    );
}

        glfwSwapBuffers(window);
    }

    // Clean up while the OpenGL context is still active.
    mirrorModel.cleanup();
    renderer.shutdown();
    rangerModel.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}