
#include <cmath>
#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>
#include "physics/Intersections.h"

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool near(float a, float b)
{
    return std::abs(a - b) < 0.0001f;
}

int main()
{
    const glm::vec2 mirrorA(5.0f, -1.0f);
    const glm::vec2 mirrorB(5.0f, 1.0f);

    float distance = 0.0f;
    glm::vec2 hitPoint(0.0f);

    // Test 1: Beam hits mirror center.
    bool hit = Intersections::raySegment2D(
        {0.0f, 0.0f}, {1.0f, 0.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(hit, "Test 1 failed: expected a hit");
    require(near(distance, 5.0f),
            "Test 1 failed: incorrect distance");
    require(glm::length(hitPoint - glm::vec2(5.0f, 0.0f))
            < 0.0001f, "Test 1 failed: incorrect hit point");

    std::cout << "Test 1 PASSED: Center hit\n";

    // Test 2: Beam misses finite mirror.
    hit = Intersections::raySegment2D(
        {0.0f, 3.0f}, {1.0f, 0.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(!hit, "Test 2 failed: expected a miss");
    std::cout << "Test 2 PASSED: Outside endpoints\n";

    // Test 3: Parallel ray.
    hit = Intersections::raySegment2D(
        {4.0f, 0.0f}, {0.0f, 1.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(!hit, "Test 3 failed: parallel ray");
    std::cout << "Test 3 PASSED: Parallel ray\n";

    // Test 4: Mirror lies behind the ray.
    hit = Intersections::raySegment2D(
        {10.0f, 0.0f}, {1.0f, 0.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(!hit, "Test 4 failed: behind-ray hit");
    std::cout << "Test 4 PASSED: Behind-ray rejection\n";

    // Test 5: Beam hits an endpoint.
    hit = Intersections::raySegment2D(
        {0.0f, 1.0f}, {1.0f, 0.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(hit, "Test 5 failed: endpoint should hit");
    std::cout << "Test 5 PASSED: Endpoint hit\n";

    // Test 6: Ray travels toward mirror from right.
    hit = Intersections::raySegment2D(
        {10.0f, 0.0f}, {-2.0f, 0.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(hit, "Test 6 failed: reverse direction");
    require(near(distance, 5.0f),
            "Test 6 failed: distance should be normalized");
    std::cout << "Test 6 PASSED: Reverse direction\n";

    // Test 7: Zero-length ray direction.
    hit = Intersections::raySegment2D(
        {0.0f, 0.0f}, {0.0f, 0.0f},
        mirrorA, mirrorB, distance, hitPoint
    );

    require(!hit, "Test 7 failed: zero ray");
    std::cout << "Test 7 PASSED: Zero-length ray\n";

    // Test 8: Degenerate mirror segment.
    hit = Intersections::raySegment2D(
        {0.0f, 0.0f}, {1.0f, 0.0f},
        {5.0f, 0.0f}, {5.0f, 0.0f},
        distance, hitPoint
    );

    require(!hit, "Test 8 failed: degenerate mirror");
    std::cout << "Test 8 PASSED: Degenerate mirror\n";

    std::cout << "\nAll 8 finite mirror tests PASSED!\n";
    return 0;
}
