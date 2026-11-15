#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/Renderer.h"
#include "rendering/Model.h"
#include "gameplay/PuzzleSystem.h"
#include "beam/BeamTracer.h"
#include "gameplay/BeamPuzzleBridge.h"
#include "mirror/Mirror.h"
#include "rendering/BeamRenderer.h"

#include <algorithm>
#include <vector>

#include "player/PlayerMovement3D.h"

void framebufferSizeCallback(
    GLFWwindow *window, int width, int height)
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
        GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(
        1280, 720,
        "Reflections of the Tomb - Puzzle Demo",
        nullptr, nullptr);

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
        window, framebufferSizeCallback);

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
    BeamRenderer beamRenderer;

    if (!beamRenderer.initialize())
    {
        std::cerr << "Beam renderer initialization failed!\n";

        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    // Load the 3D mirror model.
    Model mirrorModel;
    bool mirrorLoaded = mirrorModel.load(
        "assets/models/mirrors/mirror_complete.glb");

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
        glm::vec3(0.5f, 0.0f, 0.0f));

    mirrorTransform = glm::scale(
        mirrorTransform,
        glm::vec3(0.5f, 0.5f, 0.5f));

    PuzzleSystem puzzle;

    // Beam emitter.
    const glm::vec2 beamOrigin(0.0f, 0.0f);
    const glm::vec2 beamDirection(1.0f, 0.0f);
    const float beamHeight = 1.0f;

    // Two mirrors from Peter's working demo.
    // Mirror 1 starts misaligned.
    std::vector<Mirror> mirrors;

    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f, 20.0f, -180.0f, 180.0f);

    mirrors.emplace_back(
        glm::vec2(5.0f, 5.0f),
        4.0f, 135.0f, -180.0f, 180.0f);

    // Real and false rubies in world space.
    std::vector<RubyTarget> rubies = {
        {glm::vec3(-2.0f, 1.0f, 5.0f), 0.3f, true},
        {glm::vec3(2.0f, 1.0f, 3.0f), 0.3f, false}
    };

    // Same walls as the beam demo.
    std::vector<WallSegment> walls = {
        {{-3.0f, -1.5f}, {9.0f, -1.5f}},
        {{9.0f, -1.5f}, {9.0f, 6.5f}},
        {{9.0f, 6.5f}, {-3.0f, 6.5f}},
        {{-3.0f, 6.5f}, {-3.0f, -1.5f}}};

    std::size_t selectedMirror = 0;
    double previousTime = glfwGetTime();
    bool previousSolved = false;

    std::cout << "=== Reflections of the Tomb ===\n";
    std::cout << "1 / 2 - Select mirror\n";
    std::cout << "Q / E - Rotate selected mirror\n";
    std::cout << "R - Reset puzzle and mirrors\n";
    std::cout << "ESC - Exit\n";

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

CharacterAnimationState animationState =
    CharacterAnimationState::Idle;
bool previousE = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = std::min(
            static_cast<float>(now - previousTime),
            0.05f);
        previousTime = now;
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

        player.update(
            inputX,
            inputZ,
            dt,
            roomBounds,
            obstacles
        );

        // Select which mirror to rotate.
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            selectedMirror = 0;

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            selectedMirror = 1;

        // Rotate the selected mirror.
        const float rotationSpeed = 50.0f;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            mirrors[selectedMirror].rotateLeft(rotationSpeed * dt);

        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
            mirrors[selectedMirror].rotateRight(rotationSpeed * dt);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        bool keyR = glfwGetKey(
                        window, GLFW_KEY_R) == GLFW_PRESS;

        if (keyR && !previousR)
        {
            mirrors[0].setAngle(20.0f);
            mirrors[1].setAngle(135.0f);

            puzzle.reset();
            previousSolved = false;

            std::cout << "Puzzle reset!\n";
        }

        previousR = keyR;

        BeamTraceResult beamResult = BeamTracer::traceWithRubies(
            beamOrigin,
            beamDirection,
            mirrors,
            rubies,
            4,
            30.0f,
            beamHeight,
            walls);

        // Only activate the puzzle when the real ruby is hit.
        if (beamResult.hitRubyIndex >= 0 && !puzzle.isSolved())
        {
            BeamPuzzleBridge::processAbsorbingTrace(
                beamResult,
                rubies,
                puzzle);
        }

        // Print only when the puzzle becomes solved.
        if (puzzle.isSolved() && !previousSolved)
        {
            std::cout << "REAL RUBY ACTIVATED!\n";
            std::cout << "Door opened! Traps deactivated!\n";
        }

        previousSolved = puzzle.isSolved();
        renderer.beginFrame();

        // Temporary HUD state indicators (not world-space ruby positions).
        // Real ruby indicator: red until puzzle solved, green when solved.
        if (puzzle.isSolved())
        {
            renderer.drawRectangle(
                -0.7f, 0.2f, 0.3f, 0.3f,
                0.1f, 1.0f, 0.2f);
        }
        else
        {
            renderer.drawRectangle(
                -0.7f, 0.2f, 0.3f, 0.3f,
                0.9f, 0.1f, 0.2f);
        }

        // False ruby indicator: purple (temporary state indicator).
        renderer.drawRectangle(
            -0.2f, 0.2f, 0.3f, 0.3f,
            0.6f, 0.1f, 0.8f);

        // Door: brown when closed, green when open.
        if (puzzle.isDoorOpen())
        {
            renderer.drawRectangle(
                0.5f, -0.2f, 0.25f, 0.7f,
                0.1f, 0.8f, 0.2f);
        }
        else
        {
            renderer.drawRectangle(
                0.5f, -0.2f, 0.25f, 0.7f,
                0.5f, 0.3f, 0.1f);
        }

        // Trap: red when active, gray when safe.
        if (puzzle.areTrapsActive())
        {
            renderer.drawRectangle(
                -0.7f, -0.7f, 0.8f, 0.2f,
                1.0f, 0.1f, 0.1f);
        }
        else
        {
            renderer.drawRectangle(
                -0.7f, -0.7f, 0.8f, 0.2f,
                0.4f, 0.4f, 0.4f);
        }

        // Render the 3D mirror (debug/placeholder model, independent of physics mirrors).
        if (mirrorLoaded)
        {
            renderer.drawModel(
                mirrorModel,
                mirrorTransform,
                glm::vec3(0.8f, 0.8f, 0.8f));
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

        // Draw mirrors first so the light beam renders on top at reflection points.
        beamRenderer.drawMirrors(mirrors, selectedMirror);
        beamRenderer.draw(beamResult.path);

        glfwSwapBuffers(window);
    }

    // Clean up while the OpenGL context is still active.
    beamRenderer.shutdown();
    mirrorModel.cleanup();
    renderer.shutdown();
    rangerModel.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}