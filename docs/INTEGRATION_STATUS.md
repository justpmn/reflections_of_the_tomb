# Integration status

Updated 16 November 2026, Africa/Nairobi. Work remains exclusively on `integration`. No push or merge into `main`.

## Rollback checkpoint

Before changing rendering or levels, preserved local tag **`integration-before-presentation-overhaul`** at **`fbed5c5`**. The previous runnable package remains at `build-desktop/rollback-before-overhaul`. The overnight implementation and team contributions remain in Git history.

## Presentation overhaul delivered

### Renderer and authored assets

- Selected-scene glTF traversal composes parent/child transforms and preserves scene instances without rendering off-scene library nodes.
- Baked node normals and animated skin normals use inverse-transpose transforms. Missing NORMAL attributes generate flat face normals rather than smoothing unrelated stone faces. Negative-determinant node transforms reverse triangle winding.
- Base-color textures use the glTF UV convention without vertical flipping, sRGB decoding, authored wrap/filter settings, RGBA decoding and per-model texture reuse.
- COLOR_0 is selected explicitly: COLOR_1 no longer overwrites it in the ruby exports. Material RGBA factors, roughness, metallic and emissive factors, double-sided rendering, alpha cutoff and alpha blending are respected. Transparent primitives sort behind opaque primitives and restore depth/blend state afterward.
- Lighting uses warm directional light, cool fill, material-dependent highlights, local emitter/ruby lights, emissive ruby/rune shading and low-cost contact shadows. Golden beams use a depth-tested core and soft additive shell with depth writes disabled; no new engine or graphics dependency was introduced.
- Ranger, KayKit floors/walls/pillars, mirrors, ruby pedestals and door assemblies use uniform authored scale rather than independent stretching of X/Y/Z. A shared WorldMetrics header defines the optical plane (y=1.5), finite mirror width and character scale.
- Mirrors use the dominant reflective triangle plane, correct the authored tilt to an upright optical surface, rotate around the physical surface centre and keep the pedestal/shaft grounded. The visible reflective width equals the BeamTracer segment width.
- Rubies use **`ruby_pedestal_floatingmaster.glb`** and **`ruby_pedestal_sealing.glb`**, with named crystal/rune meshes, original colors, actual socket geometry, hover motion, sealing motion and emissive lighting. No generic replacement ruby is used.
- Door frames remain stationary. Only `panel_*` meshes retract below the floor; a world-space clip hides buried panel geometry. Closed/open silhouettes are distinct, including from the normal camera.
- Trap base tiles remain stationary; actual spike meshes translate out of/retract into the floor and are clipped below it. Spike hazard bounds match the authored visible spike footprint plus player radius.
- Pedestal collision footprints match the chosen uniform asset scales. Pits have side walls and a recessed floor. Thin KayKit wall facades replace oversized stretched caps; coplanar baffle faces were separated after screenshot review to eliminate z-fighting.

### Puzzle redesign

- Level 1 requires its single mirror to turn the source north.
- Level 2 requires both mirrors around a real stone baffle: east -> south -> west.
- Level 3 requires all three mirrors around two real stone baffles. Reflection order is north -> east -> west (reading-order indices 0 -> 2 -> 1). The west mirror first directs light north, then south to seal the two rubies sequentially.
- Level 3 deliberately replaces the previous two-emitter layout with this stronger one-emitter, three-reflection sealing sequence. Permanent sealing, trap links, reset and campaign progression are preserved.
- Every intended mirror is mechanically necessary. No rotation counter, mirror-use completion flag or artificial solution gate was added. Optional `alternate` angle metadata is used only by designer verification/capture tools, never runtime completion.
- Full proof and search details: [PUZZLE_VERIFICATION.md](PUZZLE_VERIFICATION.md).

### UI and camera

- Both requested marketing lines are removed; the title remains REFLECTIONS OF THE TOMB.
- Batched, antialiased Roman title and sans-serif body glyph atlases replace thousands of dotted bitmap-rectangle draws. No font binary or new runtime font library is required; the regeneration tool is `tools/GenerateUiGlyphs.ps1`.
- Main/pause/controls/victory screens use a translucent side panel over the actual 3D chamber, with consistent button spacing and hierarchy.
- The 1280x720 gameplay HUD has reserved top/bottom bands, readable instructions, sealed-ruby counts and nearby mirror angles to one decimal place. Shift remains fine rotation; mouse wheel controls zoom.
- A lower elevated perspective reveals object profiles. Projection-based binary camera fitting includes wall edges and the door arch while keeping them clear of the HUD. All normal-view chamber elements remain visible at the default zoom.

## Verification

