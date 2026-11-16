# Integration status

Updated: 16 November 2026 (Africa/Nairobi). Branch: `integration` only. No push or merge into main.

## Rollback and stable checkpoints

- Pre-fix tag: `integration-before-stability-fixes` at `9e002f8`.
- Previous runnable package: `build-desktop/rollback-before-stability-fixes`.
- Stability milestone: `5da49e0` removes parallel animation arrays and makes normal/exceptional application teardown scope-owned. All 40 tests passed at that checkpoint.
- The overnight implementation, earlier presentation work and team contributions remain in Git history. Levels 1 and 2 authored files are unchanged by this fix pass.

## Crash investigation and repair

The baseline Release campaign completed 30 clean subprocess runs and a clean symbolized LLDB run. A spontaneous crash in the committed baseline was not reproduced; the user's popup did not have an available dump or a matching Application Error event in the targeted query.

A controlled reconstruction of the earlier empty `rubySealing` initialization fault captured **0xC0000005, access violation reading 0x00000000**, at `GameSession::update`, original `GameSession.cpp:115`. It occurs during the first activated ruby update, before teardown. The capture establishes that defect's mechanism, not the provenance of a specific user's popup. Details, source excerpt and native stack: [CRASH_INVESTIGATION.md](CRASH_INVESTIGATION.md).

Ruby now owns its sealing progress and Trap owns its extension. The parallel arrays were removed from GameSession and presentation code, so entity creation/reset/reallocation cannot leave missing animation entries. This is a structural repair rather than null-checking the failing read.

GameApplication owns GLFW and its window using scope-based lifetime guards. GL model/shader/UI resources and audio unwind while the context exists, including exceptions. Callback bindings detach before their stack-owned state expires. Window destruction precedes GLFW termination. The campaign test itself does not link renderer/audio code.

Final stability results:

- **100/100 fresh Release campaign subprocesses exited 0**; local per-run results and executable SHA-256 are in `build-stability/repeat-campaign.json`.
- Fixed campaign and session lifecycle tests both exited **0 under LLDB**, without exception stops. Transcripts: `build-stability/fixed-campaign-lldb.json` and `fixed-lifecycle-lldb.json`.
- Regression coverage includes 512 entity vector reallocations/first-seal ticks, 200 level resets/transitions with different entity counts and repeated destruction.
- Actual GL/audio/UI startup/shutdown is tested five times plus an injected C++ exception after initialization. Expected diagnostic exception exit is 10; normal exits are 0. No Windows error-dialog suppression or system settings were used.

## Level 3 repair

The Inner Sanctum now has **two emitters, two required rubies and four mirrors**. It uses two independent two-reflection circuits, not the previous single-emitter angle-switching sequence:

| Circuit | Physical path | Reachable solution |
| --- | --- | --- |
| Left | Eastbound source -> upper-left mirror 1 -> lower-left mirror 3 -> left ruby | +45, -45 degrees |
| Right | Westbound source -> upper-right mirror 2 -> lower-right mirror 4 -> right ruby | -45, +45 degrees |

Baffles at columns 3 and 11 block diagonal shortcuts. A central wall at column 7 separates the optical circuits; the lower passage connects the player routes. Pits remain meaningful hazards. Each ruby retracts its own spike group; the exit requires both seals. Permanent activation, reset, restart and progression remain unchanged. The BeamTracer implementation and mirror optical-plane alignment are preserved.

Verification:

- Real continuous finite-mirror traces confirm each emitter hits its own ruby after exactly the intended two reflections, with no solid-wall crossings and correct physical reflection normals.
- All 16 combinations of +/-0.2 degree error around the four solution angles still hit both rubies, providing tolerance beyond a precision-mode input frame.
- Every mirror is physically necessary. A conservative visibility proof covers all real-valued angles and repeated bounces. Removal and opaque-disabling searches union hits over all states, accounting for permanent sealing. Independent optical components are factored conservatively; 26,083 actual configurations are traced per Level 3 mirror removal/disable. See [PUZZLE_VERIFICATION.md](PUZZLE_VERIFICATION.md).
- Initial poses do not seal either ruby. Reset restores all four starting angles, both activation states and animation progress.
- Both earlier puzzles continue passing their existing physics and progression checks; their authored files were not modified.

## Actual control-path verification

