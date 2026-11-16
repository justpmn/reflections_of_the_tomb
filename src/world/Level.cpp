#include "Level.h"

#include <cmath>
#include <fstream>
#include <set>
#include <sstream>

namespace world {

// ---------------- small helpers ----------------

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

glm::vec3 dirToVec(Dir d) {
    switch (d) {
        case Dir::N: return {0, 0, -1};
        case Dir::E: return {1, 0, 0};
        case Dir::S: return {0, 0, 1};
        case Dir::W: return {-1, 0, 0};
    }
    return {1, 0, 0};
}

bool parseDir(const std::string& s, Dir& out) {
    if (s == "N") { out = Dir::N; return true; }
    if (s == "E") { out = Dir::E; return true; }
    if (s == "S") { out = Dir::S; return true; }
    if (s == "W") { out = Dir::W; return true; }
    return false;
}

glm::vec3 mirrorNormal(float angleDeg) {
    float a = glm::radians(angleDeg);
    // Same as Mirror::getNormal(): surface along (cos a, 0, sin a), normal (-sin a, 0, cos a).
    return glm::normalize(glm::vec3(-std::sin(a), 0.0f, std::cos(a)));
}

// ---------------- Level queries ----------------

Tile Level::tileAt(glm::ivec2 c) const {
    if (!inBounds(c)) return Tile::Wall;
    return tiles[c.y * width + c.x];
}

glm::vec3 Level::cellCenter(glm::ivec2 c, float y) const {
    return {(c.x + 0.5f) * cellSize, y, (c.y + 0.5f) * cellSize};
}

glm::vec2 Level::cellCenterXZ(glm::ivec2 c) const {
    return {(c.x + 0.5f) * cellSize, (c.y + 0.5f) * cellSize};
}

glm::ivec2 Level::worldToCell(const glm::vec3& p) const {
    return {(int)std::floor(p.x / cellSize), (int)std::floor(p.z / cellSize)};
}

template <typename T>
static int findAt(const std::vector<T>& v, glm::ivec2 c) {
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i].cell == c) return (int)i;
    return -1;
}

int Level::mirrorAt(glm::ivec2 c) const { return findAt(mirrors, c); }
int Level::rubyAt(glm::ivec2 c) const   { return findAt(rubies, c); }
int Level::doorAt(glm::ivec2 c) const   { return findAt(doors, c); }
int Level::lightAt(glm::ivec2 c) const  { return findAt(lights, c); }
int Level::trapAt(glm::ivec2 c) const   { return findAt(traps, c); }

bool Level::isWalkable(glm::ivec2 c) const {
    if (tileAt(c) != Tile::Floor) return false;
    return mirrorAt(c) < 0 && rubyAt(c) < 0 && lightAt(c) < 0;
}

// ---------------- Loader ----------------
//
// File format (see assets/levels/README.md):
//   name: <text>
//   cell_size: 2.0          (optional)
//   wall_height: 3.0        (optional)
//   grid:
//   #########
//   ...
//   end
//   light  <i> dir <N|E|S|W>
//   mirror <i> angle <deg> [solution <deg>] [fixed]
//   door   <i> needs <ruby> [<ruby> ...]
//   traps disabled_by <ruby>          (all traps)
//   trap   <i> disabled_by <ruby>     (one trap)
//   // comment

