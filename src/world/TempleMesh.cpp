#include "TempleMesh.h"

namespace world {

// Adds a quad (2 triangles). Corners may be given in either order around the
// quad; winding is fixed so the front face points along `n`.
static void addQuad(MeshData& m, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
                    glm::vec3 n, glm::vec2 uvSize) {
    if (glm::dot(glm::cross(p1 - p0, p2 - p0), n) < 0.0f) std::swap(p1, p3);

    unsigned base = (unsigned)m.vertices.size();
    m.vertices.push_back({p0, n, {0.0f, 0.0f}});
    m.vertices.push_back({p1, n, {uvSize.x, 0.0f}});
    m.vertices.push_back({p2, n, {uvSize.x, uvSize.y}});
    m.vertices.push_back({p3, n, {0.0f, uvSize.y}});
    m.indices.insert(m.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

// Vertical face on one side of a cell-sized square, from yBottom to yTop.
// side: 0=N (-Z), 1=E (+X), 2=S (+Z), 3=W (-X). Normal points outward from the square.
static void addSideFace(MeshData& m, float x0, float z0, float x1, float z1,
                        float yBottom, float yTop, int side, bool normalInward, float cs) {
    glm::vec3 a, b;
    glm::vec3 n;
    switch (side) {
        case 0: a = {x0, 0, z0}; b = {x1, 0, z0}; n = {0, 0, -1}; break;
        case 1: a = {x1, 0, z0}; b = {x1, 0, z1}; n = {1, 0, 0};  break;
        case 2: a = {x0, 0, z1}; b = {x1, 0, z1}; n = {0, 0, 1};  break;
        default: a = {x0, 0, z0}; b = {x0, 0, z1}; n = {-1, 0, 0}; break;
    }
    if (normalInward) n = -n;
    glm::vec3 p0 = {a.x, yBottom, a.z}, p1 = {b.x, yBottom, b.z};
    glm::vec3 p2 = {b.x, yTop, b.z},    p3 = {a.x, yTop, a.z};
    float w = glm::length(b - a) / cs;
    float h = (yTop - yBottom) / cs;
    addQuad(m, p0, p1, p2, p3, n, {w, h});
}

static const glm::ivec2 kNeighbour[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};  // N E S W

TempleMesh buildTempleMesh(const Level& L) {
    TempleMesh out;
    const float cs = L.cellSize;
    const float h  = L.wallHeight;

    for (int r = 0; r < L.height; ++r) {
        for (int c = 0; c < L.width; ++c) {
            glm::ivec2 cell{c, r};
            Tile t = L.tileAt(cell);
            float x0 = c * cs, x1 = x0 + cs;
            float z0 = r * cs, z1 = z0 + cs;

            if (t == Tile::Wall) {
                // Only build faces that border open space (hidden faces skipped).
                for (int s = 0; s < 4; ++s) {
                    glm::ivec2 nb = cell + kNeighbour[s];
                    if (L.inBounds(nb) && L.tileAt(nb) != Tile::Wall)
                        addSideFace(out.walls, x0, z0, x1, z1, 0.0f, h, s, false, cs);
                }
                continue;
            }

            // Ceiling over every open cell (faces down).
            addQuad(out.ceiling, {x0, h, z0}, {x1, h, z0}, {x1, h, z1}, {x0, h, z1},
                    {0, -1, 0}, {1, 1});

            if (t == Tile::Pit) {
                float d = -L.pitDepth;
                addQuad(out.pits, {x0, d, z0}, {x1, d, z0}, {x1, d, z1}, {x0, d, z1},
                        {0, 1, 0}, {1, 1});
                for (int s = 0; s < 4; ++s) {
                    glm::ivec2 nb = cell + kNeighbour[s];
                    if (L.tileAt(nb) != Tile::Pit)
                        addSideFace(out.pits, x0, z0, x1, z1, d, 0.0f, s, true, cs);
                }
                continue;
            }

            // Floor (Floor and Pillar cells).
            addQuad(out.floor, {x0, 0, z0}, {x1, 0, z0}, {x1, 0, z1}, {x0, 0, z1},
                    {0, 1, 0}, {1, 1});

            if (t == Tile::Pillar) {
                float q = cs * 0.25f;                 // pillar is half a cell wide
                float px0 = x0 + q, px1 = x1 - q, pz0 = z0 + q, pz1 = z1 - q;
                for (int s = 0; s < 4; ++s)
                    addSideFace(out.pillars, px0, pz0, px1, pz1, 0.0f, h, s, false, cs);
            }
        }
    }
    return out;
}

std::vector<AABB> buildStaticColliders(const Level& L) {
    std::vector<AABB> boxes;
    const float cs = L.cellSize;
    for (int r = 0; r < L.height; ++r)
        for (int c = 0; c < L.width; ++c) {
            Tile t = L.tileAt({c, r});
            float x0 = c * cs, z0 = r * cs;
            if (t == Tile::Wall) {
                boxes.push_back({{x0, 0, z0}, {x0 + cs, L.wallHeight, z0 + cs}});
            } else if (t == Tile::Pillar) {
                float q = cs * 0.25f;
                boxes.push_back({{x0 + q, 0, z0 + q}, {x0 + cs - q, L.wallHeight, z0 + cs - q}});
            }
        }
    return boxes;
}

} // namespace world
