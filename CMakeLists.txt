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
    src/mirror/Mirror.cpp
    src/mirror/MirrorSystem.cpp
)

target_link_libraries(
    reflections_of_the_tomb
    PRIVATE
        glfw
        glad_gl_core_33
        glm::glm
)

# --------------------------------------------------
# Mirror transformation tests
# --------------------------------------------------

enable_testing()

add_executable(
    mirror_test
    tests/mirror_test.cpp
    src/mirror/Mirror.cpp
    src/mirror/MirrorSystem.cpp
)

target_include_directories(
    mirror_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    mirror_test
    PRIVATE
        glm::glm
)

add_test(
    NAME mirror_transformations
    COMMAND mirror_test
)

# --------------------------------------------------
# Standalone Mirror Demo
# --------------------------------------------------

add_executable(
    mirror_demo
    demos/mirror_demo.cpp
    src/mirror/Mirror.cpp
)

target_include_directories(
    mirror_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    mirror_demo
    PRIVATE
        glm::glm
)

add_executable(
    mirror_visual_demo
    demos/mirror_visual_demo.cpp
    src/mirror/Mirror.cpp
)

target_include_directories(
    mirror_visual_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    mirror_visual_demo
    PRIVATE
        glfw
        glad_gl_core_33
        glm::glm
)