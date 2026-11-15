
#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "beam/BeamSegment.h"

class BeamPlayerCollision
{
public:
    // Player position represents the center of their feet.
    // The player occupies a vertical cylinder.
    //
    // Supported beams are horizontal (constant Y),
    // matching the current BeamTracer output.
    static bool intersects(
        const std::vector<BeamSegment>& path,
        const glm::vec3& playerFeet,
        float playerRadius,
        float playerHeight
    );
};
