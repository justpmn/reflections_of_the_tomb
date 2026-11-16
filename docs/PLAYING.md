# Reflections of the Tomb

Run `Play.cmd` in the repository, or run `reflections_of_the_tomb.exe` in the presentation folder. Select **Start Game** or press Enter. Requires an OpenGL 3.3 graphics driver.

| Control | Action |
| --- | --- |
| WASD | Move Ranger through the temple |
| Q / E | Rotate the nearest mirror, within 2.2 world units |
| Hold left Shift | Fine mirror rotation |
| Mouse wheel | Zoom the 3D camera |
| R | Reset current chamber, including rubies, mirror angles and timed trap phases |
| Escape | Pause/resume |
| M | Mute/unmute all sound |
| Mouse | Select menu buttons |

Redirect the gold beams into the floating red rubies. A ruby seals permanently once hit; you can move a mirror afterward. The exit rises when all linked rubies are sealed. Walk into the doorway to advance. Ruby-controlled spikes retract when their linked ruby is sealed. Timed sections cycle independently: teal floor rails mean safe, amber warns before extension, and red accompanies raised spikes. Wait on ordinary floor, then cross the row promptly; do not wait on a spike plate. Timed hazards keep cycling even after the door unlocks unless explicitly linked to a ruby. Active spikes and pits return Ranger to the chamber spawn without erasing puzzle progress.

Pause offers Resume, Reset Chamber, Controls, Restart Campaign and Main Menu. Completing the fifth exit shows victory and Play Again.

## Presentation walkthrough (spoilers)

Angles describe the reflective surface seen from above: `45` is a backslash; `-45` is a slash. Reach each mirror before rotating it.

1. **The Entrance Hall:** the east mirror turns the eastbound beam north toward the ruby. Rotate from +45 to -45 using Q.
2. **Hall of Spikes:** set the north mirror to +45 using E and the south mirror to -45 using Q. The beam travels east, south, then west around the stone baffle to the ruby. Spikes retract and the south exit opens.
3. **The Inner Sanctum:** solve the two optical circuits independently. Set the upper-left mirror (1) to +45 and lower-left mirror (3) to -45 to seal the left ruby. Use the lower central passage to reach the other side; set upper-right mirror (2) to -45 and lower-right mirror (4) to +45 to seal the right ruby. Each ruby retracts its own spike group; both rubies are required for the final exit. Avoid the two pits. There is no angle-switching step or fourth-quadrant rotation.

4. **The Gauntlet:** two eastbound sources, four mirrors and two rubies. From the lower waiting area, observe the teal/amber/red cycle before crossing the left spike row. Set upper-left mirror 1 to +45 and lower-left mirror 3 to -45. Return below the divider and enter the right bay during its independently phased safe interval. Set upper-right mirror 2 to +45 and lower-right mirror 4 to -45. Both lower ruby-controlled spike groups retract and the exit opens. Timed rows continue cycling. Avoid the pits and use the clear central passage below the divider.
5. **The Final Trial:** three optical bays, six mirrors and three rubies. Solve left, centre, then right: upper mirrors 1/2/3 to +45 and their corresponding lower mirrors 4/5/6 to -45. Each bay has its own spike timing; wait on plain floor before committing to a crossing. The right bay has the shortest safe interval. Use the lower transverse passage around both dividers, avoid the three pits, and return through the final doorway only after all three ruby-controlled exit groups retract. Victory reports five chambers and your total respawns.

Timed rows provide 5.5, 5.0 or 4.5 seconds of fully safe movement, followed by a 1.2-second warning and 2.4/2.8/3.2 seconds of spike motion/danger. A direct cell crossing including the Ranger footprint requires about 0.64 seconds at normal speed. Pause freezes these cycles; chamber reset restores their authored phase offsets. Respawn preserves seals and current cycles; restarting the campaign resets the respawn counter.

## Build and tests

With CMake, a C++17 compiler, Ninja/your chosen generator and Python available for GLAD generation:

```powershell
cmake -S . -B build-desktop
cmake --build build-desktop --parallel 6
ctest --test-dir build-desktop --output-on-failure
cmake --build build-desktop --target presentation_build
```

The first configuration downloads GLFW, GLM and GLAD. For the already populated local dependency cache, configure with `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`.

`presentation_build` creates `build-desktop/presentation` with the executable, compiler runtime DLLs, shaders and required assets. The folder can be copied as a unit; no source tree is required.

Optional desktop checks: configure `-DENABLE_RENDER_SMOKE_TESTS=ON`. These hidden-window tests require a working OpenGL desktop. The game also accepts `--smoke --level 1` (or 2/3/4/5), `--screen start|pause|controls|victory`, `--solved`, `--view ruby|door`, and `--capture image.ppm` for validation captures. `--solved` and close-up views apply only in smoke mode. Run `python tools/CapturePresentation.py` to generate the reviewed PNG screenshots in `docs/screenshots`.

Ranger uses the supplied KayKit `Idle_A` and `Walking_A` clips retargeted by matching joint names with CPU skinning. Incompatible/missing clips use the static character. Audio uses existing assets through miniaudio and becomes silent if an audio device is unavailable. The supplied MP4 reflection chime is not used; existing WAV/OGG effects cover interactions instead.

The HUD shows the nearby mirror angle to one decimal place and the number of sealed rubies. Use Shift for precise alignment. Every required reflection is enforced by geometry; no rotation count or hidden completion gate is used. See [puzzle verification](PUZZLE_VERIFICATION.md).

## Native control verification (Windows)

`reflections_of_the_tomb.exe --control-test` replays targeted native key messages into its own hidden GLFW window. It starts the campaign with Enter, tests pause/resume, walks using WASD, aligns mirrors using Q/E and Shift, resets Level 3 with R after sealing a ruby, solves again and walks through all five levels to victory without a hazard death. It also pauses in The Gauntlet and checks that every trap phase freezes. It never changes gameplay positions, angles or ruby states directly. Diagnostic snapshots go to `build-desktop/control-captures`.

`python tools/RepeatCampaign.py --runs 100` checks fresh campaign-test process exits. `tools/DebugCampaign.py` uses the installed Android Studio LLDB adapter for symbolized checks; forensic details are in `docs/CRASH_INVESTIGATION.md`.
