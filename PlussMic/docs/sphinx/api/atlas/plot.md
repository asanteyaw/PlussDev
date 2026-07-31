<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fd312dcc243a -->

# Atlas::plot

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::plot() const -> AtlasPlotAccessor
Atlas::plot(const AtlasPlotOptions& options) const -> AtlasPlotAccessor
```

## What It Does

The entry point into the atlas-level plotting accessor — depth-stack-aware charts
(per-frame line/scatter/etc. via `frames()`, 3D surface/wireframe/point-cloud views across
depth via `surface()`/`wireframe()`/`cloud3()`, and depth-axis-specific chart types),
returning `pluss::pl::Studio` figures (or a `DataFramePlotAccessor` for a single selected
frame via `frames(index)`), the same figure/export mechanics documented on
[`DataFramePlotAccessor`](../dataframe/plot.md). Every method on the returned
`AtlasPlotAccessor` has its own page, with real rendered charts, under
[`AtlasPlotAccessor` API Reference](../../atlas_plot_accessor_api_reference).

## Parameters

**options** (`const AtlasPlotOptions&`, optional)
    A bundle of per-chart-type option structs (`figure`, `frames`, `depth_axis`, `surface`,
    `wireframe`, `cloud3`, `depth`, `depth_compare`, `depth_facet`, `depth_rolling`,
    `triplot`) — each field configures the corresponding method on the returned accessor.

## Returns

`AtlasPlotAccessor`
    A binder for the current atlas — call one of its methods to build a specific chart.

## Examples

Example 1. `plot()` binds without rendering; `frames(index)` narrows to a single frame's
own `DataFramePlotAccessor` for the familiar 2D chart methods.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({1.0, 2.0}))},
});
auto atlas = pluss::table::Atlas({f0});

auto frame_plot = atlas.plot().frames(0);
```

## Notes

- `plot()` alone does no work — it only binds the atlas; call one of the returned
  accessor's methods to actually build a chart.
- Every method except [`triplot()`](../atlas-plot/triplot.md) respects
  `options.depth_axis`, selecting and ordering which frames participate —
  `triplot()` always pools every frame's rows together instead.

## See Also

- [`AtlasPlotAccessor` API Reference](../../atlas_plot_accessor_api_reference): every
  chart method on the accessor this call returns, each with a real rendered example.
- [`DataFrame::plot`](../dataframe/plot.md): the single-frame delegate `frames(index)`
  narrows down to, and the canonical documentation for `Studio`'s view/save mechanics.
- [`Atlas::features`](features.md): narrow to specific features before plotting, if a chart
  method doesn't take a feature selector of its own.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nfeatures.html">
    <span class="label">Previous</span>
    <span class="title">nfeatures</span>
  </a>
  <a class="next-link" href="relabel_depth.html">
    <span class="label">Next</span>
    <span class="title">relabel_depth</span>
  </a>
</div>
