# Reflections of the Tomb
## Overview

**Reflections of the Tomb** is a puzzle game set in an ancient temple where players manipulate mirrors to redirect beams of light, solve puzzles, deactivate traps, and unlock pathways.

The main objective is to guide light from fixed sources through a series of rotatable mirrors toward target rubies. Players must carefully adjust mirror orientations while avoiding dangerous beams and environmental traps.

The project explores computer graphics concepts including geometric transformations, ray casting, reflection, collision detection, and real-time 3D rendering.

## Technologies

- **C++17** — Core game logic and graphics algorithms
- **OpenGL 3.3** — Graphics rendering
- **GLFW** — Window management and input handling
- **GLAD** — OpenGL function loading
- **GLM** — Vector and matrix mathematics
- **GLSL** — Shader programming
- **CMake & Ninja** — Build system
- **Blender** — 3D asset creation and modification
- **Git & GitHub** — Version control and collaboration

## Light Reflection

The game's central mechanic uses ray casting and the law of reflection to calculate how light interacts with mirrors.

The reflected direction is calculated using:

**R = D − 2(D · N)N**

Where:
- **D** — Incoming ray direction
- **N** — Mirror surface normal
- **R** — Reflected ray direction

Ray-plane intersections determine where a beam hits a mirror. The resulting direction can then be used to trace subsequent reflections.

## Installation and Setup

### Prerequisites

- C++17-compatible compiler (MSYS2 UCRT64 GCC)
- CMake 3.20+
- Ninja
- Python with Jinja2
- OpenGL 3.3-compatible graphics driver

### Build

From the project root, configure CMake:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DPython_EXECUTABLE=C:/msys64/ucrt64/bin/python.exe
```

Compile the project:

```powershell
cmake --build build
```

### Run

```powershell
.\build\reflections_of_the_tomb.exe
```

Press **ESC** to close the application.

## Test Scripts

The project includes tests for ray intersection and reflection calculations.

| Test | Description |
|---|---|
| `reflection_math` | Verifies reflected ray directions |
| `ray_plane` | Tests ray-plane intersections |
| `single_mirror` | Tests single-mirror reflection and beam segments |

Run all tests:

```powershell
ctest --test-dir build --output-on-failure
```

## Project Structure

```text
reflectionsofthetomb/
├── src/
│   ├── main.cpp
│   ├── core/          # Application management
│   ├── physics/       # Ray intersections and collisions
│   ├── beam/          # Beam tracing and reflections
│   ├── mirror/        # Mirror transformations
│   ├── player/        # Player movement and interaction
│   ├── world/         # Temple environment and levels
│   ├── gameplay/      # Rubies, traps and puzzle logic
│   └── rendering/     # Rendering and shaders
├── assets/
│   ├── models/
│   ├── textures/
│   └── audio/
├── shaders/
├── tests/
├── experiments/
├── docs/
├── CMakeLists.txt
└── README.md
```

## Assets and Credits

Third-party assets are credited to their respective creators and used according to their licensing terms.

| Asset | Creator | Source | License |
|---|---|---|---|
| To be added | — | — | — |

Asset credits will be updated as resources are incorporated into the project.

---

**Reflections of the Tomb** — Built with C++ and OpenGL.
