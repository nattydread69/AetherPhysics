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

## Physics Models
- `PhysicsModel` - Base abstract class for all physics simulations
- `AetherPhysicsModel` - Base for aether-based models
- `AetherDensityVisualizer` - 3D lattice of blue aether particles
- `WavePhysicsModel` - Wave physics (solid/viscous/liquid/gas/supersolid)
- `DemoModel` - Basic demonstration with simple shapes

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

---

# Development Guidelines

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
