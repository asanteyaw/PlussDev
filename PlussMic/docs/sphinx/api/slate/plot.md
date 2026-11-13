<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9b6c7de3c92a -->

# Slate::plot

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
SlatePlotAccessor Slate::plot() const
SlatePlotAccessor Slate::plot(const DataFramePlotOptions& options) const
```

## What It Does

Enters the single-series charting accessor bound to the current `Slate` — line, step,
area, scatter, bar, histogram, box, violin, pie, and a handful of diagnostic plots, one
chart kind per method. `plot()` itself draws nothing; it just binds the slate (and,
optionally, shared chart-wide defaults, the same `DataFramePlotOptions` struct
[`DataFrame::plot`](../dataframe/plot.md) takes) and hands back an accessor whose methods
each return a `pluss::pl::Studio` — the same figure type every 2D and 3D `DataFrame` chart
method returns.

Every method on the returned accessor works by wrapping the bound slate (and, for
two-slate overloads, a second companion slate) into a small one- or two-column
`DataFrame` and delegating straight into the matching method on
[`DataFramePlotAccessor`](../../dataframe_plot_api_reference) — see the
[SlatePlotAccessor API Reference](../../slate_plot_api_reference)'s important callout,
and [`line()`](../slate-plot/line.md)'s page, for the full naming and row-label rules
that wrapping follows.

## Parameters

**options** (`const DataFramePlotOptions&`, optional)

Chart-wide defaults shared across every plot drawn from the returned accessor — tick/label
rotation, figure size, tight layout, and more. `default_x_feature` and `default_hue_feature`
are still accepted (the same struct is reused from the `DataFrame` accessor) but have no
effect here, since every `SlatePlotAccessor` method resolves its own x-axis or hue
explicitly via a second `Slate` argument rather than a named feature string.

## Returns

`SlatePlotAccessor`
    An accessor bound to the current slate (and, if given, `options`). Each of its methods
    (`.line()`, `.scatter()`, `.bar()`, `.hist()`, and so on) builds and returns a
    `pluss::pl::Studio` figure, ready to further customize or save.

## Examples

Example 1. Enter the plotting accessor and draw a line chart with default options.

```cpp
using namespace pluss::table;

auto revenue = Slate(pluss::make_series({100.0, 120.0, 90.0, 150.0}));
revenue = revenue.name("revenue");

auto chart = revenue.plot().line();
chart.save_svg("revenue_line.svg");
```

`revenue.plot()` returns a `SlatePlotAccessor` bound to `revenue`; `.line()` draws
`revenue`'s own values against the default numeric case index (since no companion `x`
slate is given) and returns a `pluss::pl::Studio`, which `.save_svg(...)` then writes to
disk.

Example 2. Share chart-wide options (figure size) across every chart drawn from one
accessor.

```cpp
DataFramePlotOptions options;
options.width = 900.0;
options.height = 500.0;

auto accessor = revenue.plot(options);
auto chart = accessor.line();
chart.save_png("revenue_wide.png");
```

## Notes

- `plot()` never touches or copies the slate's data beyond what the requested chart needs
  to draw — it is a lightweight accessor, not a materialized copy.
- Every chart method returns a `pluss::pl::Studio`; that type's own methods (theming,
  sizing, legends, exporting) are documented in `PlussPl`'s reference, not here.
- `DataFramePlotOptions::default_x_feature`/`default_hue_feature` have no effect on this
  accessor — see the Parameters section above.
- There is no `Slate::plot3d` — 3D charting is only available on
  [`DataFrame::plot3d`](../dataframe/plot3d.md).

## See Also

- [`SlatePlotAccessor API Reference`](../../slate_plot_api_reference): the full list of
  chart methods available from this accessor.
- [`DataFrame::plot`](../dataframe/plot.md): the multi-feature counterpart this accessor's
  methods delegate into internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="percentiles.html">
    <span class="label">Previous</span>
    <span class="title">percentiles</span>
  </a>
  <a class="next-link" href="pow.html">
    <span class="label">Next</span>
    <span class="title">pow</span>
  </a>
</div>
