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