# API Reference Hub

This hub is organized by surface area.

## Core plotting engine

- [PlussPl Core API](plusspl_core_api.md)
  `Studio`, `Pane`, and `Trace` orchestration and export.
- [PlussPl Core Controls API](plusspl_core_controls_api.md)
  expanded per-method controls for axis/layout/legend/ticks/visibility and studio wiring.
- [PlussPl Object/State API](plusspl_object_model_api.md)
  accessors + state-manipulation methods for `Studio`, `Pane`, `Trace`, `Theme`, `PaletteRamp`, `Lens`, and `LightRig`.
- [PlussPl Core Full Overload Reference](plusspl_core_full_overloads_api.md)
  exhaustive one-section-per-overload catalog for all documented core call forms.

## Flash high-level plotting

- [Flash Plotting API](flash_api.md)
  2D/3D plotting families, matrix/dataset helpers, diagnostics.
- [Flash Full Overload Reference](flash_full_overloads_api.md)
  exhaustive one-section-per-overload catalog for flash plotting/style/scene call forms.

## Flash style and context

- [Flash Theming API](flash_theme_api.md)
  context scaling, style presets, palette/theme application.
- [Flash Helper and Utility API](flash_helpers_expanded_api.md)
  palette builders, dataset utilities, kind parsers, and extra style/navigation helpers.

## Flash 3D and 4D motion + frame export

- [Flash 3D and 4D API](flash_scene_animation_api.md)
  scene presets, trace presets, live simulation, frame export.
- [Flash Scene Presets and 3D Controls API](flash_scene_presets_api.md)
  explicit camera, lighting, theme, scene, and trace preset helpers.

## Header entry points

```cpp
#include <pluss/pl/studio.h>
#include <pluss/pl/flash.h>
```
