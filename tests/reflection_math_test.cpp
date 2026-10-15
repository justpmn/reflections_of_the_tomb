
#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>

#include "beam/BeamSystem.h"

int main()
{
    // Ray travelling diagonally downward.
    glm::vec3 incoming(1.0f, -1.0f, 0.0f);

    // Upward-facing mirror normal.
    glm::vec3 normal(0.0f, 1.0f, 0.0f);

    glm::vec3 result =
        BeamSystem::reflectDirection(incoming, normal);

    std::cout << "Reflected direction: "
              << result.x << ", "
              << result.y << ", "
              << result.z << '\n';

    // Expected: diagonally upward and right.
    glm::vec3 expected =
        glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));

    if (glm::length(result - expected) > 0.0001f)
    {
        throw std::runtime_error(
            "Reflection test failed"
        );
    }

    std::cout << "Reflection test PASSED!\n";

    return 0;
}
