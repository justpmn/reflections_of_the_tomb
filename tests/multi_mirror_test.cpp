
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "beam/BeamTracer.h"

void check(bool condition, const char* message)
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
    // TEST 1: No mirrors, beam travels straight.
    {
        std::vector<Mirror> mirrors;

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {2.0f, 0.0f},
            mirrors,
            4,
            10.0f,
            1.0f
        );

        check(path.size() == 1,
              "Test 1: Expected one segment");

        check(nearPoint(path[0].end, {10.0f, 1.0f, 0.0f}),
              "Test 1: Incorrect endpoint");

        std::cout << "Test 1 PASSED: Straight beam\n";
    }

    // Two mirrors redirect the beam:
    // +X -> +Z -> -X
    std::vector<Mirror> mirrors;

    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f,
        45.0f,
        -180.0f,
        180.0f
    );

    mirrors.emplace_back(
        glm::vec2(5.0f, 5.0f),
        4.0f,
        135.0f,
        -180.0f,
        180.0f
    );

    // TEST 2: Two reflections create three segments.
    {
        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            2,
            20.0f,
            1.0f
        );

        check(path.size() == 3,
              "Test 2: Expected three beam segments");

        check(nearPoint(path[0].end, {5.0f, 1.0f, 0.0f}),
              "Test 2: First mirror hit incorrect");

        check(nearPoint(path[1].end, {5.0f, 1.0f, 5.0f}),
              "Test 2: Second mirror hit incorrect");

        check(nearPoint(path[2].end, {-5.0f, 1.0f, 5.0f}),
              "Test 2: Final beam endpoint incorrect");

        std::cout
            << "Test 2 PASSED: Two mirror reflections\n";
    }

    // TEST 3: Zero allowed reflections.
    {
        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            0,
            20.0f
        );

        check(path.size() == 1,
              "Test 3: Beam should stop at first mirror");

        check(nearPoint(path[0].end, {5.0f, 1.0f, 0.0f}),
              "Test 3: Incorrect stopping point");

        std::cout
            << "Test 3 PASSED: Zero bounce limit\n";
    }

    // TEST 4: One allowed reflection.
    {
        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            mirrors,
            1,
            20.0f
        );

        check(path.size() == 2,
              "Test 4: Expected two segments");

        check(nearPoint(path[1].end, {5.0f, 1.0f, 5.0f}),
              "Test 4: Beam should stop at second mirror");

        std::cout
            << "Test 4 PASSED: One bounce limit\n";
    }

    // TEST 5: The nearest mirror wins.
    {
        std::vector<Mirror> candidates;

        // Add the farther mirror first deliberately.
        candidates.emplace_back(
            glm::vec2(8.0f, 0.0f),
            4.0f, 90.0f, -180.0f, 180.0f
        );

        candidates.emplace_back(
            glm::vec2(3.0f, 0.0f),
            4.0f, 90.0f, -180.0f, 180.0f
        );

        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            candidates,
            0,
            20.0f
        );

        check(path.size() == 1,
              "Test 5: Expected one segment");

        check(nearPoint(path[0].end, {3.0f, 1.0f, 0.0f}),
              "Test 5: Wrong mirror selected");

        std::cout
            << "Test 5 PASSED: Nearest mirror priority\n";
    }

    // TEST 6: Invalid direction yields no path.
    {
        auto path = BeamTracer::trace(
            {0.0f, 0.0f},
            {0.0f, 0.0f},
            mirrors
        );

        check(path.empty(),
              "Test 6: Invalid direction should be rejected");

        std::cout
            << "Test 6 PASSED: Invalid direction\n";
    }

    std::cout
        << "\nAll 6 multi-mirror tests PASSED!\n";

    return 0;
}
