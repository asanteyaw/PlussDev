# PlussView Architecture (Foundation)

This document captures the initial cross-library viewer architecture.

## Why this module exists

Both PlussPl and PlussMic currently implement native popup windows separately.
PlussView provides one shared host layer so visual behavior and capability checks
are consistent across libraries.

## Layering

1. `PlussPl` / `PlussMic`
2. `PlussView` API
3. `pluss::view::window` runtime and software renderer (primary path)
4. Legacy platform host layer (`show_html_window`) as migration fallback
5. OS window runtime

## MATLAB-inspired patterns used

- Host abstraction that isolates rendering engines from window backends
- Platform host selection at runtime/compile-time boundaries
- Centralized lifecycle ownership for window + save actions

## Current file layout

- `PlussView/include/pluss/view/window.h`
- `PlussView/include/pluss/view/window/*`
- `PlussView/src/window/core/runtime.cpp`
- `PlussView/src/window/render/*`
- `PlussView/src/window/scene/scene.cpp`
- `PlussView/src/core/html_window.cpp` (legacy compatibility)
- `PlussView/src/platform/*` (legacy compatibility)

## Planned tranches

1. Anti-aliased primitives + clipping + marker families
2. Text shaping + math layout + font fallback families
3. Independent shell adapters (macOS/Linux/Windows) with unified event pump
4. Unified export pipeline (PNG/SVG/PDF) from in-house frame/scene
5. Accessibility, DPI, keyboard navigation, and perf profiling
6. Regression visual tests + interaction tests

## Expected patch envelope

- Full in-house path to elite maturity: ~24-36 patches with aggressive family bundling.
