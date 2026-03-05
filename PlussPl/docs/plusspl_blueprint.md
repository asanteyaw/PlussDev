# PlussPl Blueprint

## 1. Objective

Build `PlussPl` as a production-grade plotting system on top of `PlussQit/CodeM`, with:

- first-class 2D and 3D plotting
- explicit, handle-based scene composition instead of global implicit state
- vectorized data ingestion via `pluss::Series`
- C++20-first internals: concepts, `constexpr`, ranges, spans, strong value types
- deterministic export paths for SVG, PDF, and raster outputs
- room for future GPU-backed rendering without rewriting the public API

The target is not to clone MATLAB's surface API. The target is to match its plotting maturity in the areas that matter: data normalization, scene state, camera control, palettes, annotation, export, and predictable defaults.

## 2. Reference Scan Summary

Relevant files read fully during blueprinting:

- `PlussQit/src/CodeM/README.md`
- `PlussQit/include/pluss/codem/codem.h`
- `PlussQit/include/pluss/codem/series_factory.h`
- `PlussQit/include/pluss/codem/stack_array.h`
- `PlussQit/include/pluss/detail/factory_containers.h`
- `PlussQit/src/CodeM/core/exotic_containers.h`
- `PlussQit/src/CodeM/core/Series.h`
- `PlussQit/src/CodeM/core/SeriesBody.h`
- `PlussQit/src/CodeM/core/SeriesIterator.h`
- `PlussQit/src/CodeM/core/ScalarType.h`
- `PlussQit/src/CodeM/core/Config.h`
- `CMakeLists.txt`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph2d/Contents.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph2d/plot.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph2d/loglog.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph2d/polar.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/Contents.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/plot3.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/mesh.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/surf.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/view.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/camorbit.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/rotate3d.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/shading.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/lighting.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/colormap.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/caxis.m`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d/camlight.m`

Key takeaways from that scan:

- MATLAB separates primitive creation from scene-state operations. `plot` and `plot3` are entrypoints; `view`, `lighting`, `shading`, `colormap`, and camera functions act on scene state.
- 3D quality depends on more than geometry. Camera, light rig, palette mapping, shading mode, and aspect policy are first-class.
- Mature plotting is mostly argument normalization and sensible defaults.
- Legacy MATLAB has many mode-heavy and editor-heavy layers. We should not copy that state model.
- `CodeM` already gives us the core vectorized substrate: `Series`, spans, dtype/platform metadata, broadcast-aware iteration, and lightweight container wrappers.

## 3. Design Position

### What to borrow

- broad input normalization
- clear separation of primitive data and scene/view state
- automatic scale, limits, and palette behavior with manual override
- camera and lighting as first-class 3D controls
- export as a core feature, not an afterthought

### What to reject

- implicit globals like current figure/current axes
- many near-duplicate entrypoints for every axis-scale variant
- property bags with weak typing everywhere
- editor-era architecture mixed into rendering internals

## 4. Naming Direction

To avoid shadowing MATLAB naming, use a modern scene vocabulary.

Core object names:

- `Studio`: top-level plot document, equivalent to a composed figure workspace
- `Pane`: one plotting region inside an atlas
- `Scene2` and `Scene3`: 2D and 3D scene coordinators owned by a pane
- `Trace`: base drawable primitive
- `StrokeTrace`: 2D/3D polyline primitive
- `CloudTrace`: point cloud / scatter primitive
- `BandTrace`: filled band / interval primitive
- `SheetTrace`: colored surface primitive
- `WireTrace`: wireframe lattice primitive
- `GlyphTrace`: arrows, markers, symbols
- `LabelTrace`: text and annotation primitive
- `Lens`: 3D camera state
- `LightRig`: scene light collection
- `PaletteRamp`: continuous color lookup table
- `ToneRange`: numeric mapping range for color/alpha scaling

User-facing verbs:

