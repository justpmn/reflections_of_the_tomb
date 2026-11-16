# Levels & Temple Environment â€” Zahra Isaiho

## What's in here
| File | Purpose |
|---|---|
| `src/world/Level.h/.cpp` | Level data, text-file loader, validation, solvability check |
| `src/world/TempleMesh.h/.cpp` | Builds floor / wall / ceiling / pillar / pit geometry + collision boxes |
| `src/world/LevelManager.h/.cpp` | Loads levels in order, `next()`, `reload()` |
| `assets/levels/level01-05.txt` | 5 puzzle levels, increasing difficulty |
| `tests/level_test.cpp` | Automated tests (ctest `levels`), incl. a check that the angle convention matches `Mirror` |
| `demos/level_preview.cpp` | Prints a level from above with the solution beam (ctest `level_preview_level0N`) |

The build already compiles these into `reflections_of_the_tomb` and copies this folder to `<build>/levels/`.

## Build & test
```powershell
cmake --build build
ctest --test-dir build --output-on-failure -R level
.\build\level_preview.exe assets\levels\level03.txt
```
Expected result: all level loader, coordinate, mirror-convention, geometry and manager tests pass. Triangle counts reflect the current authored baffles and decorations.

## Level file format
```
name: The Entrance Hall
cell_size: 2.0          // optional, world units per cell
wall_height: 3.0        // optional
pit_depth: 2.0          // optional
grid:
#########
#.....R.#
#L....M.#
####D####
end
light  0 dir E                       // N, E, S, W
mirror 0 angle 45 solution -45       // add "fixed" if it can't rotate
mirror 1 angle 0 solution 45 alternate -45 // optional sealing sequence
door   0 needs 0                     // door needs ruby 0 (list several: needs 0 1)
traps disabled_by 0                  // all traps; or: trap 2 disabled_by 1
```
| Char | Meaning | Char | Meaning |
|---|---|---|---|
| `#` | wall | `L` | light source |
| `.` | floor | `M` | mirror |
| `_` | pit | `R` | ruby |
| `P` | pillar (blocks player + beam) | `D` | exit door |
| `S` | player spawn (exactly 1) | `T` | spike trap |

**Numbering:** entities of each type are numbered in reading order (leftâ†’right, topâ†’bottom), starting at 0.

**Rules checked by the loader:** one spawn, â‰¥1 light/ruby/door, border is only walls/doors, every row the same width, all ruby links valid, mirror angles in [-90,90).

## Shared conventions (tell the team)
- Cell `(col,row)` â†’ world centre `((col+0.5)*cellSize, y, (row+0.5)*cellSize)`. Floor is `y = 0`.
- +X = East, +Z = South, North = âˆ’Z.
- 2D gameplay code (`Mirror`, `BeamTracer`) uses `glm::vec2(x, z)`; use `L.cellCenterXZ(cell)`.
- Mirror angle = the same convention as `mirror/Mirror.h`: the surface runs along `(cos a, sin a)` in (x, z).
  Seen from above with north up: **45Â° = `\`**, **-45Â° = `/`**. Angles stay inside Mirror's default Â±80Â° limits.
  `mirrorNormal(a) = (-sin a, 0, cos a)` equals `Mirror::getNormal()` and works with `BeamSystem::reflectDirection`.

## How teammates use it
```cpp
#include "world/LevelManager.h"
#include "world/TempleMesh.h"

