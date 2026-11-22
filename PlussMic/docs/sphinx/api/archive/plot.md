<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4bafcda5abfd -->

# Archive::plot

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
ArchivePlotAccessor Archive::plot() const
ArchivePlotAccessor Archive::plot(const KeySelection& keys) const
ArchivePlotAccessor Archive::plot(const ArchivePlotOptions& options) const
```

## What It Does

The entry point into the archive-level plotting accessor — depth-stack-aware charts
(per-catalog line/scatter/etc. via `frames()`, 3D surface/wireframe/point-cloud views
across depth via `surface()`/`wireframe()`/`cloud3()`, and depth-axis-specific chart types),
returning `pluss::pl::Studio` figures (or a `CatalogPlotAccessor` for a single selected
catalog via `frames(index)`). `ArchivePlotAccessor` is the kindle-layer counterpart to
[`AtlasPlotAccessor`](../../atlas_plot_accessor_api_reference), speaking of keys and catalogs
instead of features and frames. Every method on the returned
accessor has its own page, with real rendered charts, under
[`ArchivePlotAccessor` API Reference](../../archive_plot_accessor_api_reference).

## Parameters

**keys** (`const KeySelection&`)
    Which keys every key-selecting plot of the accessor (`depth_compare`, `depth_facet`, `triplot`) draws when a
    call names none: `all_keys`, `include_keys` or `exclude_keys`. A plot's own `keys` overrides it, and keys named in
    the call override both. See [Selecting keys](../archive-plot/depth_compare.md#selecting-keys).

**options** (`const ArchivePlotOptions&`)
    The settings each plot of the accessor starts from.

| Field | Type |
|---|---|
| `figure` | `pluss::table::DataFramePlotOptions` |
| `frames` | `pluss::table::AtlasFramesPlotOptions` |
| `depth_axis` | `pluss::table::AtlasDepthAxisOptions` |
| `surface` | `pluss::table::Surface3DPlotOptions` |
| `wireframe` | `pluss::table::Wireframe3DPlotOptions` |
| `cloud3` | `pluss::table::Scatter3DPlotOptions` |
| `depth` | `pluss::table::AtlasDepthPlotOptions` |
| `depth_compare` | `ArchiveDepthComparePlotOptions` |
| `depth_facet` | `ArchiveDepthFacetPlotOptions` |
| `depth_rolling` | `pluss::table::AtlasDepthRollingPlotOptions` |
| `triplot` | `pluss::pl::flash::TriplotOptions` |
| `keys` | `KeySelection` |

Passing `all_features`, `include_features` or `exclude_features` in `figure` throws an error that names the key
knobs to use instead.

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

Example 2. An accessor-level key selection, then a whole `ArchivePlotOptions`, and the error for a frame-style
selection. The archive is the one of [`depth_compare()`](../archive-plot/depth_compare.md#selecting-keys).

```cpp
const auto narrowed = archive.plot(pluss::kindle::KeySelection{.exclude_keys = {"pressure"}});
auto compare = narrowed.depth_compare();
auto facet = narrowed.depth_facet();
std::cout << compare.panes().front().trace_count() << " " << facet.panes().size() << "\n";

pluss::kindle::ArchivePlotOptions options;
options.keys.include_keys = {"humidity"};
options.depth_compare.statistic = pluss::table::AtlasDepthStatistic::Median;
std::cout << archive.plot(options).depth_compare().panes().front().trace_count() << "\n";

pluss::kindle::ArchivePlotOptions bad;
bad.figure.include_features = {"humidity"};
try { archive.plot(bad); } catch (const pluss::Error& e) { std::cout << e.message() << "\n"; }
```

```text
2 2
1
Archive::plot: an Archive selects keys, not features. Use all_keys, include_keys or exclude_keys (options.keys) instead of all_features, include_features or exclude_features.
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