`--control-test` posts native Windows key messages to its own hidden GLFW window. The application consumes those messages through normal `glfwPollEvents` / `glfwGetKey` and its unchanged gameplay controls. The diagnostic never writes player positions, mirror angles, ruby activation or completion state.

The replay starts with Enter, tests Escape pause/resume and paused WASD, walks to all required mirrors, aligns them using Q/E with Shift precision, resets Level 3 using R after sealing a ruby, solves again and walks through every exit to victory. Actual settled mirror angles are approximately +/-44.92 degrees, within the allowed +/-80 range. Native Shift is verified during precision frames. Final GL error is 0.

## Atmospheric and gameplay visuals

- A full-screen OpenGL 3.3 navy/indigo gradient with restrained haze replaces the flat black background. Actual framebuffer probes check hue and non-flat gradient values.
- A stone foundation and tiled terrace surround the temple. The centre remains hollow below pit floors; scenic geometry is renderer-only and is absent from gameplay collision/beam data.
- Existing KayKit lit torches add warm accents and subtle local flicker against cool shadows. Light distance haze adds restrained atmospheric depth.
- Existing authored floating/sealing ruby assets retain their geometry, glow and local illumination. Golden beams have a slightly stronger core and still terminate at real BeamTracer hit positions.
- The interactable mirror gains a small gold world-space pedestal outline as well as the existing HUD angle information. Door panel/frame separation and clipped underground animation are preserved.
- Character idle/walk skinning, static fallback, music/ambience/effects, menus, readable 1280x720 HUD and mouse-wheel zoom remain functional.

## Final verification

- Existing CMake/Ninja **Release build: passed**.
- Complete CTest suite: **43/43 passed**, including all existing tests and new lifecycle, exact Level 3 path/tolerance, native input and atmospheric framebuffer checks. Latest full run: **16.05 seconds**.
- Fourteen graphics-labelled checks pass, including normal/solved chambers, menus, material GPU probes, exceptional teardown, native controls and background readback.
- Existing audio decoder tests pass; playback engine initializes during the rendered tests.
- Rebuilt the standalone presentation package and launched its menu and all three solved chambers from the documentation directory with only Windows directories on PATH: four clean exits, each reporting `GL=0`.

## Actual screenshots inspected

All 19 PNGs below are 1280x720 OpenGL readbacks. `python tools/CapturePresentation.py` captures normal/solved scenes and additionally runs the native-input campaign. Images are committed under `docs/screenshots`.

| Images | Visual checks |
| --- | --- |
| [Menu](screenshots/menu.png), [pause](screenshots/pause.png), [controls](screenshots/controls.png), [victory](screenshots/victory.png) | Navy setting, typography, hierarchy, margins and real scene background. |
| [Level 1](screenshots/level1.png), [level 2](screenshots/level2.png), [level 3](screenshots/level3.png) | Original character/props, terrace, warm torches, pits, raised traps, four final-chamber mirrors and readable HUD. |
| [Level 1 solved](screenshots/level1-open.png), [level 2 solved](screenshots/level2-open.png), [level 3 solved](screenshots/level3-open.png) | Correct beam paths, lit ruby models, retracted traps and clear open door arch. |
| [Floating ruby](screenshots/ruby-floating.png), [sealed ruby](screenshots/ruby-sealed.png), [closed door](screenshots/door-closed.png), [open door](screenshots/door-open.png) | Authored geometry/materials, glow, stationary frame and functional panel motion. |
| [Native initial](screenshots/native-level3-initial.png), [first ruby](screenshots/native-level3-ruby1.png), [both rubies](screenshots/native-level3-ruby2.png), [fully opened](screenshots/native-level3-open.png), [native victory](screenshots/native-victory.png) | Actual WASD/QE/Shift play, intermediate 1/2 seal state, both physical circuits, selectable-mirror outline and successful exit. |

## Packaging and limits

`presentation_build` includes background shaders, lit-torch GLTF/buffer assets, existing UI glyph atlases and both ruby variants, original audio, runtime DLLs, instructions, screenshot gallery and verification documents. Launch with `Play.cmd`.

The original popup's exact binary/build remains uncorrelated; no spontaneous access violation was seen in the current verified build. Native key replay verifies the actual application's control path but is not a new human playthrough. Physical speaker audibility, other GPUs/operating systems and other keyboard layouts remain unverified. Scene lighting uses practical material shading/contact shadows rather than full physically based reflections or shadow maps. No known blocking failure remains in the tested campaign.
