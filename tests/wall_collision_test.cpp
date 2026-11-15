
#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "beam/BeamTracer.h"

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

int main()
{
    // A wall at X = 3.
    std::vector<WallSegment> walls = {
        {{3.0f, -2.0f}, {3.0f, 2.0f}}
    };

    // A diagonal mirror centered at X = 5.
    std::vector<Mirror> mirrors;
    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f,
        45.0f,
        -180.0f,
        180.0f
    );

    // TEST 1: Wall blocks a beam with no mirrors.
    {
        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {},
            4, 20.0f, 1.0f,
            walls
        );

        require(path.size() == 1,
                "Test 1: Expected one segment");

        require(nearPoint(
            path[0].end,
            {3.0f, 1.0f, 0.0f}
        ), "Test 1: Beam passed through wall");

        std::cout << "Test 1 PASSED: Wall blocks beam\n";
    }

    // TEST 2: Wall shields a mirror.
    {
        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            4, 20.0f, 1.0f,
            walls
        );

        require(path.size() == 1,
                "Test 2: Mirror behind wall must not reflect");

        require(nearPoint(
            path[0].end,
            {3.0f, 1.0f, 0.0f}
        ), "Test 2: Incorrect wall hit");

        std::cout << "Test 2 PASSED: Mirror shielded\n";
    }

    // TEST 3: Beam reflects, then hits a wall.
    {
        std::vector<WallSegment> upperWall = {
            {{4.0f, 3.0f}, {6.0f, 3.0f}}
        };

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            4, 20.0f, 1.0f,
            upperWall
        );

        require(path.size() == 2,
                "Test 3: Expected two segments");

        require(nearPoint(
            path[0].end,
            {5.0f, 1.0f, 0.0f}
        ), "Test 3: First mirror hit incorrect");

        require(nearPoint(
            path[1].end,
            {5.0f, 1.0f, 3.0f}
        ), "Test 3: Beam failed to stop at wall");

        std::cout << "Test 3 PASSED: Wall after reflection\n";
    }

    // TEST 4: Nearest wall wins regardless of order.
    {
        std::vector<WallSegment> candidates = {
            {{8.0f, -2.0f}, {8.0f, 2.0f}},
            {{3.0f, -2.0f}, {3.0f, 2.0f}}
        };

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {},
            4, 20.0f, 1.0f,
            candidates
        );

        require(path.size() == 1,
                "Test 4: Expected one segment");

        require(nearPoint(
            path[0].end,
            {3.0f, 1.0f, 0.0f}
        ), "Test 4: Nearest wall not selected");

        std::cout << "Test 4 PASSED: Nearest wall\n";
    }

    // TEST 5: Wall and mirror overlap.
    // Wall must take priority.
    {
        std::vector<WallSegment> overlappingWall = {
            {{5.0f, -2.0f}, {5.0f, 2.0f}}
        };

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            4, 20.0f, 1.0f,
            overlappingWall
        );

        require(path.size() == 1,
                "Test 5: Wall should win over mirror");

        require(nearPoint(
            path[0].end,
            {5.0f, 1.0f, 0.0f}
        ), "Test 5: Wrong stopping point");

        std::cout << "Test 5 PASSED: Wall priority\n";
    }

    // TEST 6: A wall behind the emitter is ignored.
    {
        std::vector<WallSegment> behindWall = {
            {{-3.0f, -2.0f}, {-3.0f, 2.0f}}
        };

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {},
            4, 10.0f, 1.0f,
            behindWall
        );

        require(path.size() == 1,
                "Test 6: Expected one segment");

        require(nearPoint(
            path[0].end,
            {10.0f, 1.0f, 0.0f}
        ), "Test 6: Behind-wall collision");

        std::cout << "Test 6 PASSED: Behind wall ignored\n";
    }

    // TEST 7: Wall beyond distance budget is ignored.
    {
        std::vector<WallSegment> farWall = {
            {{15.0f, -2.0f}, {15.0f, 2.0f}}
        };

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {},
            4, 10.0f, 1.0f,
            farWall
        );

        require(path.size() == 1,
                "Test 7: Expected one segment");

        require(nearPoint(
            path[0].end,
            {10.0f, 1.0f, 0.0f}
        ), "Test 7: Distance budget exceeded");

        std::cout << "Test 7 PASSED: Distance limit\n";
    }

    std::cout
        << "\nAll 7 wall collision checks PASSED!\n";

    return 0;
}
