#include "PlayerMovement3D.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace player3d {

PlayerMovement3D::PlayerMovement3D(PositionXZ start, float speed, float radius)
    : start_(start), position_(start), speed_(speed), radius_(radius) {
    if (!(speed > 0.0f) || !std::isfinite(speed) ||
        !(radius > 0.0f) || !std::isfinite(radius) ||
        !std::isfinite(start.x) || !std::isfinite(start.z)) {
        throw std::invalid_argument("Invalid player movement parameters");
    }
}

bool PlayerMovement3D::collides(PositionXZ p,
                               const std::vector<ObstacleXZ>& obstacles) const {
    for (const auto& box : obstacles) {
        const float x = std::clamp(p.x, box.minX, box.maxX);
        const float z = std::clamp(p.z, box.minZ, box.maxZ);
        const float dx = p.x - x;
        const float dz = p.z - z;
        if (dx * dx + dz * dz < radius_ * radius_) return true;
    }
    return false;
}

void PlayerMovement3D::update(float inputX, float inputZ, float dtSeconds,
                              RoomBoundsXZ bounds,
                              const std::vector<ObstacleXZ>& obstacles) {
    moving_ = false;
    if (!std::isfinite(inputX) || !std::isfinite(inputZ) ||
        !std::isfinite(dtSeconds) || dtSeconds <= 0.0f) return;
    if (bounds.maxX - bounds.minX < 2.0f * radius_ ||
        bounds.maxZ - bounds.minZ < 2.0f * radius_) return;

    // Clamp input axes before normalizing. Both keyboard diagonals and analog input work.
    inputX = std::clamp(inputX, -1.0f, 1.0f);
    inputZ = std::clamp(inputZ, -1.0f, 1.0f);
    const float magnitude = std::sqrt(inputX * inputX + inputZ * inputZ);
    if (magnitude < 0.00001f) return;
    if (magnitude > 1.0f) { inputX /= magnitude; inputZ /= magnitude; }
    facing_ = {inputX / std::max(1.0f, magnitude),
               inputZ / std::max(1.0f, magnitude)};

    const PositionXZ old = position_;
    const float dx = inputX * speed_ * dtSeconds;
    const float dz = inputZ * speed_ * dtSeconds;

    // Prevent tunneling through thin obstacles during a slow frame.
    // Advance by at most half the player's collision radius per substep.
    const float distance = std::sqrt(dx * dx + dz * dz);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / (radius_ * 0.5f))));
    const float stepX = dx / static_cast<float>(steps);
    const float stepZ = dz / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        PositionXZ candidate = position_;
        candidate.x = std::clamp(position_.x + stepX,
                                 bounds.minX + radius_, bounds.maxX - radius_);
        if (!collides(candidate, obstacles)) position_.x = candidate.x;

        candidate = position_;
        candidate.z = std::clamp(position_.z + stepZ,
                                 bounds.minZ + radius_, bounds.maxZ - radius_);
        if (!collides(candidate, obstacles)) position_.z = candidate.z;
    }

    moving_ = std::abs(position_.x - old.x) > 1e-5f ||
              std::abs(position_.z - old.z) > 1e-5f;
}

bool PlayerMovement3D::canInteract(PositionXZ target, float range) const {
    if (!std::isfinite(range) || range < 0 ||
        !std::isfinite(target.x) || !std::isfinite(target.z)) return false;
    const float dx = target.x - position_.x;
    const float dz = target.z - position_.z;
    return dx * dx + dz * dz <= range * range;
}

void PlayerMovement3D::reset() {
    position_ = start_;
    facing_ = {0.0f, -1.0f};
    moving_ = false;
}

}  // namespace player3d
