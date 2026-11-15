cmake_minimum_required(VERSION 3.20)

project(ReflectionsOfTheTomb
    VERSION 0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --------------------------------------------------
# GLFW
# --------------------------------------------------

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# --------------------------------------------------
# GLM
# --------------------------------------------------

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# --------------------------------------------------
# GLAD
# --------------------------------------------------

# --------------------------------------------------
# GLAD
# --------------------------------------------------

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    SOURCE_SUBDIR cmake
)

FetchContent_MakeAvailable(glad)

glad_add_library(
    glad_gl_core_33
    STATIC
    REPRODUCIBLE
    API gl:core=3.3
)
# --------------------------------------------------
# Executable
# --------------------------------------------------


add_executable(
    reflections_of_the_tomb
    src/main.cpp
    src/rendering/Renderer.cpp
    src/rendering/Shader.cpp
    src/rendering/Model.cpp
    src/mirror/Mirror.cpp
    src/mirror/MirrorSystem.cpp

    # Gameplay components
    src/gameplay/Ruby.cpp
    src/gameplay/Trap.cpp
    src/gameplay/Door.cpp
    src/gameplay/PuzzleSystem.cpp
    src/gameplay/RubyCollision.cpp
    
)

target_include_directories(
    reflections_of_the_tomb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    reflections_of_the_tomb
    PRIVATE
        glfw
        glad_gl_core_33
        glm::glm
)

# --------------------------------------------------
# Beam reflection tests
# --------------------------------------------------

enable_testing()

add_executable(
    reflection_math_test
    tests/reflection_math_test.cpp
    src/beam/BeamSystem.cpp
)

target_include_directories(
    reflection_math_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    reflection_math_test
    PRIVATE
        glm::glm
)

add_test(
    NAME reflection_math
    COMMAND reflection_math_test
)
# --------------------------------------------------
# Ray-plane intersection tests
# --------------------------------------------------

add_executable(
    ray_plane_test
    tests/ray_plane_test.cpp
    src/physics/Intersections.cpp
)

target_include_directories(
    ray_plane_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ray_plane_test
    PRIVATE
        glm::glm
)

add_test(
    NAME ray_plane
    COMMAND ray_plane_test
)
# --------------------------------------------------
# Single-mirror integration test
# --------------------------------------------------

add_executable(
    single_mirror_test
    tests/single_mirror_test.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
)

target_include_directories(
    single_mirror_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    single_mirror_test
    PRIVATE
        glm::glm
)

add_test(
    NAME single_mirror
    COMMAND single_mirror_test
)
# --------------------------------------------------
# Level / LevelManager demo and test (Zahra)
# --------------------------------------------------

add_executable(
    level_demo
    src/world/level_demo.cpp
    src/world/Level.cpp
    src/world/LevelManager.cpp
)

target_include_directories(
    level_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_test(
    NAME level_demo
    COMMAND level_demo
)


# Puzzle and beam integration test


add_executable(
    reflected_ruby_test
    tests/reflected_ruby_test.cpp
    src/beam/BeamSystem.cpp
    src/gameplay/PuzzleSystem.cpp
    src/gameplay/Ruby.cpp
    src/gameplay/Trap.cpp
    src/gameplay/Door.cpp
    src/gameplay/RubyCollision.cpp
)

target_include_directories(
    reflected_ruby_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    reflected_ruby_test
    PRIVATE
        glm::glm
)

add_test(
    NAME reflected_ruby
    COMMAND reflected_ruby_test
)

# --------------------------------------------------
# Finite mirror intersection tests (Peter)
# --------------------------------------------------

add_executable(
    ray_segment_test
    tests/ray_segment_test.cpp
    src/physics/Intersections.cpp
)

target_include_directories(
    ray_segment_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ray_segment_test
    PRIVATE
        glm::glm
)

add_test(
    NAME ray_segment
    COMMAND ray_segment_test
)
# --------------------------------------------------
# Multi-mirror beam tracing tests (Peter)
# --------------------------------------------------

add_executable(
    multi_mirror_test
    tests/multi_mirror_test.cpp
    src/beam/BeamTracer.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
    src/mirror/Mirror.cpp
)

target_include_directories(
    multi_mirror_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    multi_mirror_test
    PRIVATE
        glm::glm
)

add_test(
    NAME multi_mirror
    COMMAND multi_mirror_test
)
# --------------------------------------------------
# Wall collision and nearest-hit tests (Peter)
# --------------------------------------------------

add_executable(
    wall_collision_test
    tests/wall_collision_test.cpp
    src/beam/BeamTracer.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
    src/mirror/Mirror.cpp
)

target_include_directories(
    wall_collision_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    wall_collision_test
    PRIVATE
        glm::glm
)

add_test(
    NAME wall_collision
    COMMAND wall_collision_test
)
# --------------------------------------------------
# Beam and puzzle integration tests
# --------------------------------------------------

add_executable(
    beam_puzzle_integration_test
    tests/beam_puzzle_integration_test.cpp

    src/beam/BeamTracer.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
    src/mirror/Mirror.cpp

    src/gameplay/BeamPuzzleBridge.cpp
    src/gameplay/PuzzleSystem.cpp
    src/gameplay/RubyCollision.cpp
    src/gameplay/Ruby.cpp
    src/gameplay/Door.cpp
    src/gameplay/Trap.cpp
)

target_include_directories(
    beam_puzzle_integration_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    beam_puzzle_integration_test
    PRIVATE
        glm::glm
)

add_test(
    NAME beam_puzzle_integration
    COMMAND beam_puzzle_integration_test
)
# --------------------------------------------------
# Player and beam collision tests (Peter)
# --------------------------------------------------

add_executable(
    beam_player_collision_test
    tests/beam_player_collision_test.cpp
    src/physics/BeamPlayerCollision.cpp
    src/beam/BeamTracer.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
    src/mirror/Mirror.cpp
)

target_include_directories(
    beam_player_collision_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    beam_player_collision_test
    PRIVATE
        glm::glm
)

add_test(
    NAME beam_player_collision
    COMMAND beam_player_collision_test
)
# --------------------------------------------------
# Interactive beam visualization demo (Peter)
# --------------------------------------------------

add_executable(
    beam_visual_demo
    src/beam/beam_visual_demo.cpp
    src/beam/BeamTracer.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
    src/mirror/Mirror.cpp
    src/gameplay/RubyCollision.cpp
    src/rendering/Shader.cpp
)

target_include_directories(
    beam_visual_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    beam_visual_demo
    PRIVATE
        glfw
        glad_gl_core_33
        glm::glm
)
# --------------------------------------------------
# Ruby absorption and puzzle activation tests
# --------------------------------------------------

add_executable(
    ruby_absorption_test
    tests/ruby_absorption_test.cpp
    src/beam/BeamTracer.cpp
    src/beam/BeamSystem.cpp
    src/physics/Intersections.cpp
    src/mirror/Mirror.cpp
    src/gameplay/BeamPuzzleBridge.cpp
    src/gameplay/PuzzleSystem.cpp
    src/gameplay/RubyCollision.cpp
    src/gameplay/Ruby.cpp
    src/gameplay/Door.cpp
    src/gameplay/Trap.cpp
)

target_include_directories(
    ruby_absorption_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ruby_absorption_test
    PRIVATE
        glm::glm
)

add_test(
    NAME ruby_absorption
    COMMAND ruby_absorption_test
)