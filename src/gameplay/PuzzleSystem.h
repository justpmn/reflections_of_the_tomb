
#pragma once

#include <glm/glm.hpp>

#include "Ruby.h"
#include "Trap.h"
#include "Door.h"
#include "beam/BeamSegment.h"

class PuzzleSystem
{
public:
    PuzzleSystem();

    // Existing keyboard demo support.
    void hitRuby(bool isRealRuby);

    // New: detect ruby hits from a beam segment.
    void processBeam(const BeamSegment& beam);

    // Set the positions of both rubies.
    void setRubyPositions(
        const glm::vec3& realPosition,
        const glm::vec3& falsePosition,
        float radius
    );

    void reset();

    bool isSolved() const;
    bool isDoorOpen() const;
    bool areTrapsActive() const;

private:
    Ruby realRuby;
    Ruby falseRuby;
    Trap trap;
    Door door;

    glm::vec3 realRubyPosition;
    glm::vec3 falseRubyPosition;
    float rubyRadius;

    bool solved;
};
