
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
};
