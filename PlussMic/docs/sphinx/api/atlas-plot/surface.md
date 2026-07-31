<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f4363fda9bad -->

# AtlasPlotAccessor::surface

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::surface(std::string_view feature) const -> pluss::pl::Studio
AtlasPlotAccessor::surface(std::string_view feature, const Surface3DPlotOptions& options) const -> pluss::pl::Studio
```

## What It Does

Draws a continuous 3D surface mesh over a `[depth × case]` grid: the case axis
(each frame's row position) becomes the grid's x-axis, the depth axis (which frames
were selected, via `options.depth_axis`) becomes the y-axis, and each cell's height
is `feature`'s value at that `(case, frame)` position. Internally this builds the
grid via a `build_depth_case_grid` helper shared with [`wireframe()`](wireframe.md),
then renders it with `flash::sheet` (a filled mesh) instead of `wireframe()`'s
`flash::lattice` (mesh edges only).

```{important} Returns a Studio, not printable output
Like every method on `AtlasPlotAccessor`, this builds and returns a `pluss::pl::Studio` —
call `.show()`/`.save_svg(path)`/etc. to view or persist it.
```

## Parameters

**feature** (`std::string_view`)
    The numeric feature supplying each grid cell's height. Must be present, with a
    matching case count, in every frame `options.depth_axis` selects.

**options** (`const Surface3DPlotOptions&`)
    `scene`/`trace` are the shared `Plot3DSceneOptions`/`Plot3DTraceOptions` structs
    also used by [`wireframe()`](wireframe.md)/[`cloud3()`](cloud3.md) — see
    [`DataFramePlot3DAccessor::line`](../dataframe-plot3d/line.md) for the full field
    breakdown (camera, theme, lighting, shading, material).

## Returns

`pluss::pl::Studio`
    A figure with one 3D pane containing a single continuous surface mesh over the
    `[depth × case]` grid.

## Examples

Example 1. A surface over a 5-frame, 5-case-per-frame atlas, where `temp_c` grows
along both the case axis and the depth axis.

```cpp
using namespace pluss::table;

std::vector<DataFrame> frames;
for (int i = 0; i < 5; ++i) {
    double base = static_cast<double>(i);
    frames.push_back(DataFrame({
        {"temp_c", Thread(pluss::make_series({18.0 + base, 19.0 + base, 20.0 + base, 21.0 + base, 22.0 + base}))},
    }));
}
auto atlas = Atlas(frames, Thread(pluss::make_note({"day1", "day2", "day3", "day4", "day5"})));

auto chart = atlas.plot().surface("temp_c");
chart.save_svg("surface_temp.svg");
```

Every frame's `temp_c` values increase from case `0` to case `4` (a rising ramp
along x), and every frame is itself shifted `1.0` higher than the previous one (a
rising ramp along y, the depth axis) — the surface tilts upward in both directions
at once.

![Output of Example 1](img/surface-1.svg)

## Notes

- `feature` must exist, with the same case count, in every frame `options.depth_axis`
  selects — a missing feature or mismatched case count throws.
- Grid construction is identical to [`wireframe()`](wireframe.md)'s — the two
  methods differ only in how the resulting grid is rendered (filled mesh vs. mesh
  edges).
- `options.depth_axis` (an `AtlasDepthAxisOptions`, shared across every 3D/depth
  method on this accessor) controls which frames form the grid's y-axis, and in what
  order.

## See Also

- [`AtlasPlotAccessor::wireframe`](wireframe.md): the identical grid construction,
  rendered as mesh edges instead of a filled surface.
- [`AtlasPlotAccessor::cloud3`](cloud3.md): raw per-frame points instead of an
  aggregated grid.
- [`DataFramePlot3DAccessor::surface`](../dataframe-plot3d/surface.md): the
  single-frame surface method this generalizes across the depth stack (there, the
  grid comes from pivoting two features against each other rather than case-vs-depth).
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frames.html">
    <span class="label">Previous</span>
    <span class="title">frames</span>
  </a>
  <a class="next-link" href="triplot.html">
    <span class="label">Next</span>
    <span class="title">triplot</span>
  </a>
</div>
