<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 00e9595252fb -->

# Archive::plot

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::plot() const -> ArchivePlotAccessor
Archive::plot(const pluss::table::AtlasPlotOptions& options) const -> ArchivePlotAccessor
```

## What It Does

The entry point into the archive-level plotting accessor — depth-stack-aware charts
(per-catalog line/scatter/etc. via `frames()`, 3D surface/wireframe/point-cloud views
across depth via `surface()`/`wireframe()`/`cloud3()`, and depth-axis-specific chart types),
returning `pluss::pl::Studio` figures (or a `CatalogPlotAccessor` for a single selected
catalog via `frames(index)`). `ArchivePlotAccessor` is the kindle-layer counterpart to
[`AtlasPlotAccessor`](../../atlas_plot_accessor_api_reference) — every method delegates
straight through to it, tabelizing the relevant catalogs into `DataFrame`s first, reusing
its rendering logic exactly rather than duplicating it. Every method on the returned
accessor has its own page, with real rendered charts, under
[`ArchivePlotAccessor` API Reference](../../archive_plot_accessor_api_reference).

```{important} Reuses AtlasPlotOptions directly — there is no separate ArchivePlotOptions
There is no `ArchivePlotOptions` type — this accessor reuses `pluss::table::AtlasPlotOptions`
as-is, the same scribble of per-chart-type option structs (`figure`, `frames`, `depth_axis`,
`surface`, `wireframe`, `cloud3`, `depth`, `depth_compare`, `depth_facet`, `depth_rolling`,
`triplot`) [`Atlas::plot`](../atlas/plot.md) takes.
```

## Parameters

**options** (`const pluss::table::AtlasPlotOptions&`, optional)
    A scribble of per-chart-type option structs — each field configures the corresponding
    method on the returned accessor. See the important callout above.

## Returns

`ArchivePlotAccessor`
    A binder for the current archive — call one of its methods to build a specific chart.

## Examples

Example 1. `plot()` binds without rendering; `frames(index)` narrows to a single catalog's
own `CatalogPlotAccessor` for the familiar 2D chart methods.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto catalog_plot = archive.plot().frames(0);
```

## Notes

- `plot()` alone does no work — it only binds the archive; call one of the returned
  accessor's methods to actually build a chart.
- Unlike a strict `Atlas`, the `Archive` this accessor wraps doesn't require every catalog
  to share the same row count — see
  [`ArchivePlotAccessor::frames`](../archive-plot/frames.md)'s important callout on ragged
  catalogs.
- Every method except [`triplot()`](../archive-plot/triplot.md) respects
  `options.depth_axis`, selecting and ordering which catalogs participate —
  `triplot()` always pools every catalog's records together instead.

## See Also

- [`ArchivePlotAccessor` API Reference](../../archive_plot_accessor_api_reference): every
  chart method on the accessor this call returns, each with a real rendered example.
- [`Atlas::plot`](../atlas/plot.md): the frame-stack (`DataFrame`) counterpart this
  accessor delegates through.
- [`Catalog::plot`](../catalog/plot.md): the single-catalog delegate `frames(index)`
  narrows down to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrecords.html">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
