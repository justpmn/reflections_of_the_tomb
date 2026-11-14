
#include "RubyCollision.h"

#include <algorithm>

bool RubyCollision::intersects(
    const BeamSegment& beam,
    const glm::vec3& rubyPosition,
    float rubyRadius)
{
    if (rubyRadius <= 0.0f)
    {
        return false;
    }

    // Direction of the finite beam segment.
    glm::vec3 direction = beam.end - beam.start;

    float lengthSquared =
        glm::dot(direction, direction);

    // Handle a zero-length beam.
    if (lengthSquared < 1e-12f)
    {
        return false;
    }

    // Vector from beam start to ruby center.
    glm::vec3 toRuby = rubyPosition - beam.start;

    // Project ruby center onto the beam.
    float t = glm::dot(toRuby, direction)
              / lengthSquared;

    // Clamp to the finite segment [0, 1].
    t = std::clamp(t, 0.0f, 1.0f);

    // Find the closest point on the beam.
    glm::vec3 closestPoint =
        beam.start + t * direction;

    // Calculate squared distance to ruby center.
    glm::vec3 difference =
        rubyPosition - closestPoint;

    float distanceSquared =
        glm::dot(difference, difference);

    return distanceSquared <= rubyRadius * rubyRadius;
}
