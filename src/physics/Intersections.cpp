
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
bool Intersections::raySegment2D(
    const glm::vec2& rayOrigin,
    const glm::vec2& rayDirection,
    const glm::vec2& segmentA,
    const glm::vec2& segmentB,
    float& distance,
    glm::vec2& hitPoint)
{
    constexpr float EPSILON = 0.00001f;

    auto cross2D = [](const glm::vec2& a, const glm::vec2& b) {
        return a.x * b.y - a.y * b.x;
    };

    if (glm::dot(rayDirection, rayDirection) < EPSILON * EPSILON)
        return false;

    glm::vec2 r = glm::normalize(rayDirection);
    glm::vec2 s = segmentB - segmentA;

    if (glm::dot(s, s) < EPSILON * EPSILON)
        return false;

    float denominator = cross2D(r, s);

    // Parallel ray and mirror surface.
    if (std::abs(denominator) < EPSILON)
        return false;

    glm::vec2 offset = segmentA - rayOrigin;

    float t = cross2D(offset, s) / denominator;
    float u = cross2D(offset, r) / denominator;

    // Behind ray origin, or outside finite mirror endpoints.
    if (t <= EPSILON || u < -EPSILON || u > 1.0f + EPSILON)
        return false;

    distance = t;
    hitPoint = rayOrigin + r * t;

    return true;
}
