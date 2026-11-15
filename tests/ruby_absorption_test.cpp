
#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "beam/BeamTracer.h"
#include "gameplay/BeamPuzzleBridge.h"
#include "gameplay/PuzzleSystem.h"

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool nearPoint(
    const glm::vec3& a,
    const glm::vec3& b)
{
    return glm::length(a - b) < 0.001f;
}

std::vector<Mirror> createMirrors()
{
    std::vector<Mirror> mirrors;

    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f, 45.0f, -180.0f, 180.0f
    );

    mirrors.emplace_back(
        glm::vec2(5.0f, 5.0f),
        4.0f, 135.0f, -180.0f, 180.0f
    );

    return mirrors;
}

int main()
{
    // 1. Direct light stops at the ruby's surface.
    {
        std::vector<RubyTarget> rubies = {
            {{5.0f, 1.0f, 0.0f}, 0.5f, true}
        };

        auto result = BeamTracer::traceWithRubies(
            {0.0f, 0.0f}, {1.0f, 0.0f},
            {}, rubies, 4, 10.0f, 1.0f
        );

        require(result.hitRubyIndex == 0,
                "Test 1: Ruby hit not reported");

        require(result.path.size() == 1,
                "Test 1: Expected one segment");

        require(nearPoint(
            result.path.back().end,
            {4.5f, 1.0f, 0.0f}
        ), "Test 1: Beam passed through ruby");

        std::cout << "Test 1 PASSED: Ruby absorption\n";
    }

    auto mirrors = createMirrors();

    // 2. Light reaches ruby after two reflections.
    std::vector<RubyTarget> target = {
        {{-2.0f, 1.0f, 5.0f}, 0.3f, true}
    };

    auto reflected = BeamTracer::traceWithRubies(
        {0.0f, 0.0f}, {1.0f, 0.0f},
        mirrors, target, 4, 20.0f, 1.0f
    );

    require(reflected.hitRubyIndex == 0,
            "Test 2: Reflected beam missed ruby");

    require(reflected.path.size() == 3,
            "Test 2: Expected three segments");

    require(nearPoint(
        reflected.path.back().end,
        {-1.7f, 1.0f, 5.0f}
    ), "Test 2: Beam did not stop at surface");

    std::cout << "Test 2 PASSED: Reflected absorption\n";

    // 3. Wall blocks the ruby.
    {
        std::vector<WallSegment> walls = {
            {{3.0f, -2.0f}, {3.0f, 2.0f}}
        };

        std::vector<RubyTarget> rubies = {
            {{5.0f, 1.0f, 0.0f}, 0.5f, true}
        };

        auto result = BeamTracer::traceWithRubies(
            {0.0f, 0.0f}, {1.0f, 0.0f},
            {}, rubies, 4, 10.0f, 1.0f, walls
        );

        require(result.hitRubyIndex == -1,
                "Test 3: Ruby activated through wall");

        require(nearPoint(
            result.path.back().end,
            {3.0f, 1.0f, 0.0f}
        ), "Test 3: Wall not blocking beam");

        std::cout << "Test 3 PASSED: Wall protection\n";
    }

    // 4. Mirror reflects light away from a ruby behind it.
    {
        std::vector<RubyTarget> rubies = {
            {{7.0f, 1.0f, 0.0f}, 0.3f, true}
        };

        auto result = BeamTracer::traceWithRubies(
            {0.0f, 0.0f}, {1.0f, 0.0f},
            mirrors, rubies, 2, 20.0f, 1.0f
        );

        require(result.hitRubyIndex == -1,
                "Test 4: Light passed through mirror");

        std::cout << "Test 4 PASSED: Mirror protection\n";
    }

    // 5. Ruby is too high for the beam to hit.
    {
        std::vector<RubyTarget> rubies = {
            {{5.0f, 4.0f, 0.0f}, 0.5f, true}
        };

        auto result = BeamTracer::traceWithRubies(
            {0.0f, 0.0f}, {1.0f, 0.0f},
            {}, rubies, 4, 10.0f, 1.0f
        );

        require(result.hitRubyIndex == -1,
                "Test 5: Ruby hit at wrong height");

        std::cout << "Test 5 PASSED: Height separation\n";
    }

    // 6. Nearest ruby absorbs light, regardless of order.
    {
        std::vector<RubyTarget> rubies = {
            {{8.0f, 1.0f, 0.0f}, 0.5f, true},
            {{4.0f, 1.0f, 0.0f}, 0.5f, false}
        };

        auto result = BeamTracer::traceWithRubies(
            {0.0f, 0.0f}, {1.0f, 0.0f},
            {}, rubies, 4, 10.0f, 1.0f
        );

        require(result.hitRubyIndex == 1,
                "Test 6: Wrong ruby selected");

        require(nearPoint(
            result.path.back().end,
            {3.5f, 1.0f, 0.0f}
        ), "Test 6: Incorrect absorption point");

        PuzzleSystem puzzle;
        BeamPuzzleBridge::processAbsorbingTrace(
            result, rubies, puzzle
        );

        require(!puzzle.isSolved(),
                "Test 6: False ruby solved puzzle");

        std::cout << "Test 6 PASSED: False ruby absorbs\n";
    }

    // 7. Real ruby activates Jesse's puzzle mechanics.
    {
        PuzzleSystem puzzle;

        BeamPuzzleBridge::processAbsorbingTrace(
            reflected, target, puzzle
        );

        require(puzzle.isSolved(),
                "Test 7: Puzzle not solved");

        require(puzzle.isDoorOpen(),
                "Test 7: Door did not open");

        require(!puzzle.areTrapsActive(),
                "Test 7: Traps still active");

        std::cout << "Test 7 PASSED: Puzzle activation\n";
    }

    // 8. Wall wins when collision distances are tied.
    {
        std::vector<WallSegment> walls = {
            {{3.0f, -2.0f}, {3.0f, 2.0f}}
        };

        std::vector<RubyTarget> rubies = {
            {{3.5f, 1.0f, 0.0f}, 0.5f, true}
        };

        auto result = BeamTracer::traceWithRubies(
            {0.0f, 0.0f}, {1.0f, 0.0f},
            {}, rubies, 4, 10.0f, 1.0f, walls
        );

        require(result.hitRubyIndex == -1,
                "Test 8: Ruby won wall tie");

        std::cout << "Test 8 PASSED: Wall priority\n";
    }

    std::cout
        << "\nAll 8 ruby absorption checks PASSED!\n";

    return 0;
}
