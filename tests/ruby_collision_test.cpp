
#include <cassert>
#include <iostream>

#include "gameplay/RubyCollision.h"

int main()
{
    BeamSegment beam{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(10.0f, 0.0f, 0.0f)
    };

    // Ruby directly on the beam.
    assert(RubyCollision::intersects(
        beam, glm::vec3(5.0f, 0.0f, 0.0f), 0.5f
    ));

    // Ruby outside the beam's radius.
    assert(!RubyCollision::intersects(
        beam, glm::vec3(5.0f, 2.0f, 0.0f), 0.5f
    ));

    // Ruby touching the beam boundary.
    assert(RubyCollision::intersects(
        beam, glm::vec3(5.0f, 0.5f, 0.0f), 0.5f
    ));

    // Ruby beyond the end of the beam.
    assert(!RubyCollision::intersects(
        beam, glm::vec3(12.0f, 0.0f, 0.0f), 0.5f
    ));

    // Zero-length beam should not register a hit.
    BeamSegment emptyBeam{
        glm::vec3(0.0f),
        glm::vec3(0.0f)
    };

    assert(!RubyCollision::intersects(
        emptyBeam, glm::vec3(0.0f), 0.5f
    ));

    // Invalid radius should not register a hit.
    assert(!RubyCollision::intersects(
        beam, glm::vec3(5.0f, 0.0f, 0.0f), 0.0f
    ));

    std::cout << "All Ruby collision tests passed!\n";
    return 0;
}
