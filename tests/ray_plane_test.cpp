
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "physics/Intersections.h"

int main()
{
    Ray ray{
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    };

    glm::vec3 planePoint(5.0f, 0.0f, 0.0f);
    glm::vec3 normal(-1.0f, 0.0f, 0.0f);

    float distance = 0.0f;

    bool hit = Intersections::rayPlane(
        ray, planePoint, normal, distance
    );

    if (!hit || std::abs(distance - 5.0f) > 0.0001f)
        throw std::runtime_error("Forward intersection failed");

    glm::vec3 hitPoint =
        ray.origin + ray.direction * distance;

    std::cout << "Hit point: "
              << hitPoint.x << ", "
              << hitPoint.y << ", "
              << hitPoint.z << '\n';

    // Parallel rays should not intersect.
    Ray parallel{
        ray.origin,
        glm::vec3(0.0f, 0.0f, 1.0f)
    };

    if (Intersections::rayPlane(
            parallel, planePoint, normal, distance))
        throw std::runtime_error("Parallel test failed");

    // A plane behind the ray should not count.
    Ray away{
        ray.origin,
        glm::vec3(-1.0f, 0.0f, 0.0f)
    };

    if (Intersections::rayPlane(
            away, planePoint, normal, distance))
        throw std::runtime_error("Behind-ray test failed");

    std::cout << "All ray-plane tests PASSED!\n";

    return 0;
}
