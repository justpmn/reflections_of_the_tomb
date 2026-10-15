
#include "BeamSystem.h"

#include <stdexcept>

glm::vec3 BeamSystem::reflectDirection(
    const glm::vec3& incoming,
    const glm::vec3& normal)
{
    // Prevent normalization of zero-length vectors.
    if (glm::dot(incoming, incoming) < 1e-12f ||
        glm::dot(normal, normal) < 1e-12f)
    {
        throw std::invalid_argument(
            "Reflection requires non-zero vectors"
        );
    }

    // Normalize the incoming direction.
    glm::vec3 D = glm::normalize(incoming);

    // Normalize the mirror's surface normal.
    glm::vec3 N = glm::normalize(normal);

    // Apply the law of reflection.
    glm::vec3 R =
        D - 2.0f * glm::dot(D, N) * N;

    return glm::normalize(R);
}
