#include "BeamPlayerCollision.h"

#include <algorithm>
#include <cmath>

bool BeamPlayerCollision::intersects(
    const std::vector<BeamSegment>& path,
    const glm::vec3& playerFeet,
    float playerRadius,
    float playerHeight)
{
    constexpr float EPSILON = 0.00001f;

    if (playerRadius <= 0.0f || playerHeight <= 0.0f)
        return false;

    glm::vec2 playerXZ(playerFeet.x, playerFeet.z);

    for (const BeamSegment& beam : path)
    {
        // The current beam tracer produces horizontal beams.
        if (std::abs(beam.start.y - beam.end.y) > EPSILON)
            continue;

        const float beamHeight = beam.start.y;

        // The beam passes below the feet or above the head.
        if (beamHeight < playerFeet.y - EPSILON ||
            beamHeight > playerFeet.y + playerHeight + EPSILON)
        {
            continue;
        }

        // Project the beam onto the horizontal X/Z plane.
        glm::vec2 start(beam.start.x, beam.start.z);
        glm::vec2 end(beam.end.x, beam.end.z);

        glm::vec2 direction = end - start;

        float lengthSquared =
            glm::dot(direction, direction);

        // An empty beam segment cannot cause a hit.
        if (lengthSquared < EPSILON * EPSILON)
            continue;

        // Find the closest point on the finite beam segment
        // to the player's horizontal position.
        float t = glm::dot(
            playerXZ - start,
            direction
        ) / lengthSquared;

        t = std::clamp(t, 0.0f, 1.0f);

        glm::vec2 closestPoint =
            start + direction * t;

        glm::vec2 difference =
            playerXZ - closestPoint;

        float distanceSquared =
            glm::dot(difference, difference);

        if (distanceSquared <= playerRadius * playerRadius)
        {
            return true;
        }
    }

    return false;
}