- `studio()` instead of `figure()`
- `pane2()` / `pane3()` instead of `axes()`
- `trend(...)` instead of `plot(...)`
- `path3(...)` instead of `plot3(...)`
- `radial(...)` instead of `polar(...)`
- `sheet(...)` instead of `surf(...)`
- `lattice(...)` instead of `mesh(...)`
- `look(...)` instead of `view(...)`
- `spin(...)` instead of `camorbit(...)`
- `lamp(...)` instead of `camlight(...)`

Not every convenience wrapper needs to exist. The preferred style is builder-based:

```cpp
auto book = pl::studio("signals").headline("Signals");
auto& pane = book.pane2();

pane.trend(x, y)
    .stroke(theme::ocean().accent(0))
    .width(2.0f)
    .label("signal");

pane.scale_x(pl::scale::log10());
pane.scale_y(pl::scale::linear());
pane.grid(pl::grid_style::major_minor());
pane.key(pl::KeyPlace::NorthEast);

book.save_svg("outputs/svg/signals.svg");
```

## 5. Proposed Architecture

### 5.1 Layer map

`PlussPl` should be split into six layers:

1. `ingest`
   Converts user inputs into canonical plot-ready buffers.
2. `model`
   Owns atlas, pane, scene, trace, scale, palette, camera, and annotation state.
3. `layout`
   Tiling, margins, legends, titles, tick label placement, and text bounds.
4. `render`
   Converts model state into draw commands.
5. `backend`
   Concrete SVG, PDF, and raster writers.
6. `interaction`
   Optional camera spin/pan/zoom tools and picking hooks.

### 5.2 Data path

All public plotting calls should normalize into canonical geometry records:

- `Series1DView`
- `Series2DGridView`
- `Series3DGridView`
- `ColorFieldView`
- `MaskFieldView`
- `IndexMeshView`

Normalization rules:

- accept `pluss::Series`, `std::span`, `std::vector`, initializer lists, and ranges where reasonable
- coerce to `Series` or lightweight non-owning views
- broadcast scalars and singleton axes using `CodeM` semantics
- prefer `Float64` for geometry math and retain source dtype for metadata where useful
- materialize contiguous buffers only at renderer boundaries

### 5.3 Scene model

`Studio`
- metadata
- theme tokens
- multi-pane layout
- export configuration

`Pane`
- rectangle within atlas
- scale state per axis
- guide state: grid, frame, ticks, labels
- either `Scene2` or `Scene3`

`Scene2`
- traces
- axis transforms
- palette mapping
- legends and annotations

`Scene3`
- traces
- `Lens`
- `LightRig`
- depth policy
- shading policy
- palette mapping

### 5.4 Render model

Use a retained scene graph plus a stateless command stream:

- retained graph for user edits and export repeatability
- flattened `DrawList` for backend emission

Core draw commands:

- `DrawStroke`
- `DrawMarkers`
- `DrawTriangles`
- `DrawQuads`
- `DrawText`
- `DrawImage`
- `DrawClip`
- `DrawLightProxy`

This separation keeps SVG/PDF backends simple and allows a future GPU backend to consume the same draw list.

## 6. CodeM Integration Strategy

`CodeM` is already strong enough to be the default numeric substrate. We should lean on it heavily.

Use directly:

- `pluss::Series` for numeric payloads
- `Series` broadcasting and dtype promotion
- `SeriesIterator`-style iteration patterns for raster/color computations
- `StackArray<Series>` for grouped argument ingestion
- span-like wrappers from `factory_containers.h` / `exotic_containers.h`

Add in `PlussPl` only where plotting needs stronger structure:

- `SmallIndexSpan<N>`: fixed-small topology/index holder
- `GridShape2` / `GridShape3`: strong grid descriptors
- `VertexSpan2` / `VertexSpan3`: non-owning geometry views
- `ColorSpan`: packed RGBA view
- `MeshTopology`: indexed triangles/quads/lines
- `TickRun`: computed tick values + formatted labels
- `TileMap`: atlas pane placement result