LoadResult loadLevelFromString(const std::string& text) {
    LoadResult res;
    Level& L = res.level;
    std::istringstream in(text);
    std::string raw;
    std::vector<std::string> gridRows;
    std::vector<std::pair<int, std::string>> props;   // (line number, text)
    bool inGrid = false;
    int lineNo = 0;

    auto err = [&](const std::string& m) {
        res.errors.push_back("line " + std::to_string(lineNo) + ": " + m);
    };

    while (std::getline(in, raw)) {
        ++lineNo;
        std::string line = trim(raw);

        if (inGrid) {
            if (line == "end") { inGrid = false; continue; }
            if (!line.empty()) gridRows.push_back(line);
            continue;
        }
        if (line.empty() || line.rfind("//", 0) == 0) continue;

        if (line.rfind("name:", 0) == 0)        { L.name = trim(line.substr(5)); continue; }
        if (line.rfind("cell_size:", 0) == 0)   { L.cellSize = std::stof(line.substr(10)); continue; }
        if (line.rfind("wall_height:", 0) == 0) { L.wallHeight = std::stof(line.substr(12)); continue; }
        if (line.rfind("pit_depth:", 0) == 0)   { L.pitDepth = std::stof(line.substr(10)); continue; }
        if (line == "grid:") {
            inGrid = true;
            continue;
        }
        // Entity property lines need the grid built first, so keep them for pass 2.
        props.push_back({lineNo, line});
    }
    if (inGrid) err("grid: block has no 'end'");

    // ---- Build grid ----
    if (gridRows.empty()) {
        res.errors.push_back("no grid rows");
        return res;
    }
    L.height = (int)gridRows.size();
    L.width  = (int)gridRows[0].size();
    L.tiles.assign(L.width * L.height, Tile::Wall);
    int spawnCount = 0;

    for (int r = 0; r < L.height; ++r) {
        if ((int)gridRows[r].size() != L.width) {
            res.errors.push_back("grid row " + std::to_string(r) + " has width " +
                                 std::to_string(gridRows[r].size()) + ", expected " +
                                 std::to_string(L.width));
            continue;
        }
        for (int c = 0; c < L.width; ++c) {
            char ch = gridRows[r][c];
            glm::ivec2 cell{c, r};
            Tile t = Tile::Floor;
            switch (ch) {
                case '#': t = Tile::Wall; break;
                case '.': t = Tile::Floor; break;
                case '_': t = Tile::Pit; break;
                case 'P': t = Tile::Pillar; break;
                case 'S': L.playerSpawn = cell; ++spawnCount; break;
                case 'L': L.lights.push_back({cell, Dir::E}); break;
                case 'M': L.mirrors.push_back({cell}); break;
                case 'R': L.rubies.push_back({cell}); break;
                case 'D': L.doors.push_back({cell, {}}); break;
                case 'T': L.traps.push_back({cell}); break;
                default:
                    res.errors.push_back("unknown grid char '" + std::string(1, ch) + "' at (" +
                                         std::to_string(c) + "," + std::to_string(r) + ")");
            }
            L.tiles[r * L.width + c] = t;
        }
    }
    if (spawnCount != 1)
        res.errors.push_back("need exactly one 'S' player spawn, found " + std::to_string(spawnCount));

    // ---- Apply property lines ----
    for (auto& [ln, line] : props) {
        lineNo = ln;
        std::istringstream ss(line);
        std::string kind;
        ss >> kind;

        auto badIndex = [&](const std::string& what, int i, size_t n) {
            if (i < 0 || i >= (int)n) {
                err(what + " index " + std::to_string(i) + " out of range (have " + std::to_string(n) + ")");
                return true;
            }
            return false;
        };

        if (kind == "light") {
            int i; std::string key, d;
            ss >> i >> key >> d;
            if (badIndex("light", i, L.lights.size())) continue;
            if (key != "dir" || !parseDir(d, L.lights[i].dir)) err("expected: light <i> dir <N|E|S|W>");
        } else if (kind == "mirror") {
            int i; ss >> i;
            if (badIndex("mirror", i, L.mirrors.size())) continue;
            std::string key;
            while (ss >> key) {
                if (key == "angle")         ss >> L.mirrors[i].angleDeg;
                else if (key == "solution") { ss >> L.mirrors[i].solutionDeg; L.mirrors[i].hasSolution = true; }
                else if (key == "alternate") { ss >> L.mirrors[i].alternateSolutionDeg; L.mirrors[i].hasAlternateSolution = true; }
                else if (key == "fixed")    L.mirrors[i].rotatable = false;
                else err("unknown mirror option '" + key + "'");
            }
        } else if (kind == "door") {
            int i; std::string key;
            ss >> i >> key;
            if (badIndex("door", i, L.doors.size())) continue;
            if (key != "needs") { err("expected: door <i> needs <ruby...>"); continue; }
            int r;
            while (ss >> r) L.doors[i].needsRubies.push_back(r);
        } else if (kind == "traps") {
            std::string key; int r;
            ss >> key >> r;
            if (key != "disabled_by") { err("expected: traps disabled_by <ruby>"); continue; }
            for (auto& t : L.traps) t.disabledByRuby = r;
        } else if (kind == "trap") {
            int i=-1; std::string key; ss >> i >> key;
            if (badIndex("trap", i, L.traps.size())) continue;
            auto& t=L.traps[i];
            if(key=="disabled_by") { if(!(ss>>t.disabledByRuby)) err("missing ruby index"); }
            else if(key=="timed") {
                t.timed=true;
                if(!(ss>>t.safeSeconds>>t.warningSeconds>>t.extendedSeconds>>t.phaseOffset)) err("expected trap <i> timed <safe> <warning> <extended> <offset>");
                std::string option;
                if(ss>>option) {if(option!="disabled_by"||!(ss>>t.disabledByRuby))err("expected disabled_by <ruby>");}
            } else err("expected disabled_by or timed trap properties");
        } else {
            err("unknown line '" + line + "'");
        }
    }

    auto v = validateLevel(L);
    res.errors.insert(res.errors.end(), v.begin(), v.end());
    res.ok = res.errors.empty();
    return res;
}

LoadResult loadLevelFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        LoadResult r;
        r.errors.push_back("cannot open level file: " + path);
        return r;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    LoadResult r = loadLevelFromString(buf.str());
    for (auto& e : r.errors) e = path + ": " + e;
    return r;
}

// ---------------- Validation ----------------

