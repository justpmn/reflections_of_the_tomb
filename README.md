# Reflections of the Tomb

## Overview

**Reflections of the Tomb** is a 3D puzzle game set in an ancient temple where players manipulate mirrors to redirect beams of light, activate rubies, disable traps, and unlock pathways.

The objective is to solve increasingly challenging puzzles by controlling the path of light through multiple mirrors while avoiding environmental hazards.

The project applies computer graphics concepts including geometric transformations, line and polygon drawing, ray casting, reflection mathematics, collision detection, and real-time rendering.

## Team

| Member | Responsibility |
|---|---|
| Peter Nyamu | Light reflection, ray casting, and beam collisions |
| Kimberly Wangari | Player movement and interactions |
| Joe Senetwo | Mirror transformations and rotation |
| Zahra Isaiho | Level design and temple environment |
| Jesse Mbugua | Rubies, traps, doors, and puzzle mechanics |
| Joe Ndeto | Rendering, lighting, shaders, and assets |

## Tech Stack

- **C++17** — Game logic and graphics algorithms
- **OpenGL 3.3** — 3D rendering
- **GLFW & GLAD** — Window management and OpenGL function loading
- **GLM** — Vector and matrix mathematics
- **GLSL** — Shader programming
- **CMake & Ninja** — Build system
- **Blender** — 3D asset creation and modification
- **Git & GitHub** — Version control and collaboration

## Graphics Algorithms

The following graphics techniques have been implemented:

- **Bresenham's Line Algorithm:** Integer-based line drawing.
- **Polygon Representation:** Constructing objects from vertices and edges.
- **Scanline Polygon Filling:** Filling polygon interiors using the even-odd rule.
- **Geometric Translation:** Moving doors, traps, and other graphical objects.
- **Boundary Clipping:** Restricting pixel operations to valid image coordinates.
- **Ray-Plane Intersection:** Calculating where a ray intersects a surface.
- **Light Reflection:** Calculating reflected directions using surface normals.

The reflection equation is:

\[
R = D - 2(D \cdot N)N
\]

Where `D` is the incoming ray direction, `N` is the surface normal, and `R` is the reflected direction.

The repository also includes Pure C++ graphics demonstrations of simplified temple objects, including a ruby, door, and spike traps.

## Build and Run

### Requirements

- C++17-compatible compiler (MSYS2 UCRT64 GCC)
- CMake 3.20+
- Ninja
- Python with Jinja2
- OpenGL 3.3-compatible graphics driver

### Build the Game

From the project root, configure CMake:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DPython_EXECUTABLE=C:/msys64/ucrt64/bin/python.exe
```

Build:

```powershell
cmake --build build
```

Run:

```powershell
.\build\reflections_of_the_tomb.exe
```

Press **ESC** to close the application.

### Run the Pure C++ Graphics Demo

```powershell
cd experiments/software_raster_demo
g++ -std=c++17 -O2 main.cpp -o tomb_graphics.exe
.\tomb_graphics.exe
```

The program generates `tomb_graphics.ppm`, containing graphical representations of the game objects.

## Test Scripts

The project includes automated tests for the reflection system.

| Test | Description |
|---|---|
| `reflection_math` | Tests reflected ray calculations |
| `ray_plane` | Tests ray-plane intersections |
| `single_mirror` | Tests single-mirror reflection and beam segments |

Run all tests from the project root:

```powershell
ctest --test-dir build --output-on-failure
```

Run the standalone Pure C++ graphics tests from its directory:

```powershell
.\tomb_graphics.exe --self-test
```

## Project Structure

```text
reflectionsofthetomb/
├── src/
│   ├── main.cpp
│   ├── core/
│   ├── beam/
│   ├── physics/
│   ├── mirror/
│   ├── player/
│   ├── world/
│   ├── gameplay/
│   └── rendering/
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

## Progress and Limitations

The project currently has a working OpenGL application, a modular codebase, Pure C++ graphics demonstrations, and tested reflection mathematics. Ray-plane intersections and single-mirror reflections have been implemented and verified using automated tests.

The main game is still under development. Multiple-mirror propagation, finite mirror collisions, visual beam rendering, player interactions, and complete 3D environment integration are not yet finished.

**Reflections of the Tomb** — C++ / OpenGL