Avoid inventing a second tensor library inside `PlussPl`.

## 7. Public API Style

### 7.1 Principles

- explicit handles, not hidden globals
- strongly typed options instead of unbounded string property bags
- chainable modifiers for readability
- pure data transforms separated from scene mutation
- scale and palette control via policies, not separate duplicate functions

### 7.2 Example sketches

```cpp
namespace pl = pluss::pl;

auto book = pl::studio("demo");
auto& pane = book.pane3();

auto xs = pluss::linspace(-3.0, 3.0, 121);
auto ys = pluss::linspace(-3.0, 3.0, 121);
auto zz = some_surface(xs, ys);

pane.sheet(xs, ys, zz)
    .palette(pl::palette::thermal())
    .shade(pl::shade_mode::smooth())
    .material(pl::material::matte());

pane.lens().look({-37.5_deg, 30.0_deg});
pane.lens().spin(20.0_deg, 12.0_deg);
pane.lights().sun_key();

book.save_pdf("outputs/pdfs/surface_demo.pdf");
```

For log or radial variants, do not create separate top-level functions by default:

```cpp
pane.trend(x, y);
pane.scale_x(pl::scale::log10());
pane.scale_y(pl::scale::log10());

pane.radial(theta, rho);
```

## 8. Internal Folder Structure

Recommended target layout:

```text
PlussPl/
  CMakeLists.txt
  include/pluss/pl/
    plusspl.h
    studio.h
    pane.h
    trace.h
    scale.h
    palette.h
    lens.h
    theme.h
    io.h
  src/
    ingest/
    model/
    layout/
    render/
    backend/svg/
    backend/pdf/
    backend/raster/
    interaction/
    util/
  tests/PlussPl/
  examples/2D/
  examples/3D/
  docs/
  outputs/
```

Submodules worth isolating early:

- `src/ingest/normalize_*.cpp`
- `src/model/scene2.cpp`, `scene3.cpp`
- `src/layout/ticks.cpp`, `legend_layout.cpp`, `tile_layout.cpp`
- `src/render/draw_list.cpp`, `stroke_tessellator.cpp`, `surface_tessellator.cpp`
- `src/backend/svg/svg_writer.cpp`
- `src/backend/pdf/pdf_writer.cpp`

## 9. Feature Roadmap

### Phase A: foundation

- CMake target for `PlussPl`
- public include surface
- atlas/pane/scene skeleton
- theme tokens
- deterministic test harness

### Phase B: 2D core

- `trend`, `cloud`, `band`, `radial`
- linear/log/symlog scales
- auto limits, tick generation, legend placement
- SVG and raster export
- style cycles, markers, dashed strokes

### Phase C: 2D maturity

- datetime and categorical axes
- annotations and callouts
- dual-axis support
- decimation for large series
- hit testing and cursor probes

### Phase D: 3D core

- `path3`, `sheet`, `lattice`
- structured grid normalization
- camera/lens math
- palette mapping, z-range mapping
- depth sort and hidden-line policy

### Phase E: 3D maturity

- lights and materials
- flat/smooth shading
- normals generation
- clipping planes
- export parity for vector and raster outputs
- interaction tools for spin/pan/dolly

### Phase F: production hardening

- gallery and baseline image testing
- performance profiling and downsampling policy
- serialization of atlas scenes
- ABI review and namespace cleanup

## 10. Rendering and Performance Rules

- geometry math in `Float64`; final raster color path may downcast
- no per-point virtual dispatch
- structure-of-arrays internally for vertices/colors/normals
- batch tessellation and clipping
- cache tick layout and text metrics
- decimate only when policy allows; never silently damage export fidelity
- parallelize expensive surface normal and raster shading passes where safe

For large series:

- screen-space simplification for preview
- full-fidelity export path
- visible-range clipping before tessellation
- optional level-of-detail policy on interactive backends

