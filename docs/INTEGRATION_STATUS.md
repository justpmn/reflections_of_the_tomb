# Integration status

Updated: 16 November 2026 (Africa/Nairobi). Branch: `integration`. No push or merge into main.

## Final gameplay polish

Baseline: `b591380`. Rollback tag `integration-before-final-polish` was created before editing. Existing five-level data, mirror geometry, BeamTracer, audio and assets are retained.

- Character state: Alive -> Dying (0.95s) -> Respawning (spawn reposition) -> Invulnerable (2s) -> Alive. Hazard contact increments the counter exactly once, stops movement and mirror interaction, and plays the existing defeat sound. Respawn plays its existing sound and preserves seals, door state, mirror angles and trap phases. Simulation freezes during the short death pose.
- Inspected supplied GLB animation metadata: General contains Death_A/Death_B and terminal poses; MovementBasic contains jumping/falling-related clips. Death_A's target names/interpolation match Ranger, so the existing CPU skeletal sampler now plays that compatible clip once, clamped at its final sample. Missing/incompatible death clips retain a procedural collapse fallback; pit deaths also descend visually.
- Ranger opacity alternates between 1.0 and 0.32 every 0.12 simulation seconds during protection. Shader opacity is per instance, multiplies authored alpha, and defaults/resets to 1. Existing materials, textures and normals remain intact. GPU probes verify transparent draw state restoration and the next opaque instance.
- Session phases: Countdown -> Playing -> FadeOut -> Title -> FadeIn -> Countdown. Each number lasts one second, GO lasts 0.45s. Timed hazards, puzzle activation, movement and exits freeze until play begins. Input must be released after being held through GO before movement/rotation can resume.
- On exit, the old chamber fades for 0.45s; only at black is the next chamber loaded. Its metadata supplies the centered number and title for 2s, followed by a 0.45s reveal and its countdown. The fifth exit fades to victory without attempting Level 6. Countdown start audio is emitted once per entry by the same presentation path that displays it.
- Pause freezes countdown, transition, death and protection clocks. Reset retains the existing immediate-reset behavior (no replayed countdown), clears character state, restores authored trap phases and mirror poses, and resets objectives. Campaign restart clears deaths and starts Level 1's countdown.
- Timed tiles now have complete teal safe outlines, pulsing amber warning outlines with moving mechanical studs, and orange-red base outlines around visibly raised darker metallic-grey spikes. Tints apply only to the spike mesh instance; stone plates and other assets preserve their original materials.

## Five-level campaign delivered

Sequence: **The Entrance Hall -> Hall of Spikes -> The Inner Sanctum -> The Gauntlet -> The Final Trial**. Level 3 advances to Level 4. Only the fifth completed exit reaches victory. HUD counters use LevelManager's actual count; victory reports five chambers and retains the respawn counter. The existing generic LevelManager supports the expanded path list without a replacement implementation.

Rollback checkpoint: tag `integration-before-five-level-expansion` at `b0c3217`, created before changing campaign systems. Earlier stability commits/tags remain available. The authored Level 1-3 files and BeamTracer implementation are unchanged by this expansion. Existing renderer, Ranger idle/walk animation and fallback, ruby assets, music, UI and collision remain in use.

## Timed spikes

- `Trap` owns an independent cycle, offset, durations and extension value. Explicit states: Retracted, Warning, Extended. Legacy ruby-controlled traps retain their original API and behavior.
- Authored format: `trap <i> timed <safe> <warning> <extended> <offset> [disabled_by <ruby>]`. Invalid/non-finite timings are rejected. An explicit ruby link disables a timed trap; otherwise it continues cycling after puzzle completion.
- Delta-time simulation uses a double-precision phase and modulo cycle evaluation. Smoothstep extension/retraction takes 0.4 seconds at each end of Extended. Safe and Warning have fully retracted geometry. Timed danger requires visible extension above 18 percent (about 0.153 world units).
- Existing spike GLB floor and moving spike meshes are preserved. Teal/amber/red rails indicate safe/warning/extension; these render after opaque floor plates. Initial screenshot review found the plates obscuring early rails; draw order was corrected and refreshed captures inspected.
- Existing rumble provides warning; spike extending/retracting WAVs accompany movement transitions. Group transitions are coalesced within a frame to avoid duplicate overlapping copies for adjacent plates.
- Hazard collision uses circle versus the actual 1.1-unit-wide spike footprint, including corner-distance checks. Contact respawns at the chamber spawn and preserves seals/cycles. Spawn positions are safe; regression checks reject repeated respawns while stationary at spawn.
- Reset rebuilds authored phases, mirror angles, ruby states and closed doors. Pause skips gameplay simulation and freezes every trap phase; resume uses the normal frame delta rather than paused wall time.

## Authored chambers and fairness