world::LevelManager levels({"levels/level01.txt", "levels/level02.txt", "levels/level03.txt"});
if (!levels.loadFirst()) for (auto& e : levels.errors()) std::cerr << e << "\n";
const world::Level& L = levels.current();
```
| Teammate | What they take |
|---|---|
| **Joe** (rendering) | `buildTempleMesh(L)` â†’ 5 meshes. Vertex = pos(vec3) normal(vec3) uv(vec2), 32 bytes, CCW. Use `glBufferData` + attribs 0/1/2 with stride `sizeof(world::Vertex)`. A different texture per mesh (floor/walls/ceiling/pillars/pits). |
| **Kimberly** (player) | Spawn: `L.cellCenter(L.playerSpawn)`. Collision: `buildStaticColliders(L)` (AABBs). `L.isWalkable(cell)`, `L.isPit(cell)` â†’ fall/respawn with `levels.reload()`. |
| **Joy** (mirrors) | `L.mirrors[i]`: `cell`, `angleDeg` (start), `rotatable`. Place at `cellCenter(cell, height)`. |
| **Peter** (beams) | `L.lights[i]` (`world::LightSpawn`): `cell` + `dirToVec(dir)`. Blockers: walls & pillars (colliders), closed doors. Use `mirrorNormal()` so the angle convention matches. |
| **Jesse** (puzzle) | `L.rubies`, `L.doors[i].needsRubies`, `L.traps[i].disabledByRuby`. Door opens when all needed rubies are lit; trap off when its ruby is lit. On exit: `levels.next()`. |

## The 5 levels (presentation overhaul)
1. **The Entrance Hall**: one mirror must turn light north to the ruby.
2. **Hall of Spikes**: two mirrors turn light south, then west; a real wall baffle blocks the former diagonal shortcut.
3. **The Inner Sanctum**: two sources, four mirrors, two ruby targets. Independent two-reflection paths are left source -> mirror 0 -> mirror 2 -> ruby 0 and right source -> mirror 1 -> mirror 3 -> ruby 1. Baffles at columns 3/11 block diagonals; a central divider at column 7 blocks cross-circuit shortcuts. The lower passage connects both player routes.

The critical-fix redesign replaces the previous three-mirror sequential sealing step. All four solution angles are +/-45 within the actual [-80,80] Q/E range. Every combination of +/-0.2 degree alignment error around those solutions is tested against the real finite BeamTracer. The original loader, ownership and previous authored designs remain in Git history. See `docs/PUZZLE_VERIFICATION.md` for continuous-angle necessity proofs and native control verification.

## Graphics concepts used (for the report)
- **Polygon representation**: the temple is built from quads (2 triangles each) generated from the grid.
- **Hidden-surface reduction**: wall faces are only made where a wall touches open space.
- **Surface normals & winding**: each quad's winding is fixed using the cross product, `(p1-p0)Ã—(p2-p0)Â·n > 0`.
- **Reflection**: the solvability check uses `R = D - 2(DÂ·N)N`.
- **World/grid transforms**: gridâ†”world coordinate mapping, scaled by `cellSize`.
- **Collision**: axis-aligned bounding boxes (AABBs) for walls and pillars.

## Timed spike authoring

`trap <index> timed <safe-seconds> <warning-seconds> <extended-seconds> <phase-offset> [disabled_by <ruby-index>]`

Each trap owns its phase. Identical parameters/offsets form a synchronized section; different offsets produce independent groups. Safe duration must be at least 1.5 seconds, warning at least 0.75 seconds, extended at least 0.8 seconds, and all numbers finite with nonnegative offsets. Prefer the supplied 4.5-5.5 second safe windows and 1.2 second warnings. Extension/retraction each lasts 0.4 seconds within Extended. Ruby-controlled traps retain the original `disabled_by` format. Timed traps without that explicit link continue after ruby activation.

4. **The Gauntlet:** 15x11, two sources, four mirrors, two rubies, two timed crossing rows and safe lower waiting zones.
5. **The Final Trial:** 22x12, three sources, six mirrors, three rubies, three independently phased crossing rows, three pits and a final ruby-controlled exit barrier.

New bays have source -> upper mirror (+45) -> lower mirror (-45) -> ruby. Stone baffles rule out diagonal shortcuts; dividers isolate optical circuits while a lower passage connects walking routes. All mirrors are physically necessary under the continuous-angle envelope proof, not rotation counters.
