<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 25519d9a1155 -->

# AtlasPlotAccessor::depth_facet

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::depth_facet(pluss::RangeView<std::string_view> features = {}) const -> pluss::pl::Studio
AtlasPlotAccessor::depth_facet(pluss::RangeView<std::string_view> features, const AtlasDepthFacetPlotOptions& options) const -> pluss::pl::Studio
```

## What It Does

The same per-frame reduction as [`depth_compare()`](depth_compare.md) — each of
`features` reduced to one scalar per frame via `options.statistic` (default `Mean`)
— but instead of overlaying every feature's line on one pane, draws one subplot per
feature, each titled with that feature's name, x-axis labeled `"depth"`, y-axis
labeled with the feature name.

```{important} Returns a Studio, not printable output
Like every method on `AtlasPlotAccessor`, this builds and returns a `pluss::pl::Studio` —
call `.show()`/`.save_svg(path)`/etc. to view or persist it.
```

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default empty)
    The features to facet, one subplot per feature. Empty selects every numeric
    feature on the bound frames, subject to `options.max_features`.

**options** (`const AtlasDepthFacetPlotOptions&`)
    `statistic` (default `Mean`); `show_grid` (default `true`); `rows`/`cols`
    (default `0`, meaning auto-computed) size the subplot grid; `grid_gap_x`/
    `grid_gap_y` (default `16.0`) space the subplots; `max_features` (default `0`,
    unlimited) caps auto-resolved features when `features` is left empty.

## Returns

`pluss::pl::Studio`
    A figure with one subplot per compared feature, each showing that feature's
    per-frame reduced line.

## Examples

Example 1. Facet two features' per-frame means across a 5-frame atlas into separate
subplots.

```cpp
using namespace pluss::table;

std::vector<DataFrame> frames;
for (int i = 0; i < 5; ++i) {
    double base = static_cast<double>(i);
    frames.push_back(DataFrame({
        {"temp_c", Thread(pluss::make_series({18.0 + base, 19.0 + base, 20.0 + base, 21.0 + base, 22.0 + base}))},
        {"humidity", Thread(pluss::make_series({40.0 + base * 2, 42.0 + base * 2, 44.0 + base * 2, 46.0 + base * 2, 48.0 + base * 2}))},
    }));
}
auto atlas = Atlas(frames, Thread(pluss::make_note({"day1", "day2", "day3", "day4", "day5"})));

auto chart = atlas.plot().depth_facet({"temp_c", "humidity"});
chart.save_svg("depth_facet_temp_humidity.svg");
```

Two subplots appear side by side: `temp_c`'s own titled pane (mean line `20, 21, 22,
23, 24`) and `humidity`'s own titled pane (mean line `44, 46, 48, 50, 52`) — the same
underlying values as [`depth_compare()`](depth_compare.md)'s overlay, just split
apart.

![Output of Example 1](img/depth_facet-1.svg)

## Notes

- Each feature's reduction is computed independently, frame by frame.
- With `rows`/`cols` left at their default `0`, the subplot grid is auto-sized to fit
  the number of features.
- `options.depth_axis` (shared with [`depth()`](depth.md)/[`surface()`](surface.md)/
  etc.) controls which frames are included and in what order.

## See Also

- [`AtlasPlotAccessor::depth_compare`](depth_compare.md): the same per-frame
  reductions, overlaid on one pane instead of faceted into subplots.
- [`AtlasPlotAccessor::frames`](frames.md) (multi-frame overload): also builds a
  facet grid via `AtlasFramesPlotOptions`, but faceting by frame rather than by
  feature.
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth_compare.html">
    <span class="label">Previous</span>
    <span class="title">depth_compare</span>
  </a>
  <a class="next-link" href="depth_rolling.html">
    <span class="label">Next</span>
    <span class="title">depth_rolling</span>
  </a>
</div>
