<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb9ecfb2c9f1 -->

# AtlasPlotAccessor::triplot

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::triplot() const -> pluss::pl::Studio
AtlasPlotAccessor::triplot(const pluss::pl::flash::TriplotOptions& options) const -> pluss::pl::Studio
AtlasPlotAccessor::triplot(pluss::RangeView<std::string_view> features) const -> pluss::pl::Studio
AtlasPlotAccessor::triplot(pluss::RangeView<std::string_view> features, const pluss::pl::flash::TriplotOptions& options) const -> pluss::pl::Studio
```

## What It Does

Selects exactly three numeric features (`features`, if given — otherwise every
numeric feature on the *first* bound frame, which must have at least three), flattens
each selected feature's values **across every frame in the stack** into one long
column per feature, and draws a ternary (triangle) plot from the three pooled
columns.

```{important} Ignores the depth axis entirely — pools every frame's rows together
Unlike every other method on `AtlasPlotAccessor` — [`surface()`](surface.md),
[`depth()`](depth.md), [`cloud3()`](cloud3.md), and the rest all respect
`options_.depth_axis` to select which frames participate — `triplot()` consults no
depth-axis option at all. Every frame in `frames_` contributes its rows, concatenated
together, with no per-frame distinction preserved in the output (no per-frame color
or legend grouping).
```

## Parameters

**options** (`const pluss::pl::flash::TriplotOptions&`)
    Passed straight through to `flash::triplot`. If `options.labels` is left empty,
    it defaults to the three selected feature names.

**features** (`pluss::RangeView<std::string_view>`, default empty)
    Exactly the features to plot — must resolve to (or be explicitly given as)
    exactly three, deduplicated. Left empty, defaults to every numeric feature on
    `frames_.front()`, which must itself have at least three numeric features.

## Returns

`pluss::pl::Studio`
    A figure with a single ternary-plot pane, one point per row pooled across every
    frame.

## Examples

Example 1. Default feature selection: all three numeric features on the first frame,
pooled across a 5-frame, 5-case-per-frame atlas (25 points total).

```cpp
using namespace pluss::table;

std::vector<DataFrame> frames;
for (int i = 0; i < 5; ++i) {
    double base = static_cast<double>(i);
    frames.push_back(DataFrame({
        {"temp_c", Thread(pluss::make_series({18.0 + base, 19.0 + base, 20.0 + base, 21.0 + base, 22.0 + base}))},
        {"humidity", Thread(pluss::make_series({40.0 + base * 2, 42.0 + base * 2, 44.0 + base * 2, 46.0 + base * 2, 48.0 + base * 2}))},
        {"pressure", Thread(pluss::make_series({1010.0 + base, 1011.0 + base, 1012.0 + base, 1013.0 + base, 1014.0 + base}))},
    }));
}
auto atlas = Atlas(frames, Thread(pluss::make_note({"day1", "day2", "day3", "day4", "day5"})));

auto chart = atlas.plot().triplot();
chart.save_svg("triplot_default.svg");
```

`frames_.front()` (frame `0`) has exactly three numeric features — `temp_c`,
`humidity`, `pressure` — so all three are selected automatically. All 5 frames'
5 rows each are pooled into 25 total points on the triangle, per the important
callout above; there is no visual grouping by which frame a point came from.

![Output of Example 1](img/triplot-1.svg)

## Notes

- See the important callout above: this is the one method on `AtlasPlotAccessor`
  that does not respect `options_.depth_axis` — every frame always contributes.
- Requires at least three numeric features to resolve to (whether auto-selected or
  explicitly named); fewer throws.
- `options.labels` defaults to the selected feature names when left empty.

## See Also

- [`AtlasPlotAccessor::cloud3`](cloud3.md): also plots raw per-row points across
  three features, but as a 3D scatter respecting `depth_axis`, with one trace per
  frame rather than one pooled point cloud.
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="surface.html">
    <span class="label">Previous</span>
    <span class="title">surface</span>
  </a>
  <a class="next-link" href="wireframe.html">
    <span class="label">Next</span>
    <span class="title">wireframe</span>
  </a>
</div>
