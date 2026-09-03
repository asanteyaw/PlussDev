<!-- pluss-hand-authored -->

# Series::atan2

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::atan2(const Series& other) const -> Series
Series& Series::atan2_(const Series& other)
```

## What It Does

`atan2()` computes the angle, in radians, of the point `(x, y)` measured from
the positive x-axis, where `y` is the receiver and `x` is `other`.

Call it `a.atan2(b)` and read it as `atan2(y = a, x = b)`.

The plain [`atan()`](atan.md) of `y / x` only knows the *ratio*, so it always
returns an angle in `(-pi/2, pi/2)` and cannot tell `(1, 1)` from `(-1, -1)`.
`atan2()` takes the two coordinates separately, so it knows which quadrant the
point is in and returns a full-range angle in `(-pi, pi]`:

| point `(x, y)` | quadrant | `atan2(y, x)` |
| --- | --- | --- |
| `(1, 1)` | I | `pi/4` |
| `(-1, 1)` | II | `3pi/4` |
| `(-1, -1)` | III | `-3pi/4` |
| `(1, -1)` | IV | `-pi/4` |

Combined with [`hypot()`](hypot.md), which gives the radius, `atan2()` is the
angular half of converting `(x, y)` to polar form.

The convention at the origin is `atan2(0, 0) = 0`.

## Parameters

**other** (`const Series&`)

The x-coordinate `x`. Broadcasts against the receiver `y`.

## Returns

`Series` or `Series&`
    The elementwise angle in radians, in `(-pi, pi]`. `atan2_()` writes the
    result back into the receiver.

## Gradient

`atan2()` is differentiable. With `r2 = x^2 + y^2`:

```
d/dy atan2(y, x) =  x / r2
d/dx atan2(y, x) = -y / r2
```

That gradient is the same as the tangential unit vector scaled by `1 / r`: a
small change in the point perpendicular to the radius changes the angle, a
change along the radius does not. The derivative blows up as `(x, y)`
approaches the origin, where the angle is genuinely ill-defined.

## Examples

Example 1. One point per quadrant, plus the two axis directions.

```cpp
auto y = pluss::make_series({1.0, 1.0, -1.0, -1.0, 0.0, 0.0});
auto x = pluss::make_series({1.0, -1.0, -1.0, 1.0, 1.0, -1.0});

pluss::series::print(std::cout, y.atan2(x));
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.7854, 2.3562, -2.3562, -0.7854, 0.0000, 3.1416]
```

The first four are `+/-pi/4` and `+/-3pi/4`. The last two are the angle of the
positive x-axis (`0`) and the negative x-axis (`pi`).

Example 2. The origin returns `0` by convention rather than `NaN`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0}).atan2(pluss::make_series({0.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.0000]
```

Example 3. The backward is `(x / r2, -y / r2)`.

```cpp
auto y = pluss::make_series({1.0, 1.0, -1.0});
auto x = pluss::make_series({1.0, -1.0, -1.0});
y.tracks_slope_(true);
x.tracks_slope_(true);

y.atan2(x).sum().reverse();

std::cout << "dy=" << y.sensitivity() << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
```

### Output

```text
dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5, -0.5, -0.5]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5, -0.5, 0.5]
```

Every point here has `r2 = 2`, so the derivatives are `x/2` and `-y/2`.

Example 4. `other` broadcasts. A scalar x-coordinate is applied to every `y`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, -1.0}).atan2(pluss::make_series(1.0)));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.7854, -0.7854]
```

## Gotchas And Failure Modes

- Argument order is `y.atan2(x)`, not `x.atan2(y)`. Getting it backward
  reflects the angle across the line `y = x`.
- `atan2()` is not `atan()` of a ratio. Use `atan2()` whenever the sign of each
  coordinate matters, which is almost always.
- The gradient is unbounded near the origin; a tracked `atan2()` on points that
  can reach `(0, 0)` will produce very large or non-finite sensitivities there.
- `NaN` in either coordinate propagates.

## Summary

`atan2(y, x)` is the quadrant-aware angle of `(x, y)`:

- range `(-pi, pi]`; `atan2(0, 0) = 0`
- the angular half of polar form, paired with [`hypot()`](hypot.md)
- differentiable, gradient `(x / r2, -y / r2)` with `r2 = x^2 + y^2`

## See Also

- [`Series::atan`](atan.md): single-argument arctangent, range `(-pi/2, pi/2)`,
  no quadrant information.
- [`Series::hypot`](hypot.md): the radius `sqrt(x^2 + y^2)` to go with this
  angle.
- [`Series::tan`](tan.md): the forward direction, undoing an angle back to a
  slope.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="atan.html">
    <span class="label">Previous</span>
    <span class="title">atan</span>
  </a>
  <a class="next-link" href="atanh.html">
    <span class="label">Next</span>
    <span class="title">atanh</span>
  </a>
</div>