std::vector<std::string> validateLevel(const Level& L) {
    std::vector<std::string> e;
    if (L.name.empty()) e.push_back("level has no name");
    if (L.lights.empty()) e.push_back("level needs at least one light 'L'");
    if (L.rubies.empty()) e.push_back("level needs at least one ruby 'R'");
    if (L.doors.empty())  e.push_back("level needs at least one exit door 'D'");

    // Border must be closed: only walls or doors on the edge.
    for (int r = 0; r < L.height; ++r)
        for (int c = 0; c < L.width; ++c) {
            bool edge = r == 0 || c == 0 || r == L.height - 1 || c == L.width - 1;
            if (!edge) continue;
            glm::ivec2 cell{c, r};
            if (L.tileAt(cell) != Tile::Wall && L.doorAt(cell) < 0)
                e.push_back("border cell (" + std::to_string(c) + "," + std::to_string(r) +
                            ") must be a wall or door");
        }

    for (size_t i = 0; i < L.doors.size(); ++i) {
        if (L.doors[i].needsRubies.empty())
            e.push_back("door " + std::to_string(i) + " has no 'needs' rubies");
        for (int r : L.doors[i].needsRubies)
            if (r < 0 || r >= (int)L.rubies.size())
                e.push_back("door " + std::to_string(i) + " needs missing ruby " + std::to_string(r));
    }
    for (size_t i = 0; i < L.traps.size(); ++i) {
        const auto& t=L.traps[i];
        if(t.timed&&(!std::isfinite(t.safeSeconds)||!std::isfinite(t.warningSeconds)||!std::isfinite(t.extendedSeconds)||!std::isfinite(t.phaseOffset)||t.safeSeconds<1.5f||t.warningSeconds<.75f||t.extendedSeconds<.8f||t.phaseOffset<0))
            e.push_back("invalid timed trap durations/phase");
        int r = t.disabledByRuby;
        if (r < -1 || r >= (int)L.rubies.size())
            e.push_back("trap " + std::to_string(i) + " disabled_by missing ruby " + std::to_string(r));
    }
    for (size_t i = 0; i < L.mirrors.size(); ++i) {
        const MirrorSpawn& m = L.mirrors[i];
        if (m.angleDeg < -90.0f || m.angleDeg >= 90.0f)
            e.push_back("mirror " + std::to_string(i) + " angle must be in [-90,90)");
        if (m.hasSolution && (m.solutionDeg < -90.0f || m.solutionDeg >= 90.0f))
            e.push_back("mirror " + std::to_string(i) + " solution must be in [-90,90)");
    }
    return e;
}

// ---------------- Design-time beam check ----------------

GridBeam traceBeamOnGrid(const Level& L, int lightIndex, bool useSolution) {
    GridBeam out;
    if (lightIndex < 0 || lightIndex >= (int)L.lights.size()) return out;

    glm::ivec2 cell = L.lights[lightIndex].cell;
    glm::vec3 D = dirToVec(L.lights[lightIndex].dir);
    const int maxSteps = L.width * L.height * 4;   // guards against loops

    for (int step = 0; step < maxSteps; ++step) {
        cell += glm::ivec2((int)std::round(D.x), (int)std::round(D.z));
        Tile t = L.tileAt(cell);
        if (t == Tile::Wall || t == Tile::Pillar) break;   // blocked
        if (L.doorAt(cell) >= 0 || L.lightAt(cell) >= 0) break;

        out.cells.push_back(cell);

        int r = L.rubyAt(cell);
        if (r >= 0) { out.hitRuby = r; break; }

        int m = L.mirrorAt(cell);
        if (m >= 0) {
            const MirrorSpawn& ms = L.mirrors[m];
            float a = (useSolution && ms.hasSolution) ? ms.solutionDeg : ms.angleDeg;
            glm::vec3 N = mirrorNormal(a);
            D = D - 2.0f * glm::dot(D, N) * N;            // R = D - 2(D.N)N
            // Grid walk only supports axis-aligned results (+-45 mirrors).
            if (std::abs(std::abs(D.x) + std::abs(D.z) - 1.0f) > 1e-3f) break;
        }
    }
    return out;
}

bool isSolvable(const Level& L, std::vector<std::string>* why) {
    std::set<int> lit;
    // Permanent sealing permits a solution sequence. Alternate designer poses
    // are verification metadata; runtime gameplay still uses continuous tracing.
    std::vector<Level> poses{L};
    for(size_t i=0;i<L.mirrors.size();++i) if(L.mirrors[i].hasAlternateSolution) {
        const auto count=poses.size();
        for(size_t p=0;p<count;++p) {
            auto alternative=poses[p];alternative.mirrors[i].solutionDeg=L.mirrors[i].alternateSolutionDeg;
            poses.push_back(std::move(alternative));
        }
    }
    for(const auto& pose:poses) for(int i=0;i<(int)pose.lights.size();++i) {
        const auto beam=traceBeamOnGrid(pose,i,true);
        if(beam.hitRuby>=0) lit.insert(beam.hitRuby);
    }
    bool ok = true;
    for (size_t d = 0; d < L.doors.size(); ++d)
        for (int r : L.doors[d].needsRubies)
            if (!lit.count(r)) {
                ok = false;
                if (why) why->push_back("door " + std::to_string(d) + ": ruby " +
                                        std::to_string(r) + " is never lit by the solution");
            }
    return ok;
}

} // namespace world
