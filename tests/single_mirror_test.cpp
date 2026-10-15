
#include <iostream>
#include <stdexcept>
#include <cmath>

#include <glm/glm.hpp>

#include "physics/Ray.h"
#include "physics/Intersections.h"
#include "beam/BeamSystem.h"
#include "beam/BeamSegment.h"

int main()
{
    // Light travels along the positive X axis.
    Ray incomingRay{
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    };

    // Mirror at X = 5, angled to redirect
    // the beam along the positive Z axis.
    glm::vec3 mirrorPosition(5.0f, 1.0f, 0.0f);

    glm::vec3 mirrorNormal =
        glm::normalize(glm::vec3(-1.0f, 0.0f, 1.0f));

    float distance = 0.0f;

    // Step 1: Find the intersection.
    bool hit = Intersections::rayPlane(
        incomingRay,
        mirrorPosition,
        mirrorNormal,
        distance
    );

    if (!hit)
    {
        throw std::runtime_error(
            "Expected ray to hit the mirror"
        );
    }

    // Step 2: Calculate the hit point.
    glm::vec3 hitPoint =
        incomingRay.origin +
        incomingRay.direction * distance;

    // Step 3: Calculate the reflected direction.
    glm::vec3 reflectedDirection =
        BeamSystem::reflectDirection(
            incomingRay.direction,
            mirrorNormal
        );

    // Step 4: Store the incoming segment.
    BeamSegment incomingSegment{
        incomingRay.origin,
        hitPoint
    };

    // Step 5: Store the reflected segment.
    BeamSegment reflectedSegment{
        hitPoint,
        hitPoint + reflectedDirection * 4.0f
    };

    // Verify the results.
    glm::vec3 expectedHit(5.0f, 1.0f, 0.0f);
    glm::vec3 expectedDirection(0.0f, 0.0f, 1.0f);
    glm::vec3 expectedEnd(5.0f, 1.0f, 4.0f);

    const float tolerance = 0.0001f;

    if (glm::length(hitPoint - expectedHit) > tolerance)
        throw std::runtime_error("Incorrect hit point");

    if (glm::length(reflectedDirection - expectedDirection) > tolerance)
        throw std::runtime_error("Incorrect reflection");

    if (glm::length(reflectedSegment.end - expectedEnd) > tolerance)
        throw std::runtime_error("Incorrect beam endpoint");

    std::cout << "Mirror intersection: PASSED\n";
    std::cout << "Reflection direction: PASSED\n";
    std::cout << "Beam segment generation: PASSED\n";

    std::cout << "\nIncoming beam:\n";
    std::cout << incomingSegment.start.x << ", "
              << incomingSegment.start.y << ", "
              << incomingSegment.start.z << " -> "
              << incomingSegment.end.x << ", "
              << incomingSegment.end.y << ", "
              << incomingSegment.end.z << '\n';

    std::cout << "\nReflected beam:\n";
    std::cout << reflectedSegment.start.x << ", "
              << reflectedSegment.start.y << ", "
              << reflectedSegment.start.z << " -> "
              << reflectedSegment.end.x << ", "
              << reflectedSegment.end.y << ", "
              << reflectedSegment.end.z << '\n';

    std::cout << "\nSingle-mirror test PASSED!\n";

    return 0;
}
