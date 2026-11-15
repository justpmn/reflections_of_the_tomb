# Reflections of the Tomb

Run `Play.cmd` in the repository, or run `reflections_of_the_tomb.exe` in the presentation folder. Select **Start Game** or press Enter. Requires an OpenGL 3.3 graphics driver.

| Control | Action |
| --- | --- |
| WASD | Move Ranger through the temple |
| Q / E | Rotate the nearest mirror, within 2.2 world units |
| Hold left Shift | Fine mirror rotation |
| R | Reset current chamber, including rubies and mirror angles |
| Escape | Pause/resume |
| M | Mute/unmute all sound |
| Mouse | Select menu buttons |

Redirect the gold beams into the floating red rubies. A ruby seals permanently once hit; you can move a mirror afterward. The exit rises when all linked rubies are sealed. Walk into the doorway to advance. Spikes retract when their linked ruby is sealed. Active spikes and pits return Ranger to the chamber spawn without erasing puzzle progress.

Pause offers Resume, Reset Chamber, Controls, Restart Campaign and Main Menu. Completing the third exit shows victory and Play Again.

## Presentation walkthrough (spoilers)

Angles describe the reflective surface seen from above: `45` is a backslash; `-45` is a slash. Reach each mirror before rotating it.

1. **The Entrance Hall:** the east mirror turns the eastbound beam north toward the ruby. Rotate from +45 to -45 using Q.
2. **Hall of Spikes:** set the northern mirror and then the southern mirror to +45 using E. The beam travels east, south, then east to the ruby. Spikes retract and the south exit opens.
3. **The Inner Sanctum:** set the north mirror to +45, the east mirror to +45, and the southwest mirror to -45. The two emitters seal both rubies. Avoid the pit strip and active spikes while reaching the mirrors; the west side provides a safe route around the pits.

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

Optional desktop checks: configure `-DENABLE_RENDER_SMOKE_TESTS=ON`. These hidden-window tests require a working OpenGL desktop. The game also accepts `--smoke --level 1` (or 2/3), `--screen start|pause|controls|victory`, and `--capture image.ppm` for validation captures.

Ranger uses the supplied KayKit `Idle_A` and `Walking_A` clips retargeted by matching joint names with CPU skinning. Incompatible/missing clips use the static character. Audio uses existing assets through miniaudio and becomes silent if an audio device is unavailable. The supplied MP4 reflection chime is not used; existing WAV/OGG effects cover interactions instead.
