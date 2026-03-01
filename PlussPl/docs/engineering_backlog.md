# PlussPl Engineering Backlog

## Patch Sequence

This backlog is ordered. Each patch should leave the library shippable.

## Patch 1: RenderSurface2D

### Goal

Unify SVG and PDF screen-space drawing ops behind a shared `RenderSurface2D` interface.

### Why

- Current SVG/PDF renderers duplicate too much logic.
- `2D` cannot serve `3D` and `4D` cleanly without a stable screen-space abstraction.
- This is the base delegation object for all higher layers.

### Deliverables

- Introduce common 2D geometry/style structs.
- Introduce `RenderSurface2D` interface.
- Add SVG and PDF implementations.
- Route 2D renderer usage through the interface.

### Target Files

- `PlussPl/src/plot/modules/backend/render_surface_2d.h`
- `PlussPl/src/plot/modules/backend/svg/primitives.h`
- `PlussPl/src/plot/modules/backend/pdf/primitives.h`
- `PlussPl/src/plot/modules/backend/svg/render_2d.h`
- `PlussPl/src/plot/modules/backend/pdf/render_2d.h`

### Status

- `completed`

## Patch 2: Tick And Axis Policy

### Goal

Move ticks and formatters out of ad-hoc renderer logic and into delegated axis policy objects.

### Deliverables

- `AxisModel`
- `TickLocator`
- `TickFormatter`
- Linear, log, categorical, and scientific policies
- Shared tick layout for SVG/PDF

### Dependencies

- Patch 1

### Target Files

- `PlussPl/src/plot/modules/axes.h`
- `PlussPl/src/plot/modules/backend/render_surface_2d.h`
- `PlussPl/src/plot/modules/backend/svg/render_2d.h`
- `PlussPl/src/plot/modules/backend/pdf/render_2d.h`

### Status

- `completed`

## Patch 3: Layout Engine

### Goal

Create a reusable layout layer for figure margins, subplot packing, and reserved areas.

### Deliverables

- `LayoutBox`
- Title reservation
- Shared axis spacing
- Outside legend slots
- Colorbar reservation

### Dependencies

- Patch 2

### Target Files

- `PlussPl/src/plot/modules/backend/layout_2d.h`
- `PlussPl/src/plot/modules/backend/svg/render_2d.h`
- `PlussPl/src/plot/modules/backend/pdf/render_2d.h`
- `PlussPl/src/plot/modules/figure2d.h`

### Status

- `completed`

## Patch 4: Artist System

### Goal

Move plot trace rendering into delegated artist objects instead of giant renderer branches.

### Deliverables

- `Artist2D` base
- `LineArtist`
- `ScatterArtist`
- `BarArtist`
- `HistArtist`
- Legend artist split out

### Dependencies

- Patch 3

### Target Files

- `PlussPl/src/plot/modules/artists/`
- `PlussPl/src/plot/modules/backend/render_surface_2d.h`
- `PlussPl/src/plot/modules/backend/svg/render_2d.h`
- `PlussPl/src/plot/modules/backend/pdf/render_2d.h`
- `PlussPl/src/plot/modules/trace.h`

### Status

- `completed`

## Patch 5: Raster Backend

### Goal

Add serious raster export instead of the current unsupported `save_png()` path.

### Deliverables

- PNG backend
- Text metric strategy
- Antialiasing path
- Integration with `Figure2D::save_png()` and `Figure3D::save_png()`

### Dependencies

- Patch 4

### Status

- `completed`

## Patch 6: Serious 2D Core

### Goal

Expand 2D plotting beyond line/scatter/bar/hist into serious analytical graphics.

### Deliverables

- Heatmaps
- Filled regions
- Bands
- Step plots
- Error bars
- Annotations/arrows

### Dependencies

- Patch 4
- Patch 5 for raster polish

### Status

- `pending`

## Patch 7: Style And Theme System

### Goal

Centralize palette, marker, line-style, and font decisions.

### Deliverables

- `StyleResolver`
- Theme presets
- Marker catalog
- Better defaults
- User-overridable style policies

### Dependencies

- Patch 4

### Status

- `pending`

## Patch 8: Legend And Colorbar System

### Goal

Make legends and colorbars serious first-class layout objects.

### Deliverables

- Outside legends
- Legend titles and style samples
- Scalar color mapping
- 2D colorbars

### Dependencies

- Patch 3
- Patch 4
- Patch 7

### Status

- `pending`

## Milestone: Serious Static 2D

After patch 8, `PlussPl` should be a serious static 2D plotting library.

## Patch 9: Projector3D

### Goal

Create a real delegated 3D projector and camera model.

### Deliverables

- `Camera3D`
- `Projector3D`
- View transforms
- Perspective and orthographic projection

### Dependencies

- Patch 1
- Patch 2

### Status

- `pending`

## Patch 10: Artist3D

### Goal

Move 3D rendering into delegated spatial artists that hand screen-space work to `2D`.

### Deliverables

- `Line3Artist`
- `Scatter3Artist`
- `SurfaceArtist`
- `MeshArtist`

### Dependencies

- Patch 4
- Patch 9

### Status

- `pending`

## Patch 11: Depth And Occlusion

### Goal

Add real scene ordering for static exports.

### Deliverables

- `DepthSorter3D`
- Hidden-line strategy
- Painter ordering for triangles, segments, and points

### Dependencies

- Patch 9
- Patch 10

### Status

- `pending`

## Patch 12: Serious 3D Core

### Goal

Get from toy 3D to serious exportable 3D.

### Deliverables

- Colormapped surfaces
- Mesh and wireframe options
- Contour-on-surface options
- 3D colorbars

### Dependencies

- Patch 8
- Patch 10
- Patch 11

### Status

- `pending`

## Milestone: Serious Static 3D

After patch 12, `PlussPl` should have respectable static 3D plotting.

## Patch 13: Encoding4D

### Goal

Define the official 4D semantics for the library.

### Deliverables

- `w -> color`
- `w -> size`
- `w -> slice`
- `w -> facet`
- `w -> frame`

### Dependencies

- Patch 7
- Patch 12

### Status

- `pending`

## Patch 14: View4D

### Goal

Generate realizable 3D scenes or 2D projections from 4D data.

### Deliverables

- `View4D`
- 4D-to-3D scene generation
- 4D-to-2D slicing/faceting adapters

### Dependencies

- Patch 13

### Status

- `pending`

## Patch 15: 4D UX

### Goal

Make 4D usable instead of merely possible.

### Deliverables

- Slice stacks
- Small multiples
- Frame export hooks
- 4D legends and color scales

### Dependencies

- Patch 14

### Status

- `pending`

## Milestones Summary

- Serious 2D: patch `8`
- Serious 3D: patch `12`
- First credible 4D system: patch `15`

## Immediate Next Work

1. Start patch 6 so `PlussPl` can move beyond line/scatter/bar/hist into heatmaps, fills, bands, steps, and annotations.
2. Keep patch 5’s raster backend honest by ensuring new 2D features render through both vector and PNG paths.
3. Use patch 6 to make the demos unmistakably heavier and more publication-grade, not just architecturally cleaner.
