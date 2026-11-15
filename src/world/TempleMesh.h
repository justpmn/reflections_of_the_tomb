#pragma once
// Builds temple geometry (floor, walls, ceiling, pillars, pits) from a Level.
// Owner: Zahra Isaiho. Output is plain vertex/index arrays so the rendering
// module can upload them with glBufferData - no OpenGL calls in here.
//
// Vertex layout (interleaved, 8 floats = 32 bytes):
//   location 0: vec3 position
//   location 1: vec3 normal
//   location 2: vec2 uv      (1 UV unit = 1 grid cell, so textures tile per cell)
// Winding: counter-clockwise when looking at the front face (OpenGL default).

#include <vector>
#include <glm/glm.hpp>
#include "Level.h"

namespace world {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct MeshData {
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    size_t triangleCount() const { return indices.size() / 3; }
};

// Separate meshes so each can get its own texture/material.
struct TempleMesh {
    MeshData floor;
    MeshData walls;
    MeshData ceiling;
    MeshData pillars;
    MeshData pits;
};

TempleMesh buildTempleMesh(const Level& lvl);

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Solid boxes for player collision and for beam blocking (walls + pillars).
std::vector<AABB> buildStaticColliders(const Level& lvl);

} // namespace world
