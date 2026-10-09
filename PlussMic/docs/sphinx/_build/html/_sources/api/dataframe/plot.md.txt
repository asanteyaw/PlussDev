<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 117e06756190 -->

# DataFrame::plot

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::plot() const -> DataFramePlotAccessor
DataFrame::plot(const DataFramePlotOptions& options) const -> DataFramePlotAccessor
```

## What It Does

Enters the 2D charting accessor bound to the current frame — line, step, area,
scatter, bar, histogram, box, violin, heatmap, and more, one chart kind per method.
`plot()` itself draws nothing; it just binds the frame (and, optionally, shared
chart-wide defaults like a default x-feature or hue-feature) and hands back an
accessor whose methods each return a `pluss::pl::Studio` — a chart/figure object from
the `PlussPl` plotting library, which owns its own reference documentation (rendering
backends, themes, saving to PNG/SVG, and so on). This page previews entry into the
accessor; it does not re-document `Studio` or every chart kind.

Every one of the accessor's 23 chart methods has its own dedicated, example-rich
reference page — start at the
[DataFramePlotAccessor API Reference](../../dataframe_plot_api_reference) and use its
left sidebar to jump straight to any method (`line`, `bar`, `scatter`, `hist`, `box`,
`heatmap`, `auto_plot`, and more).

## Parameters

**options** (`const DataFramePlotOptions&`, optional)

Chart-wide defaults shared across every plot drawn from the returned accessor —
a default x-feature, a default hue-feature, tick/label rotation, figure size, tight
layout, and more. Omit it to use the accessor's built-in defaults.

## Returns

`DataFramePlotAccessor`
    An accessor bound to the current frame (and, if given, `options`). Each of its
    methods (`.line()`, `.scatter()`, `.bar()`, `.hist()`, and so on) builds and
    returns a `pluss::pl::Studio` figure, ready to further customize or save.

## Examples

Example 1. Enter the plotting accessor and draw a line chart with default options.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"month",  Slate(pluss::make_series({1, 2, 3, 4}))},
    {"amount", Slate(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto chart = sales.plot().line("amount", "month");
chart.save_png("monthly_sales.png");
```

`sales.plot()` returns a `DataFramePlotAccessor` bound to `sales`; `.line("amount",
"month")` draws `amount` against `month` and returns a `pluss::pl::Studio`, which
`.save_png(...)` then writes to disk. No table is printed by any of this — the
result is a rendered figure, not a `DataFrame`.

Example 2. Share chart-wide options (a default x-feature) across every chart drawn
from one accessor.

```cpp
DataFramePlotOptions options;
options.default_x_feature = "month";

auto accessor = sales.plot(options);
auto chart = accessor.line("amount");  // uses "month" as x automatically
```

Passing `x_feature` explicitly (as in Example 1) always takes precedence; the
`default_x_feature` in `options` only fills in when a chart method's own
`x_feature` argument is omitted.

## Notes

- `plot()` never touches or copies the frame's data beyond what the requested chart
  needs to draw — it is a lightweight accessor, not a materialized copy.
- Every chart method returns a `pluss::pl::Studio`; that type's own methods (theming,
  sizing, legends, exporting) are documented in `PlussPl`'s reference, not here.
- Use [`plot3d()`](plot3d.md) instead for genuinely three-dimensional charts (3D
  scatter, 3D line); `plot()` covers only 2D chart kinds.

## See Also

- [`DataFrame::plot3d`](plot3d.md): the 3D counterpart to this accessor.
- [`DataFrame::sql`](sql.md), [`DataFrame::ts`](ts.md): other dedicated accessors
  entered the same way, previewed on their own pages.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="place.html">
    <span class="label">Previous</span>
    <span class="title">place</span>
  </a>
  <a class="next-link" href="plot3d.html">
    <span class="label">Next</span>
    <span class="title">plot3d</span>
  </a>
</div>
