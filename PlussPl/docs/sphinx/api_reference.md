# API Reference

This page mirrors a seaborn-style API hub: grouped by interface family, with focused pages per major surface.

## Core interface

- [PlussPl core API](plusspl_core_api.md)
  `Studio`, `Pane`, `Trace`, rendering/export methods, pane nesting.

## Flash function interface

- [Flash plotting API](flash_api.md)
  relational, distribution, categorical, regression, grid/pair/joint helpers, matrix/dataset helpers.

## Theme and context

- [Flash theming API](flash_theme_api.md)
  style/context/palette controls and `set_theme` behavior.

## 3D and geometric 4D projection rendering

- [Flash 3D and 4D API](flash_scene_animation_api.md)
  scene presets, trace presets, `animate3`, `animate4`, and frame export.

## Header entry points

```cpp
#include <pluss/pl/studio.h>    // PlussPl core
#include <pluss/pl/flash.h>     // Flash umbrella API
```

## Notes

- The API pages here are reference-first and intentionally concise.
- Examples in this reference use existing demo style from `PlussPl/examples/*`.
