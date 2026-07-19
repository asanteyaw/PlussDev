<!-- pluss-hand-authored -->

# DataFrame::plot3d

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::plot3d() const -> DataFramePlot3DAccessor
DataFrame::plot3d(const DataFramePlotOptions& options) const -> DataFramePlot3DAccessor
```

## What It Does

Enters the 3D charting accessor bound to the current frame — 3D scatter and 3D line
charts, for genuinely three-axis data. Like [`plot()`](plot.md), `plot3d()` itself
draws nothing: it binds the frame (and optional shared chart-wide `options`) and
hands back an accessor whose methods each return a `pluss::pl::Studio` figure — the
same `PlussPl` figure type `plot()`'s 2D accessor returns, documented in `PlussPl`'s
own reference.

## Parameters

**options** (`const DataFramePlotOptions&`, optional)

Chart-wide defaults shared across every 3D chart drawn from the returned accessor —
the same options struct [`plot()`](plot.md) accepts. Omit it to use the accessor's
built-in defaults.

## Returns

`DataFramePlot3DAccessor`
    An accessor bound to the current frame (and, if given, `options`). Its methods
    (`.scatter()`, `.line()`) each build and return a `pluss::pl::Studio` figure.

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

## Notes

- `plot3d()` covers only genuinely three-dimensional chart kinds (3D scatter, 3D
  line); use [`plot()`](plot.md) for every 2D chart kind instead.
- Every chart method returns a `pluss::pl::Studio`; that type's own methods are
  documented in `PlussPl`'s reference, not here.
- Every feature named in a 3D chart call must be numeric — passing a `Note` feature
  as one of the three axes is not a supported combination.

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
