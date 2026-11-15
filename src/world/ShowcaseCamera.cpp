#include "ShowcaseCamera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace world {

ShowcaseCamera ShowcaseCamera::fitLevel(const Level& L, const glm::mat4& levelToWorld,
                                        float fovYDeg, float aspect, float margin) {
    ShowcaseCamera cam;
    cam.fovYDeg = fovYDeg;
    cam.aspect  = aspect;
    cam.margin  = margin;

    // Bounding box of the level (floor to wall top) in world space.
    const glm::vec3 size(L.width * L.cellSize, L.wallHeight, L.height * L.cellSize);
    cam.target = glm::vec3(0.0f);
    for (int i = 0; i < 8; ++i) {
        glm::vec3 c((i & 1) ? size.x : 0.0f, (i & 2) ? size.y : 0.0f, (i & 4) ? size.z : 0.0f);
        cam.corners[i] = glm::vec3(levelToWorld * glm::vec4(c, 1.0f));
        cam.target += cam.corners[i] / 8.0f;
    }

    // The bounding sphere seen through the narrower field of view always fits:
    // it is the upper bound for fitDistance()'s search.
    float radius = 0.0f;
    for (const glm::vec3& c : cam.corners) radius = std::max(radius, glm::length(c - cam.target));
    float halfY = glm::radians(fovYDeg) * 0.5f;
    float halfX = std::atan(std::tan(halfY) * aspect);
    cam.maxDistance = radius / std::sin(std::min(halfX, halfY)) / margin;
    return cam;
}

void ShowcaseCamera::orbit(float dYawDeg, float dPitchDeg) {
    yawDeg = std::fmod(yawDeg + dYawDeg, 360.0f);
    pitchDeg = std::clamp(pitchDeg + dPitchDeg, kMinPitch, kMaxPitch);
}

void ShowcaseCamera::zoomBy(float factor) { zoom = std::clamp(zoom * factor, kMinZoom, kMaxZoom); }

void ShowcaseCamera::reset() {
    ShowcaseCamera d;
    yawDeg = d.yawDeg;
    pitchDeg = d.pitchDeg;
    zoom = d.zoom;
}

glm::vec3 ShowcaseCamera::eyeAt(float dist) const {
    float y = glm::radians(yawDeg), p = glm::radians(pitchDeg);
    return target + dist * glm::vec3(std::cos(p) * std::sin(y), std::sin(p), std::cos(p) * std::cos(y));
}

bool ShowcaseCamera::fits(float dist) const {
    glm::mat4 vp = glm::perspective(glm::radians(fovYDeg), aspect, 0.1f, 100.0f) *
                   glm::lookAt(eyeAt(dist), target, glm::vec3(0, 1, 0));
    for (const glm::vec3& c : corners) {
        glm::vec4 clip = vp * glm::vec4(c, 1.0f);
        if (clip.w <= 0.0f) return false;
        if (std::abs(clip.x / clip.w) > margin || std::abs(clip.y / clip.w) > margin) return false;
    }
    return true;
}

float ShowcaseCamera::fitDistance() const {
    // Moving away only shrinks the level on screen, so binary search works.
    float lo = 0.0f, hi = maxDistance;
    for (int it = 0; it < 30; ++it) {
        float mid = 0.5f * (lo + hi);
        (fits(mid) ? hi : lo) = mid;
    }
    return hi;
}

glm::vec3 ShowcaseCamera::eye() const { return eyeAt(distance()); }

glm::mat4 ShowcaseCamera::view() const { return glm::lookAt(eye(), target, glm::vec3(0, 1, 0)); }

glm::vec2 ShowcaseCamera::towardCamera() const {
    float y = glm::radians(yawDeg);
    return {std::sin(y), std::cos(y)};
}

bool cutawayNeeded(const Level& L, float pitchDeg) {
    return L.wallHeight / std::tan(glm::radians(pitchDeg)) > L.cellSize;
}

} // namespace world
