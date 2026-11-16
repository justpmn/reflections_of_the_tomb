# Integration status

Updated: 16 November 2026 (Africa/Nairobi). Branch: `integration`. No push or merge into main.

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

- Release build passed. Complete suite: **58/58 tests passed**, including **24 graphics-labelled checks**. Final complete run: **35.29 seconds**.
- Existing tests remain enabled. New coverage includes state transitions, smooth motion, 30/60/144 FPS equivalence, reaction/crossing margins, safe overlap, raised contact, circle-corner rejection, seal preservation, independent offsets, disable/reset, and authored phase restoration.
- Level-manager coverage confirms the third chamber is no longer last and the fifth is last. All five authored puzzles pass loader/grid design checks and real continuous BeamTracer necessity checks.
- Headless campaign walks to every mirror using real movement/collision and proximity rotation, then through all five exits, with zero hazard deaths. Additional full runs shift timed phases by 7.25 and 19.1 seconds and also finish safely.
- Native Windows GLFW replay uses actual Enter, Escape, WASD, Q/E, Shift and R messages. It aligns all 17 mirrors, seals all objectives, checks frozen phases during a real Gauntlet pause, resets after a Level 3 seal, and reaches fifth-level victory with zero respawns and `GL=0`. It does not directly assign player positions, angles or ruby activation.
- Repeated Release campaign subprocess verification: 100/100 clean exits. Exception/lifecycle and original audio decoding tests pass. Existing playback engine initializes in GL checks.

## Visual verification

`tools/CapturePresentation.py` generates **37 PNG OpenGL readbacks at 1280x720**, including all five normal/solved chambers, menu/controls/pause/victory, ruby/door close-ups, six new trap-state previews, and actual native-play intermediate states.

Inspected all five normal chamber views, the new solved chamber views, safe/warning/extended previews in both new chambers, native Level 4 initial/first seal/open states, native Level 5 initial/one/two/three seals/open states, updated controls and native five-chamber victory. Checked framing, authored ruby/mirror assets, visible spike extension and state rails, active timers after seals, ruby-controlled retraction, open arches, HUD margins and zero-respawn victory.

- [The Gauntlet](screenshots/level4.png), [solved](screenshots/level4-open.png), [safe](screenshots/level4-spikes-safe.png), [warning](screenshots/level4-spikes-warning.png), [extended](screenshots/level4-spikes-extended.png).
- [The Final Trial](screenshots/level5.png), [solved](screenshots/level5-open.png), [safe](screenshots/level5-spikes-safe.png), [warning](screenshots/level5-spikes-warning.png), [extended](screenshots/level5-spikes-extended.png).
- [Native Gauntlet open](screenshots/native-level4-open.png), [Final Trial first seal](screenshots/native-level5-ruby1.png), [second seal](screenshots/native-level5-ruby2.png), [third seal](screenshots/native-level5-ruby3.png), [open](screenshots/native-level5-open.png), [five-chamber victory](screenshots/native-victory.png).

## Presentation and limits

Launch `build-desktop/presentation/Play.cmd`. The package contains all five level files, original assets/audio/animations, compiler runtime DLLs, shaders, updated instructions, screenshots and verification documents. Standalone verification passed from the documentation directory with only Windows directories on PATH: menu and solved Levels 4/5 exited cleanly with `GL=0`; the packaged executable also completed the entire native five-level keyboard replay, including timed pause verification, with zero respawns and `GL=0`.

No known blocking failure remains in the tested campaign. Native replay is automated control-path verification, not a new human playthrough. Physical speaker audibility, subjective difficulty, alternate keyboard layouts and other GPUs/operating systems remain unverified. The old reported crash popup still has no correlated original dump; the earlier controlled null-read reconstruction and repair remain documented in [crash investigation](CRASH_INVESTIGATION.md).
