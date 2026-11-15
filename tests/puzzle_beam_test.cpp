
#include <cassert>
#include <iostream>

#include "gameplay/PuzzleSystem.h"

int main()
{
    PuzzleSystem puzzle;

    puzzle.setRubyPositions(
        glm::vec3(5.0f, 0.0f, 0.0f),
        glm::vec3(5.0f, 3.0f, 0.0f),
        0.5f
    );

    // Beam that misses both rubies.
    BeamSegment miss{
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(10.0f, 5.0f, 0.0f)
    };

    puzzle.processBeam(miss);
    assert(!puzzle.isSolved());

    // Beam that hits the false ruby.
    BeamSegment falseHit{
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(10.0f, 3.0f, 0.0f)
    };

    puzzle.processBeam(falseHit);

    assert(!puzzle.isSolved());
    assert(!puzzle.isDoorOpen());
    assert(puzzle.areTrapsActive());

    // Beam that hits the real ruby.
    BeamSegment realHit{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(10.0f, 0.0f, 0.0f)
    };

    puzzle.processBeam(realHit);

    assert(puzzle.isSolved());
    assert(puzzle.isDoorOpen());
    assert(!puzzle.areTrapsActive());

    // Reset the puzzle.
    puzzle.reset();

    assert(!puzzle.isSolved());
    assert(!puzzle.isDoorOpen());
    assert(puzzle.areTrapsActive());

    std::cout << "All puzzle beam tests passed!\n";
    return 0;
}
