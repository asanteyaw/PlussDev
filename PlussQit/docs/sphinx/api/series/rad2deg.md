<!-- pluss-hand-authored -->

# Series::rad2deg

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::rad2deg() const -> Series
Series& Series::rad2deg_()
```

## What It Does

`rad2deg()` converts an angle in radians to degrees, elementwise:

```
rad2deg(x) = x * 180 / pi
```

It is the inverse of [`deg2rad()`](deg2rad.md), and the usual last step when a
computation works in radians (as the inverse trig methods
[`asin()`](asin.md) / [`acos()`](acos.md) / [`atan()`](atan.md) and
[`atan2()`](atan2.md) do) but the result should be reported in degrees.

`pi/2` becomes `90`, `pi` becomes `180`.

An integer or boolean input is promoted to a floating-point result.

## Parameters

None.

## Returns

`Series` or `Series&`
    The angle in degrees. `rad2deg_()` overwrites the current storage.

## Gradient

`rad2deg()` is differentiable. It is a scale by a constant:

```
d/dx rad2deg(x) = 180 / pi = 57.29578
```

## Examples

Example 1. The common angles, in radians in and degrees out.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, 1.5707963267948966, 3.141592653589793}).rad2deg());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 90.0000, 180.0000]
```

Example 2. An integer number of radians is promoted, not scaled by a truncated
constant.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({57, 114}, pluss::ScalarType::Int64).rad2deg());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3265.8594, 6531.7189]
```

Each result is the exact `x * 180 / pi`.

Example 3. The gradient is the constant `180 / pi`.

```cpp
auto x = pluss::make_series({1.0, 2.0});
x.tracks_slope_(true);

x.rad2deg().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[57.29578, 57.29578]
```

Example 4. Reporting an `atan2()` angle in degrees.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 1.0}).atan2(pluss::make_series({1.0, -1.0})).rad2deg());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[45.0000, 135.0000]
```

## Gotchas And Failure Modes

- The result is floating point even for an integer input.
- `NaN` propagates.
- `rad2deg()` and [`deg2rad()`](deg2rad.md) are exact inverses only up to
  floating-point rounding.

## Summary

`rad2deg(x)` scales radians to degrees by `180 / pi`:

- inverse of [`deg2rad()`](deg2rad.md)
- promotes integer and boolean input to floating point
- differentiable, constant derivative `180 / pi`

## See Also

- [`Series::deg2rad`](deg2rad.md): the inverse conversion.
- [`Series::atan2`](atan2.md), [`Series::asin`](asin.md),
  [`Series::acos`](acos.md): the radian-producing sources.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quantile.html">
    <span class="label">Previous</span>
    <span class="title">quantile</span>
  </a>
  <a class="next-link" href="ravel.html">
    <span class="label">Next</span>
    <span class="title">ravel</span>
  </a>
</div>
