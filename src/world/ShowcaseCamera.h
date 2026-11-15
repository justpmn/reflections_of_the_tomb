#pragma once
// Orbit camera that frames a whole level, for showcase videos / screenshots.
// Owner: Zahra Isaiho. No OpenGL: it only produces a view matrix
// (pass it to Renderer::setViewMatrix) and the cutaway direction for LevelScene.

#include <array>
#include <glm/glm.hpp>
#include "Level.h"

namespace world {

struct ShowcaseCamera {
    static constexpr float kMinPitch = 15.0f, kMaxPitch = 85.0f;
    static constexpr float kMinZoom  = 0.35f, kMaxZoom  = 2.5f;

    glm::vec3 target{0.0f};              // orbit centre (centre of the level's bounding box)
    std::array<glm::vec3, 8> corners{};  // level box (floor to wall top) in world space
    float maxDistance = 1.0f;            // bounding-sphere fit: always shows the whole level
    float fovYDeg = 45.0f;               // must match the renderer's projection
    float aspect  = 16.0f / 9.0f;
    float margin  = 0.95f;               // keep corners inside this fraction of the screen (NDC)

    float yawDeg   = 0.0f;      // 0 = camera south of the level (+Z), looking north
    float pitchDeg = 60.0f;     // elevation above the horizon
    float zoom     = 1.0f;      // 1 = level fills the screen at the current angle, < 1 = closer

    // Frames the level placed in the world by levelToWorld (see levelToArea()).
    static ShowcaseCamera fitLevel(const Level& lvl, const glm::mat4& levelToWorld,
                                   float fovYDeg, float aspect, float margin = 0.95f);

    void orbit(float dYawDeg, float dPitchDeg);   // pitch is clamped
    void zoomBy(float factor);                    // < 1 moves closer, clamped
    void reset();                                 // default yaw / pitch / zoom

    // Closest distance at which the whole level is on screen for the current yaw/pitch
    // (binary search over the 8 box corners). Re-fitted live, so the level fills the
    // frame from every angle.
    float fitDistance() const;
    float distance() const { return zoom * fitDistance(); }
    glm::vec3 eye() const;
    glm::mat4 view() const;
    glm::vec2 towardCamera() const;   // horizontal (x, z) direction from target to eye

private:
    glm::vec3 eyeAt(float dist) const;
    bool fits(float dist) const;
};

// True if a camera at this pitch can't see over the walls: the floor strip hidden
// behind a wall (wallHeight / tan(pitch)) is more than one cell deep.
bool cutawayNeeded(const Level& lvl, float pitchDeg);

} // namespace world