- Optimized Release build with MinGW/Ninja: passed.
- Expanded CTest suite: **38/38 passed** (final full run: 8.33 seconds), including every previous test and the original software raster self-test.
- `mandatory_reflections`: intended continuous solution sequences hit every required ruby and every intended mirror. Every mirror removal and opaque disable is tested over all integer-degree combinations of remaining mirrors in [-80,80]: 1, 161 or 25,921 configurations per case. Ruby hits are unioned across configurations, so permanent sealing cannot hide a bypass.
- In addition to the sample search, a conservative interval visibility graph proves necessity for **all real-valued angles**, including fractional angles and repeated bounces. Deleting a baffle exposes a calibrated diagonal shortcut; restoring it blocks that same actual BeamTracer ray.
- `campaign`: walks actual collision-aware routes to every mirror, solves via proximity-gated controls, revisits the final mirror for the second seal, walks through all three exits, reaches victory and restarts. Verifies hazards, remote-interaction rejection, reset semantics and completed/reset door, spike and ruby animation states.
- `model_asset_contract`: real OpenGL pixel probes verify selected-scene visibility, parent transforms, inverse-transpose normals, COLOR_0 selection, sRGB texture decoding, UV orientation, clamp sampling, RGBA factors, MASK cutoff, BLEND compositing and restored render state.
- Eleven graphics checks cover the three initial chambers, three solved/open chambers, all four menus and the GPU asset contract. OpenGL error checks pass. Idle and walk clips are sampled during smoke runs.
- Existing music, ambience and gameplay effect decoder tests pass. Audio playback engine initializes during graphics checks.

## Screenshots captured and actually inspected

All images are actual **1280x720 OpenGL readbacks**, not mockups. Regenerate with `python tools/CapturePresentation.py`. PNGs are committed under `docs/screenshots`.

| Captures | Checks performed |
| --- | --- |
| [Main menu](screenshots/menu.png), [pause](screenshots/pause.png), [controls](screenshots/controls.png), [victory](screenshots/victory.png) | Antialiased title/body text, hierarchy, button spacing, scene background, lower margins; removed copy absent. |
| [Level 1](screenshots/level1.png), [level 2](screenshots/level2.png), [level 3](screenshots/level3.png) | Uniform character/prop proportions, upright mirrors, grounded emitters, stone baffles, actual raised spikes and pits, readable HUD and camera framing. |
| [Level 1 open](screenshots/level1-open.png), [level 2 open](screenshots/level2-open.png), [level 3 open](screenshots/level3-open.png) | Correct reflected paths, sealed-ruby counts/glow, retracted spikes, unobstructed exit arches; frames stationary. |
| [Floating ruby](screenshots/ruby-floating.png), [sealed ruby](screenshots/ruby-sealed.png) | Authored pedestal, trim, rune and crystal geometry; distinct floating/socket variants, correct color layer and glow. |
| [Closed door](screenshots/door-closed.png), [opened door](screenshots/door-open.png) | Authored ornamental frame/panel, actual panel motion, clear open aperture, buried panel clipped away. |

Screenshot review found and resolved a rim-normal mirror tilt, excessive camera-fit overshoot, coplanar baffle artifacts and below-floor door visibility. The final images above were inspected again after those fixes.

## Packaging and remaining checks

`presentation_build` includes the new UI atlases and both authored ruby variants, original runtime assets, shaders and compiler runtime DLLs. `Play.cmd` remains the launch entry point. The old package remains available as the rollback copy. The rebuilt package was launched from `docs/` with compiler directories removed from PATH: the main menu and all three solved chambers loaded bundled models/UI assets/audio and returned GL error 0. No source-tree working directory was required.

No known blocking failure remains in the tested build. A new human keyboard/mouse rehearsal, physical speaker confirmation and other GPUs/operating systems remain unverified. The renderer uses practical material shading and approximate contact shadows; full physically based image lighting, shadow maps, screen-space mirror images and animation blending are outside this overhaul. Existing beam reflection physics, music, idle/walk skinning and static animation fallback remain intact.

## Stability repair checkpoint

Rollback tag: `integration-before-stability-fixes` at `9e002f8`; old runnable package copied to `build-desktop/rollback-before-stability-fixes`.

The current baseline passed 30 repeated campaign runs and a clean LLDB run. A controlled reconstruction of the earlier empty animation-array initialization defect captured 0xC0000005 at GameSession.cpp:115, during the first activated ruby tick. The user's particular popup/build remains uncorrelated; see `docs/CRASH_INVESTIGATION.md` for the exact evidence and limitation. Animation progress now belongs to Ruby/Trap objects; the parallel arrays no longer exist. GLFW/window lifetime is exception-safe and callback bindings are detached before their state expires. New entity-reallocation, multi-level reset and real GL/audio/UI exception-shutdown regression tests pass. Full suite at this checkpoint: 40 tests.

Level 3 redesign and atmospheric improvements are next; Levels 1 and 2 remain unchanged.
