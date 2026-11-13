# API Reference Hub

This hub is organized by surface area.

## Studio, Pane, Trace (per-method reference)

- [Studio API Reference](studio_api_reference.md)
  `Studio`, the top-level figure object — one method page per method, with verified
  examples and real rendered output for every overload and option.
- [Pane API Reference](pane_api_reference.md)
  `Pane`, the 2D/3D plotting area — one method page per method (108 pages), with verified
  examples and real rendered output for every overload and option, covering axis
  scale/limits/ticks/labels/grid, legend/caption/cues, alignment/visibility/placement, and
  every 2D and 3D trace-creation method.
- [Trace API Reference](trace_api_reference.md)
  `Trace`, one drawn series/element inside a `Pane` — one method page per method
  (13 pages), covering per-trace styling (`label`, `width`, `stroke`, `palette`,
  `color_limits`, `shade`, `lighting`, `material`, `transparent`, `annotate_levels`) and
  visibility (`visible`/`hide`/`show`).

## Types and options

- [Types and Options Reference](types_api_reference.md)
  The option types the methods above take (`KeyPlace`, `GridStyle`, `ScalePolicy`, `LimitMethod`,
  `TickFormat`, `Lens`, `LightRig`, `ShadeMode`, `LightingMode`, `MaterialProfile`, `Rgba`,
  `PaletteRamp`, `Theme`, and more to come) — one page per type with a table of every value or
  field, and a rendered demonstration of each value.

## flash:: high-level plotting

- [flash API Reference](flash_api_reference.md)
  `pluss::pl::flash`, the ~65-function high-level convenience layer — one method page
  per function (relational, distribution, categorical, regression, matrix, and more to
  come), with verified examples and real rendered output for every overload and option.
  28 of ~65 functions are live in the sidebar today; see that page's Coverage section for
  exactly what's written so far and what's still pending.

```{note}
An earlier generation of this hub linked several auto-generated, single-giant-page
references (`plusspl_core_api`, `flash_api`, `flash_full_overloads_api`, and others)
that used a fictional pseudo-Python signature style not matching the real C++ API. Those
have been removed as stale duplicates of the per-method references above; `flash::`'s 3D
scene/camera/lighting preset helpers and 4D animation/frame-export helpers they described
(`camera3d`, `scene_preset3d`, `trace_preset3d`, and the `hyper4d` family) are real and
still need their own hand-authored pages — tracked as follow-up work, not dropped.
```

## Header entry points

```cpp
#include <pluss/pl/studio.h>
#include <pluss/pl/flash.h>
```
