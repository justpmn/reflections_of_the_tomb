
#include <cassert>
#include <iostream>
#include <glm/glm.hpp>

#include "beam/BeamSystem.h"
#include "beam/BeamSegment.h"
#include "gameplay/PuzzleSystem.h"

int main()
{
    PuzzleSystem puzzle;

    // Place the real ruby along the reflected beam.
    puzzle.setRubyPositions(
        glm::vec3(5.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 5.0f, 0.0f),
        0.5f
    );

    // Incoming light travels downward.
    glm::vec3 incoming(0.0f, -1.0f, 0.0f);

    // A 45-degree mirror reflects it toward +X.
    glm::vec3 normal(1.0f, 1.0f, 0.0f);

    glm::vec3 reflected =
        BeamSystem::reflectDirection(incoming, normal);

    // Check Peter's reflection calculation.
    assert(glm::length(
        reflected - glm::vec3(1.0f, 0.0f, 0.0f)
    ) < 0.0001f);

    // Construct a beam leaving the mirror.
    glm::vec3 mirrorPosition(0.0f, 0.0f, 0.0f);

    BeamSegment reflectedBeam{
        mirrorPosition,
        mirrorPosition + reflected * 10.0f
    };

    // Pass the reflected beam to your puzzle.
    puzzle.processBeam(reflectedBeam);

    assert(puzzle.isSolved());
    assert(puzzle.isDoorOpen());
    assert(!puzzle.areTrapsActive());

    std::cout
        << "Reflected beam activated real ruby!\n";

    return 0;
}
