<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 04f89da2d9e2 -->

# Catalog::plot3d

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
CatalogPlot3DAccessor Catalog::plot3d() const
CatalogPlot3DAccessor Catalog::plot3d(const pluss::table::DataFramePlotOptions& options) const
```

## What It Does

The entry point into the catalog-level 3D plotting accessor — key-based
scatter3d/line3d/hist3d/surface/wireframe charts, returning `pluss::pl::Studio` figures. Like
[`plot()`](plot.md), reuses `pluss::table::DataFramePlotOptions` directly — there is no
separate `CatalogPlotOptions` type. Every method on the returned `CatalogPlot3DAccessor` has
its own page, with real rendered charts, under
[`CatalogPlot3DAccessor` API Reference](../../catalog_plot3d_accessor_api_reference).

## Parameters

**options** (`const pluss::table::DataFramePlotOptions&`, optional)
    Shared defaults applied to every chart method called on the returned accessor. Note that
    `surface()`/`wireframe()` forward this same `options` value on to a `DataFrame::plot3d()`
    call internally (via `tabelize()`), while `scatter()`/`line()`/`hist()` use it only for
    figure-level sizing/layout.

## Returns

`CatalogPlot3DAccessor`
    A binder for the current catalog — call one of its methods to build a specific 3D chart.

## Examples

Example 1. `plot3d()` binds without rendering; the returned accessor's methods do the actual
chart-building.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "score"_k = pluss::kindle::Stream(pluss::make_series({0.5, 0.7, 0.9})),
});

auto chart = ct.plot3d().scatter("id", "metric", "score");
```

## Notes

- `plot3d()` alone does no work — it only binds the catalog; call one of the returned
  accessor's methods to actually build a chart.
- Reuses `pluss::table::DataFramePlotOptions` directly — there is no `CatalogPlot3DOptions`.
- For 2D charts (line/step/area/scatter/bar/hist/density/corr_heatmap/scatter_matrix/
  auto_plot), see [`Catalog::plot`](plot.md) instead.

## See Also

- [`CatalogPlot3DAccessor` API Reference](../../catalog_plot3d_accessor_api_reference): every
  chart method on the accessor this call returns, each with a real rendered example.
- [`Catalog::plot`](plot.md): the 2D counterpart of this entry point.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot.html">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="query.html">
    <span class="label">Next</span>
    <span class="title">query</span>
  </a>
</div>
