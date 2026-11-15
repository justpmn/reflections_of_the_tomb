
#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "beam/BeamTracer.h"
#include "gameplay/BeamPuzzleBridge.h"
#include "gameplay/PuzzleSystem.h"

void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

// Test 1-5 use the original demonstration layout.
std::vector<Mirror> createMirrors()
{
    std::vector<Mirror> mirrors;

    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f, 45.0f, -180.0f, 180.0f);

    mirrors.emplace_back(
        glm::vec2(5.0f, 5.0f),
        4.0f, 135.0f, -180.0f, 180.0f);

    return mirrors;
}

int main()
{
    const auto mirrors = createMirrors();

    // TEST 1: Two reflections activate the real ruby.
    {
        PuzzleSystem puzzle;

        puzzle.setRubyPositions(
            {-2.0f, 1.0f, 5.0f},
            {0.0f, 1.0f, 9.0f},
            0.35f);

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            2,
            20.0f,
            1.0f);

        require(path.size() == 3,
                "Test 1: Expected three beam segments");

        BeamPuzzleBridge::processPath(path, puzzle);

        require(puzzle.isSolved(),
                "Test 1: Real ruby not activated");

        require(puzzle.isDoorOpen(),
                "Test 1: Door should open");

        require(!puzzle.areTrapsActive(),
                "Test 1: Traps should deactivate");

        std::cout
            << "Test 1 PASSED: Reflected beam solves puzzle\n";
    }

    // TEST 2: Wall blocks the beam before the ruby.
    {
        PuzzleSystem puzzle;

        puzzle.setRubyPositions(
            {-2.0f, 1.0f, 5.0f},
            {0.0f, 1.0f, 9.0f},
            0.35f);

        std::vector<WallSegment> walls = {
            {{1.0f, 4.0f}, {1.0f, 6.0f}}
        };

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            2,
            20.0f,
            1.0f,
            walls);

        BeamPuzzleBridge::processPath(path, puzzle);

        require(!puzzle.isSolved(),
                "Test 2: Ruby activated through a wall");

        require(!puzzle.isDoorOpen(),
                "Test 2: Door must remain closed");

        require(puzzle.areTrapsActive(),
                "Test 2: Traps must remain active");

        std::cout
            << "Test 2 PASSED: Wall protects ruby\n";
    }

    // TEST 3: False ruby does not solve the puzzle.
    {
        PuzzleSystem puzzle;

        puzzle.setRubyPositions(
            {0.0f, 1.0f, 9.0f},
            {-2.0f, 1.0f, 5.0f},
            0.35f);

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            2,
            20.0f,
            1.0f);

        BeamPuzzleBridge::processPath(path, puzzle);

        require(!puzzle.isSolved(),
                "Test 3: False ruby solved puzzle");

        require(!puzzle.isDoorOpen(),
                "Test 3: Door opened incorrectly");

        std::cout
            << "Test 3 PASSED: False ruby ignored\n";
    }

    // TEST 4: Mirror redirects light away from a ruby.
    {
        PuzzleSystem puzzle;

        puzzle.setRubyPositions(
            {7.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 9.0f},
            0.35f);

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            2,
            20.0f,
            1.0f);

        BeamPuzzleBridge::processPath(path, puzzle);

        require(!puzzle.isSolved(),
                "Test 4: Beam passed through mirror");

        std::cout
            << "Test 4 PASSED: Mirror prevents transmission\n";
    }

    // TEST 5: Direct light activates a ruby.
    {
        PuzzleSystem puzzle;

        puzzle.setRubyPositions(
            {5.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 9.0f},
            0.35f);

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {},
            4,
            10.0f,
            1.0f);

        BeamPuzzleBridge::processPath(path, puzzle);

        require(puzzle.isSolved(),
                "Test 5: Direct beam missed ruby");

        std::cout
            << "Test 5 PASSED: Direct beam activates ruby\n";
    }

    // TEST 6: Actual temple level configuration.
    // Both mirrors are rotated into a valid solution.
    {
        std::vector<Mirror> levelMirrors;

        levelMirrors.emplace_back(
            glm::vec2(4.5f, -5.0f),
            4.0f,
            45.0f,
            -180.0f,
            180.0f);

        levelMirrors.emplace_back(
            glm::vec2(4.5f, 0.0f),
            4.0f,
            135.0f,
            -180.0f,
            180.0f);

        std::vector<RubyTarget> rubies = {
            {glm::vec3(-2.5f, 1.25f, 0.0f), 0.45f, true},
            {glm::vec3(1.5f, 1.25f, 2.0f), 0.45f, false}
        };

        std::vector<WallSegment> walls = {
            {{-6.5f, -6.5f}, {6.5f, -6.5f}},
            {{6.5f, -6.5f}, {6.5f, 6.5f}},
            {{6.5f, 6.5f}, {-6.5f, 6.5f}},
            {{-6.5f, 6.5f}, {-6.5f, -6.5f}}
        };

        BeamTraceResult result = BeamTracer::traceWithRubies(
            {-0.5f, -5.0f},
            {1.0f, 0.0f},
            levelMirrors,
            rubies,
            4,
            30.0f,
            1.25f,
            walls);

        require(result.hitRubyIndex == 0,
                "Test 6: Expected beam to hit the real ruby");

        PuzzleSystem puzzle;

        BeamPuzzleBridge::processAbsorbingTrace(
            result,
            rubies,
            puzzle);

        require(puzzle.isSolved(),
                "Test 6: Puzzle should be solved");

        std::cout
            << "Test 6 PASSED: Actual temple layout is solvable\n";
    }

    std::cout
        << "\nAll 6 beam-puzzle integration tests PASSED!\n";

    return 0;
}
