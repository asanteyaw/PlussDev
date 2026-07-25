<!-- pluss-hand-authored -->

# DataFrame::plot3d

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::plot3d() const -> DataFramePlot3DAccessor
DataFrame::plot3d(const DataFramePlotOptions& options) const -> DataFramePlot3DAccessor
```

## What It Does

Enters the 3D charting accessor bound to the current frame — 3D scatter, 3D line, a
3D (bivariate) histogram, and aggregated surface/wireframe meshes, for genuinely
three-axis data. Like [`plot()`](plot.md), `plot3d()` itself draws nothing: it binds
the frame (and optional shared chart-wide `options`) and hands back an accessor whose
methods each return a `pluss::pl::Studio` figure — the same `PlussPl` figure type
`plot()`'s 2D accessor returns, documented in `PlussPl`'s own reference.

Every one of the accessor's methods has its own dedicated, example-rich reference
page — start at the
[DataFramePlot3DAccessor API Reference](../../dataframe_plot3d_accessor_api_reference)
and use its left sidebar to jump straight to any method (`scatter`, `line`, `hist`,
`surface`, `wireframe`).

## Parameters

**options** (`const DataFramePlotOptions&`, optional)

Chart-wide defaults shared across every 3D chart drawn from the returned accessor —
the same options struct [`plot()`](plot.md) accepts. Omit it to use the accessor's
built-in defaults.

## Returns

`DataFramePlot3DAccessor`
    An accessor bound to the current frame (and, if given, `options`). Its methods
    (`.scatter()`, `.line()`, `.hist()`, `.surface()`, `.wireframe()`) each build and
    return a `pluss::pl::Studio` figure.

## Examples

Example 1. Enter the 3D plotting accessor and draw a 3D scatter chart.

```cpp
using namespace pluss::table;

auto points = DataFrame({
    {"x", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"y", Thread(pluss::make_series({4.0, 3.0, 2.0}))},
    {"z", Thread(pluss::make_series({2.0, 5.0, 4.0}))},
});

auto chart = points.plot3d().scatter("x", "y", "z");
chart.save_png("points_3d.png");
```

`points.plot3d()` returns a `DataFramePlot3DAccessor` bound to `points`;
`.scatter("x", "y", "z")` plots the three numeric features against each other in
three dimensions and returns a `pluss::pl::Studio`, saved here with `.save_png(...)`.

Example 2. Share chart-wide options (a wider figure) across a 3D chart.

```cpp
DataFramePlotOptions options;
options.width = 900.0;

auto chart = points.plot3d(options).scatter("x", "y", "z");
```

Example 3. Draw an aggregated surface mesh instead — see
[`surface()`](../dataframe-plot3d/surface.md) for the full grid-construction rules.

```cpp
auto readings = DataFrame({
    {"depth_m", Thread(pluss::make_series({10.0, 10.0, 20.0, 20.0}))},
    {"hour",    Thread(pluss::make_series({0.0, 12.0, 0.0, 12.0}))},
    {"temp_c",  Thread(pluss::make_series({18.0, 20.0, 15.0, 16.5}))},
});

auto surface_chart = readings.plot3d().surface("hour", "depth_m", "temp_c");
surface_chart.save_svg("temp_surface.svg");
```

## Notes

- `plot3d()` covers only genuinely three-dimensional chart kinds (3D scatter, 3D
  line, 3D histogram, surface, wireframe); use [`plot()`](plot.md) for every 2D
  chart kind instead.
- Every chart method returns a `pluss::pl::Studio`; that type's own methods are
  documented in `PlussPl`'s reference, not here.
- Every axis feature is converted to a numeric value before plotting: a `Series`
  feature is used as-is, a `Timeline`/`TimeIndex` feature is converted to epoch
  nanoseconds, and a `Note` feature is cast to numbers (text that doesn't parse as a
  number becomes `NaN` rather than throwing outright). See each method's own page
  for the exact conversion rule and throw text.
- `surface()` and `wireframe()` build their grid via `DataFrame::pivot_table()`
  internally, which only supports 7 of `DataFrame::GroupOp`'s 10 aggregation values
  (`Std`/`Var`/`Mode` throw) — see [`surface()`](../dataframe-plot3d/surface.md)'s
  important callout.

## See Also

- [`DataFrame::plot`](plot.md): the 2D counterpart to this accessor.
- [`DataFrame::matrix`](matrix.md): the raw numeric matrix behind a frame, for
  lower-level access outside the plotting accessors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot.html">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="pow.html">
    <span class="label">Next</span>
    <span class="title">pow</span>
  </a>
</div>
