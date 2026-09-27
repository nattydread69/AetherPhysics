# Contributing

## Scope

- Keep changes focused. Separate refactors from feature work when practical.
- Avoid unrelated formatting churn in touched files.
- Update public-facing docs when the public API, build flags, examples, or packaging behavior changes.

## Build

Linux/WSL quick start:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

## Verification

## Style

- Follow the existing style in the file you are editing.
- Keep comments focused on intent and behavior, not obvious mechanics.
- Prefer compatibility-preserving API changes unless a breaking change is deliberate and documented.

## Packaging

If you change exported headers, install rules, or `find_package()` behavior, make sure the package smoke tests still pass. Those checks validate that an installed tree and a relocated installed tree both remain consumable.
