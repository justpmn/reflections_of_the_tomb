
#pragma once

#include <glm/glm.hpp>
#include "beam/BeamSegment.h"

class RubyCollision
{
public:
    // Checks whether a finite beam segment
    // intersects a spherical ruby.
    static bool intersects(
        const BeamSegment& beam,
        const glm::vec3& rubyPosition,
        float rubyRadius
    );
};
