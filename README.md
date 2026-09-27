# AetherPhysics

AetherPhysics is a real-time 3D physics visualizer written in C++20. It runs on
Vulkan through the Light Vulkan Graphics library.
It has interactive models of aether density and of wave propagation in
different media.

## Models

### Aether Density Visualizer (default, `F1`)

A 3D lattice of aether particles whose density varies with position. Rays
through the lattice show how the density field bends paths.

The density can be shown two ways (toggle with `V`):

- **Fog** (default): volumetric fog whose thickness follows the density above
  the background level, so it is densest at the centre and fades out with
  distance. It is coloured by orbital speed with the same colours as the
  particles.
- **Particles**: the orbiting particle lattice, coloured by how fast each
  particle goes round the vertical axis: blue for the slowest (outer corners),
  through cyan, to orange for the fastest (next to the axis). The lattice
  size depends on the detected GPU (see [GPU scaling](#gpu-scaling)).

### Wave Physics (`F2`)

A plate on the left drives a 2D wave through a choice of media, to show which
media can carry a transverse (shear) wave like light.

| Medium       | Transverse (shear)                  | Longitudinal (pressure)       |
|--------------|-------------------------------------|-------------------------------|
| `SOLID`      | Travels as an elastic wave          | Travels as an elastic wave    |
| `VISCOUS`    | Decays with distance; viscosity sets how far it reaches | Travels, damped by viscosity |
| `LIQUID`     | Not supported: dies near the plate  | Travels (sound)               |
| `GAS`        | Not supported: mostly thermal motion | Travels (sound)              |
| `SUPERSOLID` | Travels with very little loss       | Travels with very little loss |
| `SUPERFLUID` | Not supported at all: slips past the plate | Travels with no loss   |

The solids are spring lattices, so their waves emerge from the springs. The
fluids are tracer particles that follow a prescribed wave field. A line across
the middle shows the average displacement (solids) or velocity (fluids) at each
distance from the plate; for shear waves in fluids, a faint curve near the
bottom shows how quickly they die away.

The superfluid is the absolute-zero limit: zero viscosity, no thermal motion.
Like the supersolid it is frictionless, but it cannot carry shear at all, so it
shows the other half of the aether puzzle: a medium planets could move through
freely, but one that could not carry light.

You can switch the wave between **transverse** and **longitudinal**. Set the
medium, wave type, viscosity and driving frequency from the on-screen panel or
the keyboard (see below). The viscosity setting only affects the Viscous medium.

Each model has an **About this view** panel explaining the physics on screen.
Its last paragraph follows what you're looking at (fog or particles; the
current medium and wave type). Drag its title bar to move it, or click the
arrow to collapse it.

## Controls

### Global

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `F1`           | Switch to the Aether Density Visualizer |
| `F2`           | Switch to Wave Physics                  |
| `F3`           | Print the model menu to the console     |
| `Esc`          | Quit                                    |

### Aether Density Visualizer

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `V`            | Toggle particles / volumetric fog       |
| `+` / `=`      | Lengthen density rays by 10 segments    |
| `-`            | Shorten density rays by 10 segments     |

### Wave Physics

| Key            | Action                                            |
|----------------|---------------------------------------------------|
| `F5` – `F10`   | Medium: solid, viscous, liquid, gas, supersolid, superfluid |
| `T`            | Toggle transverse / longitudinal wave             |
| `+` / `=`      | Increase viscosity by 0.1 (range 0–1)             |
| `-`            | Decrease viscosity by 0.1                         |
| `.` / `>`      | Increase frequency by 0.01 (range 0.05–0.3)       |
| `,` / `<`      | Decrease frequency by 0.01                        |
| `M`            | Print the wave controls menu                      |
| `Alt` + `W`    | Print the wave controls menu and current status   |

Keypad `1`–`6` also pick the medium, and keypad `+`/`-` work too. The top-row
digits `1`–`4` belong to Light Vulkan Graphics, which uses them to switch its
render mode (normal, wireframe, unlit, spheres).

## GPU scaling

When the app starts, it estimates the available VRAM and picks a lattice size
for the density visualizer:

| Tier      | VRAM     | Lattice       |
|-----------|----------|---------------|
| `MINIMAL` | < 1 GB   | 9³            |
| `LOW`     | 1–2 GB   | 13³           |
| `MEDIUM`  | 2–4 GB   | 17³           |
| `HIGH`    | 4–8 GB   | 25³           |
| `ULTRA`   | 8 GB+    | 33³           |

GPUs with more than 8 GB of VRAM get a larger lattice, up to 37³. Those with
more than 16 GB get up to 41³. The detected tier is printed to the console at
startup.

## Requirements

- A C++20 compiler (GCC 11+, Clang 14+, or MSVC 2022)
- CMake 3.16 or newer
- A GPU and driver that support Vulkan
- Light Vulkan Graphics 2.1.1 or later, installed so that
  `find_package(LightVulkanGraphics CONFIG)` can find it
- [GLFW 3](https://www.glfw.org/)
- [GLM](https://github.com/g-truc/glm)

If Light Vulkan Graphics is installed outside the default search path, point
CMake at it:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/lightVulkanGraphics/install
```

## Building

```bash
cmake -B build
cmake --build build
```

With Ninja on Linux/WSL:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

## Running

```bash
./build/AetherPhysics
```

The app opens on the Aether Density Visualizer. Use the keys above to switch
models.

## Project layout

```
include/   Headers for the physics models and utilities
src/       Implementation files
CMakeLists.txt
```

| Class                     | Role                                             |
|---------------------------|--------------------------------------------------|
| `PhysicsApp`              | Window, input handling and model switching       |
| `PhysicsModel`            | Abstract base class for all simulations          |
| `AetherPhysicsModel`      | Base class for aether-based models               |
| `AetherDensityVisualizer` | 3D aether density lattice with ray tracing       |
| `WavePhysicsModel`        | Wave propagation in solid, fluid and gas media   |
| `GPUCapabilities`         | VRAM detection and lattice size selection        |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). To report a security issue, see
[SECURITY.md](SECURITY.md).

## License

Copyright (C) 2025 Dr. Nathanael John Inkson.

AetherPhysics is licensed under the
[GNU Lesser General Public License v3.0 or later](LICENSE)
(`LGPL-3.0-or-later`).
