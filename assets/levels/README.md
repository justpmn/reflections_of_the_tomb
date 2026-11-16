# Levels & Temple Environment — Zahra Isaiho

## What's in here
| File | Purpose |
|---|---|
| `src/world/Level.h/.cpp` | Level data, text-file loader, validation, solvability check |
| `src/world/TempleMesh.h/.cpp` | Builds floor / wall / ceiling / pillar / pit geometry + collision boxes |
| `src/world/LevelManager.h/.cpp` | Loads levels in order, `next()`, `reload()` |
| `src/world/LevelScene.h/.cpp` | Level → list of 3D asset placements `{asset, transform}` + the asset table (no OpenGL) |
| `src/world/LevelSceneRenderer.h/.cpp` | Loads each asset once with `Model` and draws the placements with `Renderer::drawModel` |
| `assets/levels/level01-03.txt` | 3 puzzle levels, increasing difficulty |
| `tests/level_test.cpp` | Automated tests (ctest `levels`), incl. a check that the angle convention matches `Mirror` |
| `tests/level_scene_test.cpp` | Placement counts/positions, cutaway, and every asset file exists with the measured size (ctest `level_scene`) |
| `demos/level_preview.cpp` | Prints a level from above with the solution beam (ctest `level_preview_level0N`) |
| `demos/level_export_obj.cpp` | Writes the plain temple geometry as a Wavefront `.obj` |

The build already compiles these into `reflections_of_the_tomb` and copies this folder to `<build>/levels/`.
The game loads `assets/levels/level01.txt` relative to the working directory, so run it from the repo root:
`.\build\reflections_of_the_tomb.exe`.

## Build & test
```powershell
cmake --build build
ctest --test-dir build --output-on-failure -R level
.\build\level_preview.exe assets\levels\level03.txt
```
Expected `level_test` summary:
```
  The Entrance Hall  (9x7, 194 triangles)
  Hall of Spikes  (11x9, 330 triangles)
  The Inner Sanctum  (13x11, 522 triangles)
All level tests passed
```

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

**Numbering:** entities of each type are numbered in reading order (left→right, top→bottom), starting at 0.

**Rules checked by the loader:** one spawn, ≥1 light/ruby/door, border is only walls/doors, every row the same width, all ruby links valid, mirror angles in [-90,90).

