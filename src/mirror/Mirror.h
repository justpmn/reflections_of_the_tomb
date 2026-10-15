
#pragma once

#include <glm/vec2.hpp>

class Mirror {
public:
    Mirror(
        const glm::vec2& center,
        float length,
        float initialAngle = 0.0f,
        float minAngle = -80.0f,
        float maxAngle = 80.0f
    );

    // Rotate the mirror using Q/E controls
    void rotateLeft(float degrees = 5.0f);
    void rotateRight(float degrees = 5.0f);

    // Set the angle while respecting rotation limits
    void setAngle(float degrees);

    // Get mirror properties
    glm::vec2 getCenter() const;
    float getAngle() const;

    // Get the two endpoints of the mirror
    glm::vec2 getEndpointA() const;
    glm::vec2 getEndpointB() const;

    // Get the perpendicular unit normal
    glm::vec2 getNormal() const;

private:
    glm::vec2 center;
    float halfLength;
    float angleDeg;
    float minAngleDeg;
    float maxAngleDeg;
};