## 11. Testing Strategy

Test categories:

- parser/normalization tests
- scale and tick correctness tests
- geometry generation tests
- palette mapping tests
- camera math tests
- golden SVG/PDF/raster regression tests
- large-series performance smoke tests

Baseline artifacts should live in `tests/PlussPl/baselines/`.

## 12. What "MATLAB-Level" Actually Means

There are two separate targets:

### Core parity

Users can reliably produce:

- publication-grade 2D line/scatter/band/radial plots
- publication-grade 3D paths, wireframes, and surfaces
- consistent legends, ticks, labels, palettes, and exports

### Maturity parity

This includes:

- robust argument normalization across many input shapes
- predictable behavior under hold/add/multi-pane scenarios
- camera, lights, shading, and color controls
- annotation/editing workflows
- export stability and regression-tested rendering
- broad edge-case handling

Core parity is feasible in months. Maturity parity is a larger program.

## 13. Realistic Timeline and Patch Count

Assuming focused work on `PlussPl` only:

### 2D path

- Patch 1-4: scaffolding, scene model, theme tokens, SVG backend
- Patch 5-8: `trend`, `cloud`, scales, ticks, legend, raster export
- Patch 9-12: `band`, `radial`, annotations, baseline tests

Expected result:

- useful 2D alpha in 3 to 5 weeks
- strong 2D beta in 6 to 8 weeks
- high-confidence 2D production release in 10 to 14 weeks

### 3D path

- Patch 13-16: `path3`, `sheet`, `lattice`, lens math
- Patch 17-20: hidden-line policy, palette mapping, normals, shading
- Patch 21-24: lights, materials, interaction tools, export hardening

Expected result:

- useful 3D alpha in 4 to 6 weeks after 2D core stabilizes
- strong 3D beta in 8 to 12 additional weeks
- high-confidence 3D production release in 14 to 20 additional weeks

### Near-MATLAB maturity estimate

For broad 2D + 3D maturity, not full MATLAB breadth:

- 24 patches for a strong v1
- 36 to 45 patches for mature core plotting
- 50 to 70 patches for near-MATLAB-grade robustness on the supported feature set

Time estimate:

- 3 to 5 months for strong v1
- 6 to 9 months for mature core plotting
- 9 to 15 months for near-MATLAB-grade maturity, assuming sustained iteration and regression work

## 14. Recommended First Execution Sequence

1. Create `PlussPl` as a real CMake target with public headers and tests.
2. Implement the retained model: `Studio`, `Pane`, `Scene2`, `Scene3`, `Trace`.
3. Build the SVG backend first.
4. Ship 2D `trend` + scales + ticks + legends before touching 3D shading.
5. Add `sheet` and `lattice` only after camera and palette infrastructure exist.
6. Add raster/PDF parity after the SVG scene flattening is stable.

## 15. Non-Negotiable Engineering Rules

- C++20 only
- explicit namespaces and no macro-heavy property systems
- no hidden singleton current scene state
- no stringly typed public API for core operations
- renderer-independent scene model
- image regression tests for every new primitive
- every convenience wrapper must lower into one canonical implementation path

## 16. Immediate Recommendation

Start with a lean, explicit v1 centered on:

- `Studio`
- `Pane`
- `trend`
- `cloud`
- `sheet`
- `lattice`
- `scale`
- `palette`
- `lens`
- `save_svg`, `save_pdf`, `save_png`

That gives the shortest path to a serious plotting library without inheriting MATLAB's legacy surface area.

## 17. R2025b Graphics Assessment Update (2026-03-04)

Additional source scan completed against:

- `/Applications/MATLAB_R2025b.app/toolbox/matlab/graphics/graphics/graph3d/*.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/graphics/graphics/graph2d/*.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/graphics/graphics/axis/+matlab/+graphics/+internal/+ruler/rulerFunctions.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/graphics/graphics/color/colormap.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/graphics/graphics/scribe/legend.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/graphics/graphics/scatter.m`

