
#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "beam/BeamTracer.h"
#include "physics/BeamPlayerCollision.h"

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main()
{
    std::vector<Mirror> mirrors;

    // +X -> +Z -> -X
    mirrors.emplace_back(
        glm::vec2(5.0f, 0.0f),
        4.0f, 45.0f, -180.0f, 180.0f
    );

    mirrors.emplace_back(
        glm::vec2(5.0f, 5.0f),
        4.0f, 135.0f, -180.0f, 180.0f
    );

    auto path = BeamTracer::trace(
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        mirrors,
        2, 20.0f, 1.0f
    );

    require(path.size() == 3,
            "Expected three beam segments");

    // TEST 1: Player stands in direct beam.
    require(BeamPlayerCollision::intersects(
        path, {2.0f, 0.0f, 0.0f}, 0.4f, 2.0f
    ), "Test 1: Direct beam did not hit player");

    std::cout << "Test 1 PASSED: Direct hit\n";

    // TEST 2: Player stands beside the beam.
    require(!BeamPlayerCollision::intersects(
        path, {2.0f, 0.0f, 1.0f}, 0.4f, 2.0f
    ), "Test 2: Incorrect collision beside beam");

    std::cout << "Test 2 PASSED: Near miss\n";

    // TEST 3: Player is behind the first mirror.
    require(!BeamPlayerCollision::intersects(
        path, {7.0f, 0.0f, 0.0f}, 0.4f, 2.0f
    ), "Test 3: Beam passed through mirror");

    std::cout << "Test 3 PASSED: Mirror blocks direct path\n";

    // TEST 4: Reflected beam hits player.
    require(BeamPlayerCollision::intersects(
        path, {-2.0f, 0.0f, 5.0f}, 0.4f, 2.0f
    ), "Test 4: Reflected beam missed player");

    std::cout << "Test 4 PASSED: Reflected beam hit\n";

    // TEST 5: Wall blocks beam before player.
    std::vector<WallSegment> walls = {
        {{1.0f, 4.0f}, {1.0f, 6.0f}}
    };

    auto blockedPath = BeamTracer::trace(
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        mirrors,
        2, 20.0f, 1.0f,
        walls
    );

    require(!BeamPlayerCollision::intersects(
        blockedPath, {-2.0f, 0.0f, 5.0f},
        0.4f, 2.0f
    ), "Test 5: Player hit through wall");

    std::cout << "Test 5 PASSED: Wall protection\n";

    // TEST 6: Beam travels above the player's head.
    auto elevatedPath = BeamTracer::trace(
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {},
        4, 10.0f, 3.0f
    );

    require(!BeamPlayerCollision::intersects(
        elevatedPath, {2.0f, 0.0f, 0.0f},
        0.4f, 2.0f
    ), "Test 6: Player hit above head");

    std::cout << "Test 6 PASSED: Height separation\n";

    // TEST 7: Invalid player radius.
    require(!BeamPlayerCollision::intersects(
        path, {2.0f, 0.0f, 0.0f}, 0.0f, 2.0f
    ), "Test 7: Invalid radius accepted");

    std::cout << "Test 7 PASSED: Invalid radius\n";

    std::cout
        << "\nAll 7 player collision checks PASSED!\n";

    return 0;
}
