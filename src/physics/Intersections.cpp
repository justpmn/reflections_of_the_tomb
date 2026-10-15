
#include "Intersections.h"

#include <cmath>

bool Intersections::rayPlane(
    const Ray& ray,
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal,
    float& distance)
{
    constexpr float EPSILON = 0.00001f;

    // Calculate the dot product of ray and plane normal.
    float denominator =
        glm::dot(ray.direction, planeNormal);

    // Ray is parallel to the plane.
    if (std::abs(denominator) < EPSILON)
    {
        return false;
    }

    // Calculate the intersection parameter.
    float t =
        glm::dot(
            planePoint - ray.origin,
            planeNormal
        ) / denominator;

    // Intersection is behind the origin,
    // or too close to it.
    if (t <= EPSILON)
    {
        return false;
    }

    distance = t;

    return true;
}