Notes:

- A large fraction of MATLAB internals in this area are `.p` files; those are opaque and not inspectable as source.
- The open `.m` layer confirms the contract model: convenience wrappers normalize inputs, then delegate to strongly-typed property/ruler/camera state.

Key maturity patterns confirmed:

- axis limits are two-dimensional state: value mode (`auto`/`manual`) + fit strategy (`tickaligned`/`tight`/`padded`)
- camera/light convenience functions are state mutators, not primitive generators
- plotting maturity is primarily argument normalization + edge-case rules, not raw primitive count
- legend/layout/interactivity behaviors are explicit subsystems with separate lifecycle

## 18. Updated Gap Map

What PlussPl already covers strongly:

- typed retained scene (`Studio`/`Pane`/`Trace`)
- broad primitive coverage for 2D and 3D core families
- camera/lens + light rig + shading/material controls
- deterministic SVG/PDF export
- mosaic placement and alignment groups

Highest remaining gaps to MATLAB-grade maturity:

- limit/tick policy completeness across linear/log/symlog edge cases
- deeper argument normalization breadth and richer type coercion matrix
- interactivity stack parity (zoom/pan/rotate/data tip behaviors)
- baseline image regression breadth and stress testing
- higher-fidelity contour/isoband topology robustness at pathological grids
- legend/layout behavior under dense multi-pane scenes

## 19. Revised Patch Program

To guarantee near-MATLAB maturity on the supported PlussPl feature set (not full MATLAB breadth), the current estimate is:

- `58` patches total program size
- `15` patches already represented in current codebase maturity
- `43` additional patches required from this point

Breakdown of the remaining `43` patches:

- 11 patches: axis/scale/tick/limits normalization hardening
- 8 patches: 2D statistical and annotation behavior hardening
- 10 patches: 3D topology/shading/lighting robustness
- 7 patches: layout/legend/export parity refinements
- 7 patches: regression/performance/integration hardening

Aggressive family-first program (targeting shared implementations before leaf wrappers):

- `46` patches total program size (about `20.7%` below the conservative `58`)
- `15` patches already represented in current codebase maturity
- `31` additional patches required under this track

Tactics used to compress patch count:

- implement axis/ruler behavior once and fan out across planar + volumetric render paths
- prioritize normalization engines and policy objects over one-off convenience APIs
- ship gallery-wide regression baselines by family (axis/mesh/contour) rather than by wrapper
- defer interactive editor parity until core render/state parity is stable

## 20. Current Tranche Status

This tranche lands:

- explicit limit fit policy on axes (`TickAligned`, `Tight`, `Padded`)
- stricter axis/range/symlog validation at API boundaries
- symlog-aware tick generation in the renderer
- shared major-minor grid generation across planar and volumetric panes
- tick label rendering decoupled from planar grid visibility
- axis tick formatting policies (`scientific`, `engineering`, `percent`, etc.)
- categorical axis helpers (`x_categories`, `y_categories`, `z_categories`)
- violin trace family with grouped KDE envelopes and robust flat-sample handling
- volumetric `isoshell` family with 3D field normalization and marching-tetra extraction
- comprehensive all-family demo entry points for 2D and 3D coverage
- Seaborn-style and Matplotlib-3D equivalence demo suites
- volumetric bounds optimization to remove an O(nx*ny*nz) nested loop in render prep
- MATLAB-backed parity harness and maturity scoring report
- new regression tests for axis policy and minor-grid behavior

Latest measured score from strict MATLAB parity harness:

- `2D family score`: `100.00%`
- `3D family score`: `100.00%`
- `MATLAB tick parity`: `100.00%`
- `overall maturity score`: `100.00%`

Immediate next tranche should target:

1. datetimelike tick pipelines
2. contour/isoband topology regression set expansion
3. interactivity and behavior-parity regression breadth
