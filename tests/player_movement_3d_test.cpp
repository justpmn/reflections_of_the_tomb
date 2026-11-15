#include "player/PlayerMovement3D.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace player3d;

static bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }

int main() {
    const RoomBoundsXZ room{-4, 4, -4, 4};

    PlayerMovement3D player({0, 0}, 2.0f, 0.25f);
    player.update(1, 0, 0.5f, room);
    assert(near(player.position().x, 1.0f));
    assert(player.isMoving());
    player.reset();
    assert(near(player.position().x, 0.0f));

    player.update(1, 1, 0.5f, room);
    const auto pos = player.position();
    assert(near(std::sqrt(pos.x * pos.x + pos.z * pos.z), 1.0f));

    player.reset();
    player.update(-1, 0, 100.0f, room);
    assert(near(player.position().x, -3.75f));

    player.reset();
    const std::vector<ObstacleXZ> wall{{0.4f, 1.4f, -1.0f, 1.0f}};
    player.update(1, 1, 0.5f, room, wall);
    assert(player.position().x < 0.4f);
    assert(player.position().z > 0.0f); // slides along the obstacle

    // Even on a long frame, a player cannot tunnel through a thin wall.
    player.reset();
    const std::vector<ObstacleXZ> thinWall{{0.6f, 0.65f, -1.0f, 1.0f}};
    player.update(1, 0, 1.0f, room, thinWall);
    assert(player.position().x <= 0.35f + 0.0001f);

    player.reset();
    assert(player.canInteract({0.6f, 0.0f}, 1.0f));
    assert(!player.canInteract({2.0f, 0.0f}, 1.0f));

    player.update(0, 0, 0.1f, room);
    assert(!player.isMoving());

    std::cout << "All player movement and interaction tests PASSED!\n";
}
