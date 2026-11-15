#include "Player.h"
#include <stdexcept>

Player::Player(int x, int y, int radius)
    : x_(x),
      y_(y),
      radius_(radius),
      startX_(x),
      startY_(y) {

    if (radius < 8) {
        throw std::invalid_argument(
            "Player radius must be at least 8."
        );
    }
}

void Player::midpointCircle(
    int cx,
    int cy,
    int radius,
    const PlotPixel& plot
) {
    int x = 0;
    int y = radius;
    int decision = 1 - radius;

    while (x <= y) {
        // Eight-way symmetry.
        plot(cx + x, cy + y);
        plot(cx - x, cy + y);
        plot(cx + x, cy - y);
        plot(cx - x, cy - y);

        plot(cx + y, cy + x);
        plot(cx - y, cy + x);
        plot(cx + y, cy - x);
        plot(cx - y, cy - x);

        ++x;

        if (decision < 0) {
            // Choose the east pixel.
            decision += 2 * x + 1;
        } else {
            // Choose the south-east pixel.
            --y;
            decision += 2 * (x - y) + 1;
        }
    }
}

void Player::draw(const PlotPixel& plot) const {
    // Outer circle: player's head/helmet.
    midpointCircle(x_, y_, radius_, plot);

    // Inner circle: helmet detail.
    midpointCircle(x_, y_, radius_ - 4, plot);

    // Left eye.
    midpointCircle(
        x_ - radius_ / 3,
        y_ - radius_ / 5,
        radius_ / 8,
        plot
    );

    // Right eye.
    midpointCircle(
        x_ + radius_ / 3,
        y_ - radius_ / 5,
        radius_ / 8,
        plot
    );

    // All parts use offsets from the player centre.
    // Therefore, they move together when the centre changes.
}

void Player::translate(int dx, int dy) {
    // Translation:
    // x' = x + dx
    // y' = y + dy
    x_ += dx;
    y_ += dy;
}

void Player::reset() {
    x_ = startX_;
    y_ = startY_;
}
