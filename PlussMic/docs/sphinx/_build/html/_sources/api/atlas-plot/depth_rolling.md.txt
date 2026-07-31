<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d50e582d84ac -->

# AtlasPlotAccessor::depth_rolling

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::depth_rolling(std::string_view feature) const -> pluss::pl::Studio
AtlasPlotAccessor::depth_rolling(std::string_view feature, const AtlasDepthRollingPlotOptions& options) const -> pluss::pl::Studio
```

## What It Does

Reduces `feature` to one scalar per frame (statistic `options.statistic`, default
`Mean` — the same per-frame reduction [`depth()`](depth.md) uses), then applies a
simple trailing moving average (`options.window`, default `3`) over that reduced
series, and plots both the raw reduced series (gray, optional) and the smoothed
series (teal) against depth.

```{important} Returns a Studio, not printable output
Like every method on `AtlasPlotAccessor`, this builds and returns a `pluss::pl::Studio` —
call `.show()`/`.save_svg(path)`/etc. to view or persist it.
```

## Parameters

**feature** (`std::string_view`)
    The numeric feature to reduce, then smooth, one value per frame.

**options** (`const AtlasDepthRollingPlotOptions&`)
    `statistic` (default `Mean`) is the per-frame reduction, same set as
    [`depth()`](depth.md)'s `AtlasDepthStatistic`; `window` (default `3`) is the
    rolling average's window size; `show_raw` (default `true`) toggles the
    unsmoothed series; `show_grid` (default `true`); `raw_line_width`/
    `rolling_line_width` control the two lines' thickness.

## Returns

`pluss::pl::Studio`
    A figure with one 2D pane containing the raw reduced series (if `show_raw`) and
    its rolling-average smoothed counterpart.

## Examples

Example 1. Plot a feature's per-frame mean alongside its default 3-wide rolling
average, across a 5-frame atlas.

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

auto chart = atlas.plot().depth_rolling("temp_c");
chart.save_svg("depth_rolling_temp.svg");
```

The raw (gray) series is `temp_c`'s per-frame mean — the same straight `20, 21, 22,
23, 24` line [`depth()`](depth.md)'s Example 1 plots — with the rolling (teal) series
overlaid alongside it.

![Output of Example 1](img/depth_rolling-1.svg)

## Notes

- The reduction happens first (per frame, `options.statistic`), and the rolling
  average is computed over that already-reduced series — not over each frame's raw
  case-level values.
- Set `options.show_raw = false` to draw only the smoothed line.
- `options.depth_axis` (shared with [`depth()`](depth.md)/[`surface()`](surface.md)/
  etc.) controls which frames are included and in what order, before reduction.

## See Also

- [`AtlasPlotAccessor::depth`](depth.md): the unsmoothed per-frame reduction this
  method builds on.
- [`AtlasPlotAccessor::depth_compare`](depth_compare.md): several features' reduced
  series overlaid, without any smoothing.
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth_facet.html">
    <span class="label">Previous</span>
    <span class="title">depth_facet</span>
  </a>
  <a class="next-link" href="frames.html">
    <span class="label">Next</span>
    <span class="title">frames</span>
  </a>
</div>
