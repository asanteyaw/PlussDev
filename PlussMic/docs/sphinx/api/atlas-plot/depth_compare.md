<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72c3fbb015ee -->

# AtlasPlotAccessor::depth_compare

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::depth_compare(pluss::RangeView<std::string_view> features = {}) const -> pluss::pl::Studio
AtlasPlotAccessor::depth_compare(pluss::RangeView<std::string_view> features, const AtlasDepthComparePlotOptions& options) const -> pluss::pl::Studio
```

## What It Does

Reduces each of `features` to one scalar per frame (statistic `options.statistic`,
default `Mean`) and overlays every feature's resulting depth-indexed line on a single
2D pane — the multi-feature counterpart of [`depth()`](depth.md), without a band.
When `features` is left empty, every numeric feature (up to `options.max_features`,
if set) is included.

```{important} Returns a Studio, not printable output
Like every method on `AtlasPlotAccessor`, this builds and returns a `pluss::pl::Studio` —
call `.show()`/`.save_svg(path)`/etc. to view or persist it.
```

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default empty)
    The features to compare. Empty selects every numeric feature on the bound
    frames, subject to `options.max_features`.

**options** (`const AtlasDepthComparePlotOptions&`)
    `statistic` (default `Mean`); `show_grid` (default `true`); `line_width`
    (default `1.5F`); `max_features` (default `0`, meaning unlimited) caps how many
    auto-resolved features are included when `features` is left empty.

## Returns

`pluss::pl::Studio`
    A figure with one 2D pane containing one line per compared feature.

## Examples

Example 1. Compare two features' per-frame means across a 5-frame atlas.

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

auto chart = atlas.plot().depth_compare({"temp_c", "humidity"});
chart.save_svg("depth_compare_temp_humidity.svg");
```

`temp_c`'s mean line runs `20, 21, 22, 23, 24` (slope `+1` per frame); `humidity`'s
runs `44, 46, 48, 50, 52` (slope `+2` per frame, since `humidity` was constructed
with a steeper per-frame increment) — both plotted on the same `"dayN"` x-axis.

![Output of Example 1](img/depth_compare-1.svg)

## Notes

- Each feature's reduction is computed independently, frame by frame — never mixing
  values across frames or across features.
- `options.depth_axis` (shared with [`depth()`](depth.md)/[`surface()`](surface.md)/
  etc.) controls which frames are included and in what order.

## See Also

- [`AtlasPlotAccessor::depth`](depth.md): a single feature's reduced line, with an
  optional min/max band this method doesn't draw.
- [`AtlasPlotAccessor::depth_facet`](depth_facet.md): the same multi-feature
  comparison, drawn as separate subplots instead of one overlaid pane.
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth.html">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="depth_facet.html">
    <span class="label">Next</span>
    <span class="title">depth_facet</span>
  </a>
</div>
