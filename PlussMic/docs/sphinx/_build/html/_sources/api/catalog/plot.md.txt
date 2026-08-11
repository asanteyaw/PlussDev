<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9e2e04b5507d -->

# Catalog::plot

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::plot() const -> CatalogPlotAccessor
Catalog::plot(const pluss::table::DataFramePlotOptions& options) const -> CatalogPlotAccessor
```

## What It Does

The entry point into the catalog-level 2D plotting accessor — key-based line/step/area/
scatter/bar/hist/density/corr_heatmap/scatter_matrix/auto_plot charts, returning
`pluss::pl::Studio` figures. There is no separate `CatalogPlotOptions` type — the accessor
reuses `pluss::table::DataFramePlotOptions` as-is, the same struct
[`DataFrame::plot`](../dataframe/plot.md) takes, so fields like `default_x_feature` still
apply even though what they resolve is a catalog key rather than a frame feature. Every
method on the returned `CatalogPlotAccessor` has its own page, with real rendered charts,
under [`CatalogPlotAccessor` API Reference](../../catalog_plot_accessor_api_reference).

## Parameters

**options** (`const pluss::table::DataFramePlotOptions&`, optional)
    Shared defaults (`default_x_feature`, `default_hue_feature`, tick/label formatting,
    figure size, ...) applied to every chart method called on the returned accessor.

## Returns

`CatalogPlotAccessor`
    A binder for the current catalog — call one of its methods to build a specific chart.

## Examples

Example 1. `plot()` binds without rendering; the returned accessor's methods do the actual
chart-building.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "day"_k = pluss::kindle::Stream(pluss::make_note({"Mon", "Tue", "Wed"})),
    "sales"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 1.5})),
});

auto chart = ct.plot().line("sales", "day");
```

## Notes

- `plot()` alone does no work — it only binds the catalog; call one of the returned
  accessor's methods to actually build a chart.
- Reuses `pluss::table::DataFramePlotOptions` directly — there is no `CatalogPlotOptions`.
- For 3D charts (scatter3d/line3d/hist3d/surface/wireframe), see
  [`Catalog::plot3d`](plot3d.md) instead.

## See Also

- [`CatalogPlotAccessor` API Reference](../../catalog_plot_accessor_api_reference): every
  chart method on the accessor this call returns, each with a real rendered example.
- [`Catalog::plot3d`](plot3d.md): the 3D counterpart of this entry point.
- [`DataFrame::plot`](../dataframe/plot.md): the shared `DataFramePlotOptions` struct's
  canonical documentation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
  <a class="next-link" href="plot3d.html">
    <span class="label">Next</span>
    <span class="title">plot3d</span>
  </a>
</div>
