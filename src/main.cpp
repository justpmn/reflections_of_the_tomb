
#include <iostream>
#include <algorithm>
#include <vector>

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
#include "player/PlayerMovement3D.h"

// Zahra: showcase mode for videos.
// Showcase = draw the data-driven level (assets/levels/level01.txt) with the repo's
//            3D assets INSTEAD of the hard-coded temple room below, hide that room's
//            puzzle props (rubies, mirrors, emitter, beam), and use an orbit camera
//            that frames the whole level:
//              Left/Right = orbit, Up/Down = raise/lower, +/- = zoom, C = reset camera.
// Otherwise = the temple room demo exactly as on main (fixed camera).
// kShowcaseLevelOnly is the mode the game starts in; F1 toggles at runtime.
// The room's code is unchanged; it is only wrapped in if (!showcaseMode).
#include "world/LevelManager.h"
#include "world/LevelScene.h"
#include "world/LevelSceneRenderer.h"
#include "world/ShowcaseCamera.h"
constexpr bool kShowcaseLevelOnly = true;

// ==========================================
// WINDOW RESIZE CALLBACK
// ==========================================

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
}

// ==========================================
// MAIN
// ==========================================

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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    int width = 0;
    int height = 0;

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
    // LOAD MODELS
    // ==========================================

    Model mirrorModel;

    const bool mirrorLoaded = mirrorModel.load(
        "assets/models/mirrors/mirror_complete.glb"
    );

    if (mirrorLoaded)
        std::cout << "Mirror model loaded successfully!\n";
    else
        std::cerr << "Failed to load mirror model!\n";

    Model floorModel;

    const bool floorLoaded = floorModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/floor_tile_large.gltf"
    );

    if (floorLoaded)
        std::cout << "Dungeon floor loaded successfully!\n";
    else
        std::cerr << "Failed to load dungeon floor!\n";

    Model wallModel;

    const bool wallsLoaded = wallModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/wall.gltf"
    );

    if (wallsLoaded)
        std::cout << "Temple walls loaded successfully!\n";
    else
        std::cerr << "Failed to load temple walls!\n";

    Model decoratedPillarModel;

    const bool decoratedPillarsLoaded = decoratedPillarModel.load(
        "assets/vendor/kaykit_dungeon/"
        "KayKit_Dungeon_Pack_1.1_FREE/"
        "Assets/gltf/pillar_decorated.gltf"
    );

    if (decoratedPillarsLoaded)
        std::cout << "Decorated pillars loaded successfully!\n";
    else
        std::cerr << "Failed to load decorated pillars!\n";

    Model doorwayModel;

    const bool doorwayLoaded = doorwayModel.load(
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
            std::cerr << "WARNING: Expected separate door and frame meshes.\n";
    }
    else
    {
        std::cerr << "Failed to load temple doorway!\n";
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

    // Zahra: level scene (drawn in showcase mode). Placed over the temple room's
    // +-6.5 footprint with its floor top at y = 0, where the Ranger stands.
    // Loaded up front so F1 can switch to it at any time.
    world::LevelManager levels({"assets/levels/level01.txt",
                                "assets/levels/level02.txt",
                                "assets/levels/level03.txt"});
    world::LevelScene levelScene;
    world::LevelSceneRenderer levelSceneRenderer;
    glm::mat4 levelToWorld(1.0f);
    bool levelReady = false;

    {
        if (levels.loadFirst())
        {
            // The camera starts south (+Z) of the level, so lower the walls
            // whose front faces away from it (rebuilt as the camera moves).
            world::LevelSceneOptions sceneOptions;
            sceneOptions.cutawayToward = glm::vec2(0.0f, 1.0f);
            levelScene = world::buildLevelScene(levels.current(), sceneOptions);
            levelSceneRenderer.load();
            levelToWorld = world::levelToArea(
                levels.current(), glm::vec2(0.0f), 6.5f);
            levelReady = true;
        }
        else
        {
            for (const auto& e : levels.errors())
                std::cerr << e << '\n';
        }
    }

    // Zahra: showcase orbit camera (fov 40 / aspect match Renderer::initialize).
    world::ShowcaseCamera showcaseCamera = world::ShowcaseCamera::fitLevel(
        levels.current(), levelToWorld, 40.0f, 1280.0f / 720.0f);
    glm::vec2 builtCutaway(0.0f, 1.0f);   // cutaway the current levelScene was built with

    // Zahra: runtime showcase toggle (F1). Joe's fixed camera is restored when
    // switching back to the temple room.
    bool showcaseMode = kShowcaseLevelOnly && levelReady;
    bool previousF1 = false;
    const glm::mat4 defaultView = renderer.getViewMatrix();
    std::cout << "F1 - Toggle level showcase / temple room\n";

    // ==========================================
    // DOOR TRANSFORM AND ANIMATION
    // ==========================================

    glm::mat4 doorwayTransform(1.0f);

    doorwayTransform = glm::translate(
        doorwayTransform,
        glm::vec3(0.0f, -0.5f, 6.5f)
    );

    float doorCurrentAngle = 0.0f;

    const float doorOpenAngle = glm::radians(90.0f);
    const float doorAnimationSpeed = glm::radians(90.0f);

    // ==========================================
    // RUBY TRANSFORMS
    // ==========================================

    const glm::vec3 realRubyPosition(-2.5f, -0.5f, 0.0f);
    const glm::vec3 falseRubyPosition(1.5f, -0.5f, 2.0f);

    glm::mat4 realRubyTransform(1.0f);

    realRubyTransform = glm::translate(
        realRubyTransform,
        realRubyPosition
    );

    glm::mat4 falseRubyTransform(1.0f);

    falseRubyTransform = glm::translate(
        falseRubyTransform,
        falseRubyPosition
    );

    // ==========================================
    // INITIALIZE PUZZLE AND BEAM
    // ==========================================

    PuzzleSystem puzzle;

    const glm::vec2 beamOrigin(-0.5f, -5.0f);
    const glm::vec2 beamDirection(1.0f, 0.0f);

    const float beamHeight = 1.25f;

    // ==========================================
    // MIRROR CONFIGURATION
    // ==========================================

    const float mirror1StartAngle = 20.0f;
    const float mirror2StartAngle = 135.0f;

    std::vector<Mirror> mirrors;

    mirrors.emplace_back(
        glm::vec2(2.5f, -5.0f),
        4.0f,
        mirror1StartAngle,
        -180.0f,
        180.0f
    );

    mirrors.emplace_back(
        glm::vec2(2.5f, 0.0f),
        4.0f,
        mirror2StartAngle,
        -180.0f,
        180.0f
    );

    // ==========================================
    // RUBY COLLISION TARGETS
    // ==========================================

    const std::vector<RubyTarget> rubies = {
        {glm::vec3(-2.5f, 1.25f, 0.0f), 0.45f, true},
        {glm::vec3(1.5f, 1.25f, 2.0f), 0.45f, false}
    };

    // ==========================================
    // TEMPLE WALL BOUNDARIES
    // ==========================================

    const std::vector<WallSegment> walls = {
        {{-6.5f, -6.5f}, { 6.5f, -6.5f}},
        {{ 6.5f, -6.5f}, { 6.5f,  6.5f}},
        {{ 6.5f,  6.5f}, {-6.5f,  6.5f}},
        {{-6.5f,  6.5f}, {-6.5f, -6.5f}}
    };

    std::size_t selectedMirror = 0;

    bool previousR = false;
    bool previousSolved = false;

    float previousFrameTime = static_cast<float>(glfwGetTime());
    double previousTime = glfwGetTime();   // Ranger movement timer

    std::cout << "\n=== Reflections of the Tomb ===\n";
    std::cout << "1 / 2 - Select mirror\n";
   std::cout << "WASD - Move Ranger\n";
std::cout << "Q / T - Rotate selected mirror\n";
std::cout << "E - Check mirror interaction\n";
std::cout << "R - Reset puzzle and Ranger\n";
    std::cout << "ESC - Exit\n";

    // ==========================================
    // MAIN GAME LOOP
    // ==========================================

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

        const float currentFrameTime =
            static_cast<float>(glfwGetTime());

        const float deltaTime = std::min(
            currentFrameTime - previousFrameTime,
            0.05f
        );

        previousFrameTime = currentFrameTime;

        // ======================================
        // INPUT
        // ======================================

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            selectedMirror = 0;

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            selectedMirror = 1;

        const float rotationSpeed = 50.0f;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            mirrors[selectedMirror].rotateLeft(
                rotationSpeed * deltaTime
            );
        }

        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        {
            mirrors[selectedMirror].rotateRight(
                rotationSpeed * deltaTime
            );
        }

        // ======================================
        // RESET PUZZLE
        // ======================================

        const bool keyR =
            glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

        if (keyR && !previousR)
        {
            mirrors[0].setAngle(mirror1StartAngle);
            mirrors[1].setAngle(mirror2StartAngle);

            puzzle.reset();

            doorCurrentAngle = 0.0f;
            previousSolved = false;

            std::cout << "Puzzle reset!\n";
        }

        previousR = keyR;

        // ======================================
        // TRACE BEAM
        // ======================================

        const BeamTraceResult beamResult =
            BeamTracer::traceWithRubies(
                beamOrigin,
                beamDirection,
                mirrors,
                rubies,
                4,
                30.0f,
                beamHeight,
                walls
            );

        // ======================================
        // UPDATE PUZZLE
        // ======================================

        if (beamResult.hitRubyIndex >= 0 && !puzzle.isSolved())
        {
            BeamPuzzleBridge::processAbsorbingTrace(
                beamResult,
                rubies,
                puzzle
            );
        }

        if (puzzle.isSolved() && !previousSolved)
        {
            std::cout << "REAL RUBY ACTIVATED!\n";
            std::cout << "Door opened! Traps deactivated!\n";
        }

        previousSolved = puzzle.isSolved();

        // ======================================
        // DOOR ANIMATION
        // ======================================

        const float targetDoorAngle =
            puzzle.isSolved() ? doorOpenAngle : 0.0f;

        const float angleStep =
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

        glm::mat4 animatedDoorTransform = doorwayTransform;

        animatedDoorTransform = glm::translate(
            animatedDoorTransform,
            glm::vec3(-1.0f, 0.0f, 0.0f)
        );

        animatedDoorTransform = glm::rotate(
            animatedDoorTransform,
            doorCurrentAngle,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        animatedDoorTransform = glm::translate(
            animatedDoorTransform,
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        // Zahra: F1 toggles between the level showcase and the temple room.
        const bool keyF1 = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
        if (keyF1 && !previousF1 && levelReady)
        {
            showcaseMode = !showcaseMode;
            if (!showcaseMode)
                renderer.setViewMatrix(defaultView);
            std::cout << (showcaseMode ? "Showcase: level\n" : "Showcase: temple room\n");
        }
        previousF1 = keyF1;

        // Zahra: showcase camera controls + cutaway that follows the camera.
        if (showcaseMode)
        {
            auto down = [&](int key)
            { return glfwGetKey(window, key) == GLFW_PRESS; };

            float yawInput = 0.0f, pitchInput = 0.0f;
            if (down(GLFW_KEY_LEFT))  yawInput -= 1.0f;
            if (down(GLFW_KEY_RIGHT)) yawInput += 1.0f;
            if (down(GLFW_KEY_UP))    pitchInput += 1.0f;
            if (down(GLFW_KEY_DOWN))  pitchInput -= 1.0f;
            showcaseCamera.orbit(60.0f * yawInput * deltaTime,
                                 40.0f * pitchInput * deltaTime);

            if (down(GLFW_KEY_EQUAL) || down(GLFW_KEY_KP_ADD))
                showcaseCamera.zoomBy(std::exp(-1.0f * deltaTime));
            if (down(GLFW_KEY_MINUS) || down(GLFW_KEY_KP_SUBTRACT))
                showcaseCamera.zoomBy(std::exp(1.0f * deltaTime));
            if (down(GLFW_KEY_C))
                showcaseCamera.reset();

            renderer.setViewMatrix(showcaseCamera.view());

            // Cut away walls between the camera and the room only when the
            // camera is too low to see over them; rebuild when that changes.
            glm::vec2 wantCutaway(0.0f);
            if (world::cutawayNeeded(levels.current(), showcaseCamera.pitchDeg))
                wantCutaway = showcaseCamera.towardCamera();

            if (glm::length(wantCutaway - builtCutaway) > 1e-3f)
            {
                world::LevelSceneOptions sceneOptions;
                sceneOptions.cutawayToward = wantCutaway;
                levelScene = world::buildLevelScene(levels.current(), sceneOptions);
                builtCutaway = wantCutaway;
            }
        }

        // ======================================
        // BEGIN FRAME
        // ======================================

        renderer.beginFrame();

        // Zahra: showcase mode draws the level instead of the temple room.
        if (showcaseMode)
            levelSceneRenderer.draw(renderer, levelScene, levelToWorld);

        if (!showcaseMode) // Zahra: showcase flag, temple room + puzzle props unchanged
        {
        // ======================================
        // FLOOR
        // ======================================

        if (floorLoaded)
        {
            for (int x = -1; x <= 1; ++x)
            {
                for (int z = -1; z <= 1; ++z)
                {
                    glm::mat4 tileTransform(1.0f);

                    tileTransform = glm::translate(
                        tileTransform,
                        glm::vec3(
                            static_cast<float>(x) * 4.0f,
                            -0.5f,
                            static_cast<float>(z) * 4.0f
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
        // TEMPLE WALLS
        // ======================================

        if (wallsLoaded)
        {
            // Back wall.
            for (int i = -1; i <= 1; ++i)
            {
                glm::mat4 backWall(1.0f);

                backWall = glm::translate(
                    backWall,
                    glm::vec3(
                        static_cast<float>(i) * 4.0f,
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

            // Left and right walls.
            for (int i = -1; i <= 1; ++i)
            {
                glm::mat4 leftWall(1.0f);

                leftWall = glm::translate(
                    leftWall,
                    glm::vec3(
                        -6.5f,
                        -0.5f,
                        static_cast<float>(i) * 4.0f
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

                glm::mat4 rightWall(1.0f);

                rightWall = glm::translate(
                    rightWall,
                    glm::vec3(
                        6.5f,
                        -0.5f,
                        static_cast<float>(i) * 4.0f
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

            // Front wall sections; centre reserved for doorway.
            for (int i : {-1, 1})
            {
                glm::mat4 frontWall(1.0f);

                frontWall = glm::translate(
                    frontWall,
                    glm::vec3(
                        static_cast<float>(i) * 4.0f,
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
        // DOORWAY
        // ======================================

        if (doorwayLoaded)
        {
            if (doorwayModel.getMeshCount() >= 2)
            {
                renderer.drawModelMesh(
                    doorwayModel,
                    1,
                    doorwayTransform,
                    glm::vec3(1.0f)
                );

                renderer.drawModelMesh(
                    doorwayModel,
                    0,
                    animatedDoorTransform,
                    glm::vec3(1.0f)
                );
            }
            else
            {
                renderer.drawModel(
                    doorwayModel,
                    doorwayTransform,
                    glm::vec3(1.0f)
                );
            }
        }

        // ======================================
        // DECORATED PILLARS
        // ======================================

        if (decoratedPillarsLoaded)
        {
            const glm::vec3 decoratedPositions[] = {
                {-4.2f, -0.5f,  3.0f},
                { 4.2f, -0.5f,  3.0f},
                {-4.2f, -0.5f, -4.0f},
                { 4.2f, -0.5f, -4.0f}
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
        // RUBIES
        // ======================================

        renderer.drawGemstone(
            realRubyTransform,
            puzzle.isSolved()
                ? glm::vec3(0.1f, 1.0f, 0.2f)
                : glm::vec3(0.9f, 0.05f, 0.12f)
        );

        renderer.drawGemstone(
            falseRubyTransform,
            glm::vec3(0.6f, 0.1f, 0.9f)
        );

        // ======================================
        // MIRROR ASSEMBLIES
        // ======================================

        if (mirrorLoaded)
        {
            for (std::size_t i = 0; i < mirrors.size(); ++i)
            {
                const glm::vec2 centre = mirrors[i].getCenter();

                // --------------------------------------
                // BUILT-IN STONE PEDESTAL
                // --------------------------------------

                glm::mat4 baseTransform(1.0f);

                baseTransform = glm::translate(
                    baseTransform,
                    glm::vec3(centre.x, -0.5f, centre.y)
                );

                baseTransform = glm::scale(
                    baseTransform,
                    glm::vec3(0.5f)
                );

                for (unsigned int meshIndex = 22;
                     meshIndex <= 31;
                     ++meshIndex)
                {
                    renderer.drawModelMesh(
                        mirrorModel,
                        meshIndex,
                        baseTransform,
                        glm::vec3(0.72f, 0.72f, 0.76f)
                    );
                }

                // --------------------------------------
                // MIRROR FRAME AND REFLECTIVE SURFACE
                // --------------------------------------

                glm::mat4 mirrorTransform(1.0f);

                // Place the reflective surface centre at the
                // same world position used by the beam physics.
                mirrorTransform = glm::translate(
                    mirrorTransform,
                    glm::vec3(
                        centre.x,
                        beamHeight,
                        centre.y
                    )
                );

                // Rotate the assembly using the physics angle.
                const float visualAngle = -mirrors[i].getAngle();

                mirrorTransform = glm::rotate(
                    mirrorTransform,
                    glm::radians(visualAngle),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                // Move the model origin so that the reflective
                // surface's local centre aligns with the beam.
                // These are the reflective surface local coordinates
                // identified in the supplied model.
                mirrorTransform = glm::scale(
                    mirrorTransform,
                    glm::vec3(0.65f)
                );

                mirrorTransform = glm::translate(
                    mirrorTransform,
                    glm::vec3(0.0f, -2.0295f, -0.1858f)
                );

                // Draw the mirror assembly meshes.
                for (unsigned int meshIndex = 0;
                     meshIndex <= 21;
                     ++meshIndex)
                {
                    renderer.drawModelMesh(
                        mirrorModel,
                        meshIndex,
                        mirrorTransform,
                        i == selectedMirror
                            ? glm::vec3(1.0f, 0.85f, 0.45f)
                            : glm::vec3(0.8f)
                    );
                }
            }
        }
        } // end showcase flag (temple room + puzzle props)
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

        if (!showcaseMode) // Zahra: showcase flag, emitter + beam unchanged
        {
        // ======================================
        // VISIBLE BEAM EMITTER
        // ======================================

        glm::mat4 emitterTransform(1.0f);

        emitterTransform = glm::translate(
            emitterTransform,
            glm::vec3(
                beamOrigin.x,
                beamHeight,
                beamOrigin.y
            )
        );

        emitterTransform = glm::scale(
            emitterTransform,
            glm::vec3(0.35f)
        );

        renderer.drawGemstone(
            emitterTransform,
            glm::vec3(1.0f, 0.72f, 0.12f)
        );

        // ======================================
        // EMISSIVE BEAM
        // ======================================

        renderer.drawBeamSegments(
            beamResult.path,
            glm::vec3(1.0f, 0.85f, 0.25f),
            0.14f
        );
        } // end showcase flag (emitter + beam)

        // ======================================
        // END FRAME
        // ======================================

        glfwSwapBuffers(window);
    }

    // ==========================================
    // CLEANUP
    // ==========================================

    floorModel.cleanup();
    wallModel.cleanup();
    decoratedPillarModel.cleanup();
    doorwayModel.cleanup();
    mirrorModel.cleanup();
    levelSceneRenderer.cleanup();

    renderer.shutdown();
    rangerModel.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