| Chamber | Layout and objectives | Spike timing |
| --- | --- | --- |
| The Gauntlet | 15x11 cells; two sources, four necessary mirrors, two rubies; two crossing sections, lower waiting/passage area, two pits and ruby-controlled exit barrier | Left: 5.5s safe / 1.2s warning / 2.4s extension, offset 0. Right: 5.0 / 1.2 / 2.8, offset 1.7s. |
| The Final Trial | Larger 22x12 chamber; three sources, six necessary mirrors, three rubies; three independently phased sections, two dividers, three pits and three ruby-controlled exit groups | Adds rightmost section: 4.5s safe / 1.2s warning / 3.2s extension, offset 3.4s. |

Each optical bay uses source -> upper mirror (+45) -> lower mirror (-45) -> ruby. Baffles eliminate diagonal bypasses. Dividers isolate optical circuits while lower passages connect movement routes. Every authored mirror participates in a real finite beam path; removing any one makes at least one required ruby unreachable even when seals persist between configurations. The proof covers the continuous supported angle range; integer-degree real-tracer searches independently verify removal and opaque disabling. See [puzzle verification](PUZZLE_VERIFICATION.md).

A straight crossing including the Ranger footprint requires about 0.64 seconds at 4 world units/second. Safe windows are at least 4.5 seconds, with a separate 1.2-second warning. Players can wait on normal floor before crossing and can return by the same route. Tests cross plate rows directly rather than trying to wait or walk sideways on them. No jumps or unavoidable damage are required.

## Verification

- Release build passed. Complete suite: **63/63 tests passed**, including **28 graphics-labelled checks**. Latest complete run: **38.81 seconds**.
- Existing tests remain enabled. New coverage includes state transitions, smooth motion, 30/60/144 FPS equivalence, reaction/crossing margins, safe overlap, raised contact, circle-corner rejection, seal preservation, independent offsets, disable/reset, and authored phase restoration.
- Level-manager coverage confirms the third chamber is no longer last and the fifth is last. All five authored puzzles pass loader/grid design checks and real continuous BeamTracer necessity checks.
- Headless campaign walks to every mirror using real movement/collision and proximity rotation, then through all five exits, with zero hazard deaths. Additional full runs shift timed phases by 7.25 and 19.1 seconds and also finish safely.
- Native Windows GLFW replay uses actual Enter, Escape, WASD, Q/E, Shift and R messages. It aligns all 17 mirrors, seals all objectives, checks frozen phases during a real Gauntlet pause, resets after a Level 3 seal, and reaches fifth-level victory with zero respawns and `GL=0`. It does not directly assign player positions, angles or ruby activation.
- Repeated Release campaign subprocess verification: 100/100 clean exits. Exception/lifecycle and original audio decoding tests pass. Existing playback engine initializes in GL checks.

## Visual verification

`python tools/CapturePresentation.py --output build-desktop/polish-screenshots` produces 41 PNG OpenGL readbacks at 1280x720. Current captures and PPM intermediates remain in ignored build directories, excluded from staging as requested. The package can include these local verification PNGs without committing them.

Inspected current captures: `countdown.png` (Level 1 numeral 3), `transition-level2.png` (gold title on fully black screen), `ranger-death.png` (skeletal collapsed pose), `ranger-protected.png` (partial transparency), all three `level4-spikes-*.png` and all three `level5-spikes-*.png`, and `native-victory.png` (five chambers, zero respawns). Checked pose, opacity, stone materials, complete colored outlines, warning studs, raised spike contrast, title alignment and HUD bounds. Additional normal/solved campaign and asset views are available in the same directory. The earlier committed gallery in `docs/screenshots` remains as the previous milestone record.

New headless `gameplay_flow` coverage includes countdown number order, frozen movement/mirrors/hazards, held input gating, paused clocks, death/respawn order and spawn, protection start/expiry, hazard immunity, one counter increment per death, preserved puzzle state, immediate reset, restart, metadata-driven transition ordering and fifth-level victory. Existing campaign movement tests now wait through real entry/fade/title phases rather than bypassing them. Native GLFW replay additionally holds W/E during countdown and for twelve frames after GO, verifies no unintended action, releases, and then completes the entire campaign.

## Presentation and limits

Launch `build-desktop/presentation/Play.cmd`. The package contains all five level files, original assets/audio/animations, compiler runtime DLLs, shaders, updated instructions, screenshots and verification documents. Standalone polish verification passed from the documentation directory with only Windows directories on PATH: countdown, Level 2 title, death and protected-Ranger views exited cleanly with `GL=0`; the packaged executable also completed the entire native five-level keyboard replay, including countdown held-key checks and timed pause verification, with zero respawns and `GL=0`.

No known blocking failure remains in the tested campaign. Native replay is automated control-path verification, not a new human playthrough. Physical speaker audibility, subjective difficulty, alternate keyboard layouts and other GPUs/operating systems remain unverified. The old reported crash popup still has no correlated original dump; the earlier controlled null-read reconstruction and repair remain documented in [crash investigation](CRASH_INVESTIGATION.md).
