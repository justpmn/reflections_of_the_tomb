#include "PlayerController.h"

#include <cctype>
#include <stdexcept>

PlayerController::PlayerController(
    Player& player,
    int width,
    int height,
    int step
)
    : player_(player),
      width_(width),
      height_(height),
      step_(step) {

    const int radius = player_.getRadius();

    if (
        step <= 0 ||
        player_.getX() - radius < 0 ||
        player_.getY() - radius < 0 ||
        player_.getX() + radius >= width ||
        player_.getY() + radius >= height
    ) {
        throw std::invalid_argument(
            "Player must fit inside canvas; step must be positive."
        );
    }
}

bool PlayerController::handleInput(char key) {
    // Accept both uppercase and lowercase commands.
    key = static_cast<char>(
        std::tolower(static_cast<unsigned char>(key))
    );

    int dx = 0;
    int dy = 0;

    switch (key) {
        case 'w':
            dy = -step_;
            break;

        case 's':
            dy = step_;
            break;

        case 'a':
            dx = -step_;
            break;

        case 'd':
            dx = step_;
            break;

        case 'r':
            player_.reset();
            return true;

        default:
            return false;
    }

    const int nextX = player_.getX() + dx;
    const int nextY = player_.getY() + dy;
    const int radius = player_.getRadius();

    // Reject movement if any part of the outer circle
    // would leave the canvas.
    if (
        nextX - radius < 0 ||
        nextX + radius >= width_ ||
        nextY - radius < 0 ||
        nextY + radius >= height_
    ) {
        return false;
    }

    player_.translate(dx, dy);
    return true;
}
