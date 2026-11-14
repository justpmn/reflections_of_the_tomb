
#include <cassert>
#include <iostream>
#include "gameplay/PuzzleSystem.h"

int main()
{
    PuzzleSystem puzzle;

    // Initial state.
    assert(!puzzle.isSolved());
    assert(!puzzle.isDoorOpen());
    assert(puzzle.areTrapsActive());

    // Hit the false ruby.
    puzzle.hitRuby(false);

    assert(!puzzle.isSolved());
    assert(!puzzle.isDoorOpen());
    assert(puzzle.areTrapsActive());

    // Hit the real ruby.
    puzzle.hitRuby(true);

    assert(puzzle.isSolved());
    assert(puzzle.isDoorOpen());
    assert(!puzzle.areTrapsActive());

    // Additional hits should not undo success.
    puzzle.hitRuby(false);
    assert(puzzle.isSolved());
    assert(puzzle.isDoorOpen());

    // Reset everything.
    puzzle.reset();

    assert(!puzzle.isSolved());
    assert(!puzzle.isDoorOpen());
    assert(puzzle.areTrapsActive());

    std::cout << "All PuzzleSystem tests passed!\n";
    return 0;
}
