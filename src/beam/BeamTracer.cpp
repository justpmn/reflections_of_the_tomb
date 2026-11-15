
#include "BeamTracer.h"

#include "BeamSystem.h"
#include "physics/Intersections.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float EPSILON = 0.00001f;

    enum class HitType
    {
        None,
        Mirror,
        Wall,
        Ruby
    };

    struct NearestHit
    {
        HitType type = HitType::None;

        float distance =
            std::numeric_limits<float>::infinity();

        glm::vec2 point{0.0f};

        const Mirror* mirror = nullptr;
        int rubyIndex = -1;
    };

    glm::vec3 to3D(
        const glm::vec2& point,
        float height)
    {
        return {point.x, height, point.y};
    }

    // Ray-sphere intersection for a horizontal light ray.
    // Returns the FIRST contact with the ruby surface.
    bool rayRuby(
        const glm::vec2& origin,
        const glm::vec2& direction,
        float beamHeight,
        const RubyTarget& ruby,
        float& distance,
        glm::vec2& hitPoint)
    {
        if (ruby.radius <= 0.0f)
            return false;

        glm::vec3 start = to3D(origin, beamHeight);

        // 'direction' is normalized by the caller.
        glm::vec3 D(direction.x, 0.0f, direction.y);
        glm::vec3 offset = start - ruby.position;

        float b = glm::dot(offset, D);

        float c = glm::dot(offset, offset)
                  - ruby.radius * ruby.radius;

        float discriminant = b * b - c;

        if (discriminant < 0.0f)
            return false;

        float t = -b - std::sqrt(discriminant);

        // A source already inside the ruby is absorbed
        // immediately at its starting position.
        if (t < 0.0f)
        {
            if (c > 0.0f)
                return false;

            t = 0.0f;
        }

        distance = t;
        hitPoint = origin + direction * t;
        return true;
    }
}

std::vector<BeamSegment> BeamTracer::trace(
    const glm::vec2& origin,
    const glm::vec2& direction,
    const std::vector<Mirror>& mirrors,
    std::size_t maxBounces,
    float maxDistance,
    float beamHeight,
    const std::vector<WallSegment>& walls)
{
    // Existing callers continue to work unchanged.
    return traceWithRubies(
        origin,
        direction,
        mirrors,
        {},
        maxBounces,
        maxDistance,
        beamHeight,
        walls
    ).path;
}

BeamTraceResult BeamTracer::traceWithRubies(
    const glm::vec2& origin,
    const glm::vec2& direction,
    const std::vector<Mirror>& mirrors,
    const std::vector<RubyTarget>& rubies,
    std::size_t maxBounces,
    float maxDistance,
    float beamHeight,
    const std::vector<WallSegment>& walls)
{
    BeamTraceResult result;

    if (maxDistance <= 0.0f ||
        glm::dot(direction, direction) < 1e-12f)
    {
        return result;
    }

    glm::vec2 currentPosition = origin;
    glm::vec2 currentDirection =
        glm::normalize(direction);

    float remainingDistance = maxDistance;
    std::size_t bounceCount = 0;

    while (remainingDistance > EPSILON)
    {
        NearestHit nearest;

        // ------------------------------------
        // 1. Search mirrors.
        // ------------------------------------
        for (const Mirror& mirror : mirrors)
        {
            float distance = 0.0f;
            glm::vec2 point(0.0f);

            bool hit = Intersections::raySegment2D(
                currentPosition,
                currentDirection,
                mirror.getEndpointA(),
                mirror.getEndpointB(),
                distance,
                point
            );

            if (hit &&
                distance <= remainingDistance &&
                distance < nearest.distance)
            {
                nearest.type = HitType::Mirror;
                nearest.distance = distance;
                nearest.point = point;
                nearest.mirror = &mirror;
                nearest.rubyIndex = -1;
            }
        }

        // ------------------------------------
        // 2. Search walls.
        // Walls win exact or near-equal ties.
        // ------------------------------------
        for (const WallSegment& wall : walls)
        {
            float distance = 0.0f;
            glm::vec2 point(0.0f);

            bool hit = Intersections::raySegment2D(
                currentPosition,
                currentDirection,
                wall.start,
                wall.end,
                distance,
                point
            );

            if (!hit || distance > remainingDistance)
                continue;

            bool closer =
                distance < nearest.distance - EPSILON;

            bool wallWinsTie =
                nearest.type == HitType::Mirror &&
                std::abs(distance - nearest.distance)
                    <= EPSILON;

            if (nearest.type == HitType::None ||
                closer || wallWinsTie)
            {
                nearest.type = HitType::Wall;
                nearest.distance = distance;
                nearest.point = point;
                nearest.mirror = nullptr;
                nearest.rubyIndex = -1;
            }
        }

        // ------------------------------------
        // 3. Search ruby targets.
        // Rubies absorb only if they are closer
        // than any blocking surface.
        // ------------------------------------
        for (std::size_t i = 0; i < rubies.size(); ++i)
        {
            float distance = 0.0f;
            glm::vec2 point(0.0f);

            bool hit = rayRuby(
                currentPosition,
                currentDirection,
                beamHeight,
                rubies[i],
                distance,
                point
            );

            if (!hit || distance > remainingDistance)
                continue;

            bool closer =
                nearest.type == HitType::None ||
                distance < nearest.distance - EPSILON;

            if (closer)
            {
                nearest.type = HitType::Ruby;
                nearest.distance = distance;
                nearest.point = point;
                nearest.mirror = nullptr;
                nearest.rubyIndex = static_cast<int>(i);
            }
        }

        // ------------------------------------
        // 4. No hit: continue until max distance.
        // ------------------------------------
        if (nearest.type == HitType::None)
        {
            glm::vec2 end =
                currentPosition +
                currentDirection * remainingDistance;

            result.path.push_back({
                to3D(currentPosition, beamHeight),
                to3D(end, beamHeight)
            });

            break;
        }

        // ------------------------------------
        // 5. Stop the current segment at
        // the nearest physical intersection.
        // ------------------------------------
        result.path.push_back({
            to3D(currentPosition, beamHeight),
            to3D(nearest.point, beamHeight)
        });

        remainingDistance -= nearest.distance;

        // ------------------------------------
        // 6. Ruby absorption.
        // ------------------------------------
        if (nearest.type == HitType::Ruby)
        {
            result.hitRubyIndex = nearest.rubyIndex;
            break;
        }

        // ------------------------------------
        // 7. Wall absorption.
        // ------------------------------------
        if (nearest.type == HitType::Wall)
            break;

        // ------------------------------------
        // 8. Mirror reflection.
        // ------------------------------------
        if (bounceCount >= maxBounces ||
            remainingDistance <= EPSILON)
        {
            break;
        }

        glm::vec2 normal =
            nearest.mirror->getNormal();

        glm::vec3 incoming3D(
            currentDirection.x,
            0.0f,
            currentDirection.y
        );

        glm::vec3 normal3D(
            normal.x,
            0.0f,
            normal.y
        );

        glm::vec3 reflected =
            BeamSystem::reflectDirection(
                incoming3D,
                normal3D
            );

        currentDirection = glm::normalize(
            glm::vec2(reflected.x, reflected.z)
        );

        currentPosition = nearest.point;
        ++bounceCount;
    }

    return result;
}
