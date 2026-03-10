# API Reference Hub

This hub is organized by surface area.

## Core plotting engine

- [PlussPl Core API](plusspl_core_api.md)
  `Studio`, `Pane`, and `Trace` orchestration and export.
- [PlussPl Core Controls API](plusspl_core_controls_api.md)
  expanded per-method controls for axis/layout/legend/ticks/visibility and studio wiring.
- [PlussPl Object/State API](plusspl_object_model_api.md)
  accessors + state-manipulation methods for `Studio`, `Pane`, `Trace`, `Theme`, `PaletteRamp`, `Lens`, and `LightRig`.

## Flash high-level plotting

- [Flash Plotting API](flash_api.md)
  2D/3D plotting families, matrix/dataset helpers, diagnostics.

## Flash style and context

- [Flash Theming API](flash_theme_api.md)
  context scaling, style presets, palette/theme application.

## Flash 3D and 4D motion + frame export

- [Flash 3D and 4D API](flash_scene_animation_api.md)
  scene presets, trace presets, live simulation, frame export.

## Generated Outputs Policy

All output images shown in the API pages are produced from runnable snippets in:

`PlussPl/examples/docs/flash_api_capture_demo.cpp`

Generated artifacts are stored under:

`PlussPl/docs/sphinx/_static/plots/methods/`

## Header entry points

```cpp
#include <pluss/pl/studio.h>
#include <pluss/pl/flash.h>
```
