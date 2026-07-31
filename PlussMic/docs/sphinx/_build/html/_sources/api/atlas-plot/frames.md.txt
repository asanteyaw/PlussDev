<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c562af9f8ee0 -->

# AtlasPlotAccessor::frames

[Back to AtlasPlotAccessor API Reference](../../atlas_plot_accessor_api_reference)

## Signatures

```cpp
AtlasPlotAccessor::frames(std::int64_t frame_index = 0) const -> DataFramePlotAccessor
AtlasPlotAccessor::frames(pluss::RangeView<std::int64_t> frame_indices, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const -> pluss::pl::Studio
AtlasPlotAccessor::frames(pluss::RangeView<std::int64_t> frame_indices, std::string_view y_feature, std::optional<std::string_view> x_feature, const AtlasFramesPlotOptions& options) const -> pluss::pl::Studio
```

## What It Does

Three different things depending on the overload:

- **`frames(frame_index)`**: not a chart method at all — returns the plain
  [`DataFramePlotAccessor`](../../dataframe_plot_api_reference) for a single selected
  frame (`DataFrame::plot(options_.figure)`), the same accessor you'd get from
  calling `.plot()` directly on that one frame. Supports negative indexing (`-1` is
  the last frame). This is how you reach the *entire* single-frame 2D plotting
  surface (`line`, `scatter`, `bar`, `hist`, and so on) for one frame out of the
  stack.
- **`frames(frame_indices, y_feature, x_feature)`**: draws `y_feature` from each
  selected frame as its own line trace, all overlaid on one 2D pane (the default
  `AtlasFrameSelectionMode::Overlay`). `x_feature` resolves each frame's x-axis; left
  as `std::nullopt`, each frame's own row/case-label thread is used instead.
- **`frames(frame_indices, y_feature, x_feature, options)`**: the same overlay, or —
  with `options.mode = AtlasFrameSelectionMode::Facet` — one subplot per selected
  frame instead of one overlaid pane.

```{important} Only the multi-frame overloads return a Studio
`frames(frame_index)` returns a `DataFramePlotAccessor`, not a `Studio` — chain a
further method like `.line(...)`/`.scatter(...)` onto it to get a chart. The two
multi-frame overloads return a `pluss::pl::Studio` directly, same as every other
method on `AtlasPlotAccessor` — call `.show()`/`.save_svg(path)`/etc. to view it;
`std::cout << chart` does not compile for either return type.
```

## Parameters

**frame_index** (`std::int64_t`, default `0`, single-frame overload)
    The frame position to select. Negative values count from the end (`-1` is the
    last frame). Throws if the atlas has no frames or the index is out of bounds.

**frame_indices** (`pluss::RangeView<std::int64_t>`, multi-frame overloads)
    The frame positions to include, deduplicated and normalized (supports negative
    indices per-entry, same as `frame_index`).

**y_feature** (`std::string_view`, multi-frame overloads)
    The numeric feature plotted on the y-axis, pulled from each selected frame.
    Throws if missing from any selected frame.

**x_feature** (`std::optional<std::string_view>`, default `std::nullopt`,
multi-frame overloads)
    The feature supplying the x-axis. Left as `std::nullopt`, each frame's own
    row/case-label thread is used instead.

**options** (`const AtlasFramesPlotOptions&`, three-argument multi-frame overload)
    `mode` (`Overlay` or `Facet`, default `Overlay`); `rows`/`cols` (default `0`,
    auto-sized) and `grid_gap_x`/`grid_gap_y` for `Facet` layout; `show_grid`
    (default `true`); `line_width` (default `1.5F`).

## Returns

`DataFramePlotAccessor`
    (Single-frame overload) The full 2D plotting accessor for one selected frame.

`pluss::pl::Studio`
    (Multi-frame overloads) A figure with either one overlaid pane (`Overlay`) or one
    subplot per selected frame (`Facet`).

## Examples

Example 1. Overlay `temp_c` from three selected frames (`0`, `2`, `4`) of a 5-frame
atlas onto one pane.

```cpp
using namespace pluss::table;

std::vector<DataFrame> frames_vec;
for (int i = 0; i < 5; ++i) {
    double base = static_cast<double>(i);
    frames_vec.push_back(DataFrame({
        {"temp_c", Thread(pluss::make_series({18.0 + base, 19.0 + base, 20.0 + base, 21.0 + base, 22.0 + base}))},
    }));
}
auto atlas = Atlas(frames_vec, Thread(pluss::make_note({"day1", "day2", "day3", "day4", "day5"})));

auto chart = atlas.plot().frames({0, 2, 4}, "temp_c");
chart.save_svg("frames_overlay.svg");
```

Three lines appear on one pane — "day1", "day3", and "day5"'s `temp_c` series — each
its own color, legend-labeled by depth label, with `x_feature` left as `std::nullopt`
so each frame's own default row index (`0`-`4`) supplies the x-axis.

![Output of Example 1](img/frames-1.svg)

Example 2. The same three frames, faceted into separate subplots instead of
overlaid.

```cpp
AtlasFramesPlotOptions options;
options.mode = AtlasFrameSelectionMode::Facet;
options.rows = 1;
options.cols = 3;

auto chart = atlas.plot().frames({0, 2, 4}, "temp_c", std::nullopt, options);
chart.save_svg("frames_facet.svg");
```

Three separate subplots appear side by side, one per selected frame, each titled
with that frame's depth label.

![Output of Example 2](img/frames-2.svg)

## Notes

- `frames(frame_index)` is the entry point into the entire single-frame 2D plotting
  surface for one frame — see [`Atlas::frame`](../atlas/frame.md)/
  [`Atlas::dloc`](../atlas/dloc.md) for the non-plotting equivalent of selecting a
  single frame.
- The multi-frame overloads never aggregate across frames — each selected frame
  contributes its own independent trace or subplot.
- `Overlay` mode colors and legend-labels each frame's trace by its depth position
  (or `depth_labels()`, if set); `Facet` mode titles each subplot the same way.

## See Also

- [`DataFramePlotAccessor`](../../dataframe_plot_api_reference): the full single-frame
  plotting surface `frames(frame_index)` returns.
- [`AtlasPlotAccessor::depth_facet`](depth_facet.md): also builds a facet grid, but
  faceting by *feature* (after reducing each frame) rather than by frame.
- [`Atlas::plot`](../atlas/plot.md): how to enter this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth_rolling.html">
    <span class="label">Previous</span>
    <span class="title">depth_rolling</span>
  </a>
  <a class="next-link" href="surface.html">
    <span class="label">Next</span>
    <span class="title">surface</span>
  </a>
</div>
