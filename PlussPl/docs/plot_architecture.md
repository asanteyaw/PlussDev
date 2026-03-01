# PlussPl Plot Architecture

## Design Direction

`PlussPl` should not become a pile of isolated plotting systems. The plotting stack should be layered by responsibility and delegation:

`RenderSurface2D -> PlotArtists2D -> Projector3D -> SceneComposer3D -> Encoding4D/View4D`

The library should prefer composition over inheritance and small specialized objects over large god-objects.

## Ownership Model

### 2D Owns Screen-Space Truth

`2D` owns:

- Layout
- Ticks and tick formatting
- Axis labels
- Legends
- Text
- Colorbars
- Style resolution
- Raster/vector output
- Final drawing surfaces

`2D` is the screen-space servant for higher dimensions.

### 3D Owns Spatial Truth

`3D` owns:

- Camera
- Projection
- Depth sorting
- Surface and mesh semantics
- Spatial scene composition

`3D` should delegate screen-space rendering to `2D` primitives. It should not reimplement text, legends, ticks, export, or layout.

### 4D Owns Encoding Truth

`4D` owns:

- Mapping the fourth dimension to color
- Mapping the fourth dimension to size
- Small multiples and faceting
- Slicing
- Animation frames
- Time scrub style views
- Iso-volume or threshold selection semantics

`4D` should delegate each realized view to `3D`, which in turn delegates final drawing to `2D`.

## Core Delegation Rule

Large objects orchestrate. Small objects do the real work.

That means:

- `Figure2D` delegates to layout, axis, artist, and render-surface objects.
- `Figure3D` delegates to camera, projector, depth sorter, and artist objects, then passes screen-space work to `2D`.
- `Figure4D` delegates to encoders and view generators, then hands realized scenes to `3D`.

## Target Object Model

### Small Objects

#### Axis and Tick System

- `AxisModel`
  - Limits
  - Scale
  - Inversion
  - Tick policy
  - Formatter
- `TickLocator`
  - Linear
  - Log
  - Categorical
  - Scientific
- `TickFormatter`
  - Numeric
  - Scientific
  - Log
  - Categorical

#### Layout and Style

- `LayoutBox`
  - Margins
  - Subplot packing
  - Legend placement
  - Colorbar placement
- `StyleResolver`
  - Palette
  - Markers
  - Line styles
  - Themes
  - Defaults

#### 2D Drawing

- `RenderSurface2D`
  - Abstract screen-space draw interface
- `Canvas2D`
  - Low-level draw ops on top of render surfaces
- `Artist2D`
  - `LineArtist`
  - `ScatterArtist`
  - `BarArtist`
  - `HeatmapArtist`
  - `TextArtist`
  - `LegendArtist`
  - `ColorbarArtist`

#### 3D Drawing

- `Camera3D`
  - Azimuth
  - Elevation
  - Distance
  - Perspective/orthographic mode
- `Projector3D`
  - World-to-screen projection
- `DepthSorter3D`
  - Triangle, segment, and point ordering
- `Artist3D`
  - `Line3Artist`
  - `Scatter3Artist`
  - `SurfaceArtist`
  - `MeshArtist`

#### 4D Encoding

- `Encoding4D`
  - `w -> color`
  - `w -> size`
  - `w -> slice`
  - `w -> facet`
  - `w -> frame`
- `View4D`
  - Produces one or more `Scene3D` instances from a 4D dataset

### Large Objects

- `Figure2D`
- `Figure3D`
- `Figure4D`

These should remain orchestration layers, not dumping grounds.

## Explicit Non-Goals

- `3D` should not duplicate `2D` text, legend, tick, or export code.
- `4D` should not start as a monolithic “draw a 4D object” subsystem.
- `PlussPl` should not absorb `PlotGenCpp` code directly.

## Relationship To PlotGenCpp

`PlotGenCpp` is useful as a feature and usability reference, but it should remain external for now because:

- Its license is `GPL-3.0`, while the repo is `BSD-3-Clause`.
- Its architecture is `SFML`-centric and 2D-centric.
- Coercing it into `CodeM`/`Series`/delegation-first semantics would cost nearly as much as building the right internal architecture.

The right use is:

- Copy ideas
- Do not copy code
- Keep `PlussPl` native to the `Pluss` ecosystem

## Serious Plotting Definition

### Serious 2D

Serious 2D means:

- Proper axes and tick policies
- Real subplot layout
- Raster and vector export
- Heatmaps and filled primitives
- Annotations, legends, and colorbars
- Style control that does not look toy-grade

### Serious 3D

Serious 3D means:

- Camera and projection control
- Real scene composition
- Depth sorting
- Surface and mesh support
- Colormaps and scalar mapping
- Reuse of 2D export/layout/text infrastructure

For `PlussPl`, this explicitly includes:

- Implied volatility surfaces
- Option price surfaces
- MATLAB-style simulation trajectories on top of surfaces
- Export quality suitable for research and quant reporting

### Credible 4D

Credible 4D means:

- Explicit and documented 4D encodings
- Delegated realization into 3D scenes or 2D views
- Multiple output modes such as slice, facet, and frame
- No fake “4D renderer” shortcut that breaks the architecture

## Long-Term Rule

Every new plotting capability should answer:

1. Which small object owns this responsibility?
2. Which larger object should delegate to it?
3. Can the next higher dimension reuse it without reimplementation?

If the answer to `3` is no, the design is probably wrong.
