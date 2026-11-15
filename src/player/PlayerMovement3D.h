#pragma once

#include <vector>

namespace player3d {

// World-space XZ coordinates. Y is vertical and remains controlled by the scene.
struct PositionXZ {
    float x = 0.0f;
    float z = 0.0f;
};

// Axis-aligned obstacle, specified in world-space coordinates.
struct ObstacleXZ {
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

struct RoomBoundsXZ {
    float minX = -4.0f;
    float maxX = 4.0f;
    float minZ = -4.0f;
    float maxZ = 4.0f;
};

class PlayerMovement3D {
public:
    PlayerMovement3D(PositionXZ start = {}, float speed = 2.5f, float radius = 0.25f);

    // inputX: A=-1, D=+1; inputZ: W=-1, S=+1.
    // Inputs are normalized to avoid faster diagonal movement.
    // Collision checks happen separately along X and Z to allow wall sliding.
    void update(float inputX, float inputZ, float dtSeconds,
                RoomBoundsXZ bounds,
                const std::vector<ObstacleXZ>& obstacles = {});

    bool canInteract(PositionXZ target, float range = 1.0f) const;
    void reset();

    PositionXZ position() const { return position_; }
    PositionXZ facing() const { return facing_; }
    bool isMoving() const { return moving_; }
    float radius() const { return radius_; }

private:
    bool collides(PositionXZ proposed, const std::vector<ObstacleXZ>& obstacles) const;

    PositionXZ start_;
    PositionXZ position_;
    PositionXZ facing_{0.0f, -1.0f};
    float speed_ = 2.5f;
    float radius_ = 0.25f;
    bool moving_ = false;
};

}  // namespace player3d
