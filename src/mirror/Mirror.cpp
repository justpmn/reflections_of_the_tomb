
#include "Mirror.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
    constexpr float PI = 3.14159265358979323846f;

    float toRadians(float degrees) {
        return degrees * PI / 180.0f;
    }
}

Mirror::Mirror(
    const glm::vec2& center,
    float length,
    float initialAngle,
    float minAngle,
    float maxAngle
)
    : center(center),
      halfLength(length / 2.0f),
      angleDeg(initialAngle),
      minAngleDeg(minAngle),
      maxAngleDeg(maxAngle) {

    if (length <= 0.0f || minAngle > maxAngle) {
        throw std::invalid_argument("Invalid mirror length or angle limits");
    }

    setAngle(initialAngle);
}

void Mirror::rotateLeft(float degrees) {
    setAngle(angleDeg - std::abs(degrees));
}

void Mirror::rotateRight(float degrees) {
    setAngle(angleDeg + std::abs(degrees));
}

void Mirror::setAngle(float degrees) {
    angleDeg = std::clamp(degrees, minAngleDeg, maxAngleDeg);
}

glm::vec2 Mirror::getCenter() const {
    return center;
}

float Mirror::getAngle() const {
    return angleDeg;
}

glm::vec2 Mirror::getEndpointA() const {
    const float angle = toRadians(angleDeg);

    return {
        center.x - halfLength * std::cos(angle),
        center.y - halfLength * std::sin(angle)
    };
}

glm::vec2 Mirror::getEndpointB() const {
    const float angle = toRadians(angleDeg);

    return {
        center.x + halfLength * std::cos(angle),
        center.y + halfLength * std::sin(angle)
    };
}

glm::vec2 Mirror::getNormal() const {
    const float angle = toRadians(angleDeg);

    return {
        -std::sin(angle),
        std::cos(angle)
    };
}
