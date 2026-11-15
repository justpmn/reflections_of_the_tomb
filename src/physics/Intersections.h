
#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

class Intersections
{
public:
    static bool rayPlane(
        const Ray& ray,
        const glm::vec3& planePoint,
        const glm::vec3& planeNormal,
        float& distance
    );
    static bool raySegment2D(
        const glm::vec2& rayOrigin,
        const glm::vec2& rayDirection,
        const glm::vec2& segmentA,
        const glm::vec2& segmentB,
        float& distance,
        glm::vec2& hitPoint
    );
};