## Shared conventions (tell the team)
- Cell `(col,row)` → world centre `((col+0.5)*cellSize, y, (row+0.5)*cellSize)`. Floor is `y = 0`.
- +X = East, +Z = South, North = −Z.
- 2D gameplay code (`Mirror`, `BeamTracer`) uses `glm::vec2(x, z)`; use `L.cellCenterXZ(cell)`.
- Mirror angle = the same convention as `mirror/Mirror.h`: the surface runs along `(cos a, sin a)` in (x, z).
  Seen from above with north up: **45° = `\`**, **-45° = `/`**. Angles stay inside Mirror's default ±80° limits.
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
| **Joe** (rendering) | `buildTempleMesh(L)` → 5 meshes. Vertex = pos(vec3) normal(vec3) uv(vec2), 32 bytes, CCW. Use `glBufferData` + attribs 0/1/2 with stride `sizeof(world::Vertex)`. A different texture per mesh (floor/walls/ceiling/pillars/pits). |
| **Kimberly** (player) | Spawn: `L.cellCenter(L.playerSpawn)`. Collision: `buildStaticColliders(L)` (AABBs). `L.isWalkable(cell)`, `L.isPit(cell)` → fall/respawn with `levels.reload()`. |
| **Joy** (mirrors) | `L.mirrors[i]`: `cell`, `angleDeg` (start), `rotatable`. Place at `cellCenter(cell, height)`. |
| **Peter** (beams) | `L.lights[i]` (`world::LightSpawn`): `cell` + `dirToVec(dir)`. Blockers: walls & pillars (colliders), closed doors. Use `mirrorNormal()` so the angle convention matches. |
| **Jesse** (puzzle) | `L.rubies`, `L.doors[i].needsRubies`, `L.traps[i].disabledByRuby`. Door opens when all needed rubies are lit; trap off when its ruby is lit. On exit: `levels.next()`. |

## Drawing a level with the repo's assets
`buildLevelScene(L)` returns one placement per thing to draw; `LevelSceneRenderer` draws them.

| Level content | Asset | Placement |
|---|---|---|
| floor cell (not pit / trap) | KayKit `floor_tile_large` | cell centre, top flush with y = 0 |
| visible wall face | KayKit `wall` | one slab per face that touches open space, flush with the cell edge, facing the room; hidden walls and faces towards a door cell are skipped |
| `P` pillar | KayKit `pillar_decorated` | cell centre, half a cell wide, floor to ceiling |
| `D` door | `models/door/temple_door.glb` (team door) | door cell, facing into the temple; if the file fails to load, KayKit `wall_doorway` is drawn in the same spot (`Placement::fallback`) |
| `M` mirror | `models/mirrors/mirror_complete.glb` | cell centre, rotated to its **start** angle (45 = `\`, model +Z = `mirrorNormal(a)`) |
| `R` ruby | `models/ruby/ruby_pedestal_floating.glb` | cell centre |
| `T` trap | `models/traps/spike_trap.glb` | replaces the floor tile |
| torches | KayKit `torch_mounted` | every 6th visible wall face (not cut-away ones), 60% up the wall |

- All paths and measured sizes are in one table: `assetTable()` in `LevelScene.cpp`. `level_scene_test`
  re-measures every file with cgltf and fails if a file is missing or its size changed.
- Sizes are measured **with glTF node transforms applied**, the same way Joe's `Model` (main, PR #16)
  loads them, so multi-part models such as `mirror_complete.glb` and `spike_trap.glb` assemble correctly.
- Textures and lighting come from Joe's `Model`/`Renderer`. `LevelSceneRenderer::colorFor` is only
  used for meshes without a texture.
- Scaling: the largest X/Z extent becomes `footprint × cellSize`. Walls, doorway and pillars are
  stretched to `wallHeight`; the rest scale uniformly.
- `levelToArea(L, centreXZ, halfSize, y)` places the level in the game world.
  main.cpp uses centre (0,0), half-size 6.5 (the temple room's ±6.5 footprint) and floor top y = 0,
  where the Ranger stands.
- `LevelSceneOptions::cutawayToward`: walls whose front faces away from that direction (towards the
  camera) are drawn at 25% height. In showcase mode main.cpp points it at the camera and turns it off
  when the camera is high enough to see over the walls.

### Showcase mode (main.cpp)
`constexpr bool kShowcaseLevelOnly = true;` draws level01 instead of the hard-coded temple room,
hides the room's puzzle props and beam, and uses an orbit camera that fits the level.
Controls: ←/→ orbit, ↑/↓ raise/lower (15°–85°), `+`/`-` zoom, `C` reset; WASD still moves the Ranger.
**F1** toggles between the level showcase and the temple room at runtime (Joe's fixed camera is restored).
Set the flag to `false` to start in the temple room demo exactly as on main.

```cpp
world::LevelManager levels({"assets/levels/level01.txt", "assets/levels/level02.txt", "assets/levels/level03.txt"});
levels.loadFirst();
world::LevelScene scene = world::buildLevelScene(levels.current());
world::LevelSceneRenderer sceneRenderer;
sceneRenderer.load();                       // after the GL context exists
// each frame, after renderer.beginFrame():
sceneRenderer.draw(renderer, scene, world::levelToArea(levels.current(), {0, 0}, 4.0f));
```

## The 3 levels
1. **The Entrance Hall**: 1 mirror, 1 ruby. Teaches rotating a mirror.
2. **Hall of Spikes**: 2 mirrors in a chain; the ruby also turns off the spike row.
3. **The Inner Sanctum**: 2 lights, 3 mirrors, 2 rubies; the door needs both. Pits and spikes add hazards.

Every mirror starts at the wrong angle (a test checks the puzzle isn't already solved).

## Graphics concepts used (for the report)
- **Polygon representation**: the temple is built from quads (2 triangles each) generated from the grid.
- **Hidden-surface reduction**: wall faces are only made where a wall touches open space.
- **Surface normals & winding**: each quad's winding is fixed using the cross product, `(p1-p0)×(p2-p0)·n > 0`.
- **Reflection**: the solvability check uses `R = D - 2(D·N)N`.
- **World/grid transforms**: grid↔world coordinate mapping, scaled by `cellSize`.
- **Collision**: axis-aligned bounding boxes (AABBs) for walls and pillars.
