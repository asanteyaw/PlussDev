<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a6fac50d76e6 -->

# AtlasPlotAccessor::wireframe

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::wireframe(std::string_view feature) const -> pluss::pl::Studio
AtlasPlotAccessor::wireframe(std::string_view feature, const Wireframe3DPlotOptions& options) const -> pluss::pl::Studio
```

## What It Does

Draws the same `[depth × case]` grid as [`surface()`](surface.md) — case axis on x,
selected depth positions on y, `feature`'s values as height — but rendered as
`flash::lattice` (mesh edges only) instead of `surface()`'s `flash::sheet` (a filled
mesh). Grid construction (`build_depth_case_grid`) is identical between the two
methods; only the rendering primitive differs.

```{important} Returns a Studio, not printable output
Like every method on `AtlasPlotAccessor`, this builds and returns a `pluss::pl::Studio` —
call `.show()`/`.save_svg(path)`/etc. to view or persist it.
```

## Parameters

**feature** (`std::string_view`)
    The numeric feature supplying each grid cell's height. Must be present, with a
    matching case count, in every frame `options.depth_axis` selects.

**options** (`const Wireframe3DPlotOptions&`)
    `scene`/`trace` are the shared `Plot3DSceneOptions`/`Plot3DTraceOptions` structs
    also used by [`surface()`](surface.md)/[`cloud3()`](cloud3.md) — see
    [`DataFramePlot3DAccessor::line`](../dataframe-plot3d/line.md) for the full field
    breakdown.

## Returns

`pluss::pl::Studio`
    A figure with one 3D pane containing the `[depth × case]` grid rendered as mesh
    edges.

## Examples

Example 1. The same grid as [`surface()`](surface.md)'s example, rendered as a
wireframe instead of a filled mesh.

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

auto chart = atlas.plot().wireframe("temp_c");
chart.save_svg("wireframe_temp.svg");
```

Same underlying `[5 × 5]` grid as [`surface()`](surface.md)'s example (values rising
along both the case axis and the depth axis), but visible here as mesh edges rather
than a filled surface.

![Output of Example 1](img/wireframe-1.svg)

## Notes

- `feature` must exist, with the same case count, in every frame `options.depth_axis`
  selects — a missing feature or mismatched case count throws, same as
  [`surface()`](surface.md).
- Grid construction is identical to [`surface()`](surface.md)'s — the two methods
  differ only in rendering primitive.
- `options.depth_axis` controls which frames form the grid's y-axis, and in what
  order.

## See Also

- [`AtlasPlotAccessor::surface`](surface.md): the identical grid, rendered as a
  filled mesh instead of edges.
- [`AtlasPlotAccessor::cloud3`](cloud3.md): raw per-frame points instead of an
  aggregated grid.
- [`DataFramePlot3DAccessor::wireframe`](../dataframe-plot3d/wireframe.md): the
  single-frame wireframe method this generalizes across the depth stack.
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="triplot.html">
    <span class="label">Previous</span>
    <span class="title">triplot</span>
  </a>
</div>
