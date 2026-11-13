<!-- pluss-hand-authored -->

# Series::deg2rad

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::deg2rad() const
Series& Series::deg2rad_()
Series& Series::deg2rad_() const
```

## What It Does

`deg2rad()` converts an angle in degrees to radians, elementwise:

```
deg2rad(x) = x * pi / 180
```

It exists as a named method rather than a bare `mul()` so the unit conversion
reads clearly and carries the exact constant. The trigonometric methods
([`sin()`](sin.md), [`cos()`](cos.md), [`tan()`](tan.md)) all expect radians,
so `deg2rad()` is the usual first step when your data is in degrees.

`90` degrees becomes `pi/2`, `180` becomes `pi`, `-45` becomes `-pi/4`.

An integer or boolean input is promoted to a floating-point result, because the
conversion never produces an integer.

## Parameters

None.

## Returns

`Series` or `Series&`
    The angle in radians. `deg2rad_()` overwrites the current storage.

## Gradient

`deg2rad()` is differentiable. It is a scale by a constant, so the derivative is
that constant:

```
d/dx deg2rad(x) = pi / 180 = 0.017453293
```

## In-Place Form

`deg2rad_()` is the in-place sibling of [`deg2rad()`](deg2rad.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `deg2rad_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 90.0, 180.0});
x.deg2rad_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1.5707963, 3.1415927]
```

## Examples

Example 1. The common angles.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, 90.0, 180.0, -45.0}).deg2rad());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.5708, 3.1416, -0.7854]
```

`1.5708` is `pi/2`, `3.1416` is `pi`, `-0.7854` is `-pi/4`.

Example 2. An integer input is promoted rather than truncating the constant.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0, 90, 180}, pluss::ScalarType::Int64).deg2rad());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 1.5708, 3.1416]
```

Example 3. The gradient is the constant `pi / 180`.

```cpp
auto x = pluss::make_series({45.0, 90.0});
x.tracks_slope_(true);

x.deg2rad().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.017453293, 0.017453293]
```

Example 4. Chained with `sin()` to take the sine of an angle given in degrees.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, 30.0, 90.0}).deg2rad().sin());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.5000, 1.0000]
```

## Gotchas And Failure Modes

- The trig methods want radians. Calling `sin()` directly on a degree value is a
  common bug that `deg2rad()` prevents.
- The result is floating point even for an integer input.
- `NaN` propagates.

## Summary

`deg2rad(x)` scales degrees to radians by `pi / 180`:

- promotes integer and boolean input to floating point
- differentiable, constant derivative `pi / 180`

## See Also

- [`Series::rad2deg`](rad2deg.md): the inverse conversion.
- [`Series::sin`](sin.md), [`Series::cos`](cos.md), [`Series::tan`](tan.md): the
  radian-expecting consumers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumsum.html">
    <span class="label">Previous</span>
    <span class="title">cumsum</span>
  </a>
  <a class="next-link" href="detach.html">
    <span class="label">Next</span>
    <span class="title">detach</span>
  </a>
</div>
