# AetherPhysics Development Guide

## Git Workflow
**IMPORTANT: Do not commit code automatically.** The user will handle all git commits manually. 

After making code changes:
1. Build and test the code
2. Report what was changed and whether it works
3. Wait for the user to review and commit
4. Only commit if explicitly asked by the user

This applies to:
- Bug fixes
- New features
- Refactoring
- Build configuration changes
- Any other code modifications

## Project Structure
- `/include/` - Header files for physics models and utilities
- `/src/` - Implementation files
- `/build/` - Build artifacts (excluded from git via .gitignore)

## Dependencies
- Light Vulkan Graphics 2.1.1 or later (`find_package(LightVulkanGraphics 2.1.1 CONFIG REQUIRED)`), which provides the renderer, `GraphicsModel` and the `lightGraphics::ui` widgets. 2.1.1 is the first version with `VolumeColorSource` and the wait-for-GPU destroy fix, both of which the fog needs.
- GLFW 3 and GLM
- C++20, CMake 3.16+

## Application and Physics Models
- `PhysicsApp` - Owns the window, polls the keyboard each frame and switches models
- `PhysicsModel` - Abstract base class for all physics simulations (`initialize`/`update`/`cleanup`). Also provides the "About this view" explanation panel: `createInfoPanel()` (fixed paragraphs), `setInfoDetail()` (the paragraph that follows the current view) and `destroyInfoPanel()`, which each model's `cleanup()` must call. Keep the panel text in step with what the simulation actually does.
- `AetherPhysicsModel` - Base for aether-based models
- `AetherDensityVisualizer` - Aether density field with a light ray (default model, `F1`). Two display modes toggled with `V`: volumetric fog (the default) or the orbiting particle lattice, coloured by angular speed (blue slowest, orange fastest). The fog shows the density excess using the library's volume renderer (`createTexture3D`/`createVolume`), with an RGBA8 texture and `VolumeColorSource::TextureRgba`: RGB is the orbital-speed colour, alpha is the density excess. The fog volume is not a scene object, so `cleanup()` must destroy it; `clearObjects()` does not.
- `WavePhysicsModel` - Wave physics (solid/viscous/liquid/gas/supersolid) with an on-screen control panel (`F2`)
- `GPUCapabilities` - VRAM detection and lattice resolution selection

## Keyboard Input
- All key handling starts in `PhysicsApp::handleKeyboardInput()`. It updates `keysPressed`/`keysJustPressed`/`keysJustReleased` for every key at the start of each frame. Use the `keysJustPressed` edge flags for one-shot actions.
- Global keys: `F1`/`F2` switch models, `F3` prints the menu, `Esc` quits.
- Light Vulkan Graphics reads some keys itself on every frame: `W`/`A`/`S`/`D`/`Q`/`E` (camera), top-row `1`-`4` (render mode), `N`/`P`/`O` and the arrow keys (rigged animation), `Left Shift` (camera speed). Don't bind app shortcuts to these.
- Model-specific keys only apply while that model is active. `WavePhysicsModel::handleKeyPress()` owns the wave bindings. Keep it, `WavePhysicsModel::printModeMenu()` and the Controls section of `README.md` in sync.

## Build and Test
```bash
cmake -B build
cmake --build build
./build/AetherPhysics
```

## GPU Capabilities
The application automatically detects GPU VRAM and scales particle count:
- MINIMAL: <1GB → 9³ particles
- LOW: 1-2GB → 13³ particles
- MEDIUM: 2-4GB → 17³ particles
- HIGH: 4-8GB → 25³ particles
- ULTRA: 8GB+ → 33³ particles

GPUs with more than 8 GB get up to 37³ particles, and more than 16 GB up to 41³.

## License Headers
Every source file (`.h`, `.cpp`, `CMakeLists.txt`) starts with the full `LGPL-3.0-or-later` notice naming "AetherPhysics". Copy it from an existing file when adding new ones.
