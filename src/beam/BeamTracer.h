
#pragma once

#include <cstddef>
#include <vector>
#include <glm/glm.hpp>

#include "BeamSegment.h"
#include "mirror/Mirror.h"

struct WallSegment
{
    glm::vec2 start;
    glm::vec2 end;
};

// A spherical ruby target in 3D world coordinates.
struct RubyTarget
{
    glm::vec3 position;
    float radius;
    bool isReal;
};

// Full path plus the first ruby that absorbed the light.
// -1 means no ruby was hit.
struct BeamTraceResult
{
    std::vector<BeamSegment> path;
    int hitRubyIndex = -1;
    std::vector<std::size_t> reflectedMirrorIndices;
};

class BeamTracer
{
public:
    // Existing API: preserved for current tests.
    static std::vector<BeamSegment> trace(
        const glm::vec2& origin,
        const glm::vec2& direction,
        const std::vector<Mirror>& mirrors,
        std::size_t maxBounces = 4,
        float maxDistance = 100.0f,
        float beamHeight = 1.0f,
        const std::vector<WallSegment>& walls = {}
    );

    // New API: mirrors, walls and absorbing rubies.
    static BeamTraceResult traceWithRubies(
        const glm::vec2& origin,
        const glm::vec2& direction,
        const std::vector<Mirror>& mirrors,
        const std::vector<RubyTarget>& rubies,
        std::size_t maxBounces = 4,
        float maxDistance = 100.0f,
        float beamHeight = 1.0f,
        const std::vector<WallSegment>& walls = {}
    );
};
