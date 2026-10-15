
#pragma once

#include <glm/glm.hpp>

class BeamSystem
{
public:
    // Calculate the reflected direction of an incoming ray.
    static glm::vec3 reflectDirection(
        const glm::vec3& incoming,
        const glm::vec3& normal
    );
};
