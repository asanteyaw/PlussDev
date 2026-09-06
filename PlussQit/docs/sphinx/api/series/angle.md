<!-- pluss-hand-authored -->

# Series::angle

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::angle() const
```

## What It Does

Returns the phase of each element, in radians, in the range `(-pi, pi]`.

For a complex input it is `atan2(imag, real)` — the angle the number makes with
the positive real axis. For a real input it follows the same `atan2(0, x)`
convention: `0` for a positive value, `pi` for a negative value, and `0` at
zero.

Together with [`abs()`](abs.md), `angle()` gives the polar form of a complex
series, and [`pluss::series::polar`](../series_factory/polar.md) rebuilds a
complex series from magnitude and angle.

## From Coordinates To Direction

A complex value `x + yi` contains two independent kinds of information:
`abs()` measures how far it is from the origin, while `angle()` measures its
direction. Scaling by a positive real number changes the magnitude but leaves
the angle unchanged. Multiplying by a unit complex value rotates the angle.

The result is expressed in **radians**. The branch cut lies on the negative
real axis: directions just above it approach `+pi`, while directions just
below it approach `-pi`. Those directions are geometrically close even though
their numeric angles differ by nearly `2*pi`; use a phase-unwrapping operation
before taking differences across that cut.

## Parameters

None.

## Returns

`Series`
    The phase in radians. Shape is preserved; the dtype is `float32` for a
    `Complex64` / real `float32` input and `float64` for a `Complex128` / real
    `float64` input.

## Gradient

`angle()` is differentiable.

- On a **complex** input the backward follows the standard rule
  `d(angle)/dz = i * conj(z) / |z|^2` (Wirtinger form), so a reverse pass
  produces a non-zero complex sensitivity.
- On a **real** input the phase is piecewise constant (`0` or `pi`), so its
  local slope is `0` everywhere it is defined. A reverse pass therefore returns
  a zero sensitivity rather than raising — the real and complex paths agree.

At the origin the phase is defined as `0` and the gradient is not
mathematically meaningful; the backward returns `0` there too.

## Examples

Example 1. The phase of a complex series.

```cpp
auto z = pluss::make_series(
    {std::complex<float>{3, 4}, {-1, 0}, {0, 2}, {0, 0}});

pluss::series::print(std::cout, z.angle()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.9273, 3.1416, 1.5708, 0.0000]
```

`atan2(4, 3) = 0.9273`, `atan2(0, -1) = pi`, `atan2(2, 0) = pi/2`,
`atan2(0, 0) = 0`.

Example 2. On a real input, `angle()` is `0` for positive values and `pi` for
negative ones.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, -3.0, 0.0}).angle()
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 3.1416, 0.0000]
```

Example 3. The gradient on a real leaf is zero — the reverse pass succeeds and
fills the sensitivity with `0`.

```cpp
auto a = pluss::make_series({2.0, -1.0});
a.tracks_slope_(true);
a.angle().sum().reverse();

pluss::series::print(std::cout, a.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.0000, -0.0000]
```

Example 4. Axis-aligned values make the quadrants explicit. The fifth value,
`3 + 4j`, lies in the first quadrant; the origin is assigned phase zero.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({1.0f, 0.0f, -1.0f, 0.0f, 3.0f, 0.0f}),
    pluss::make_series({0.0f, 1.0f,  0.0f, -1.0f, 4.0f, 0.0f}));
pluss::series::print(std::cout, z.angle()) << "\n";
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 1.5708, 3.1416, -1.5708, 0.9273, 0.0000]
```

Example 5. The complex sensitivity is easiest to understand in Cartesian
coordinates. For `z = x + yi`,

```
d angle / dx = -y / (x^2 + y^2)
d angle / dy =  x / (x^2 + y^2)
```

At `3 + 4j`, these are `-4/25` and `3/25`, represented together as
`-0.16 + 0.12j`. The origin receives zero because no direction is defined
there.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({3.0f, 0.0f}),
    pluss::make_series({4.0f, 0.0f}));
z.tracks_slope_(true);
z.angle().sum().reverse();
pluss::series::print(std::cout, z.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=complex64, platform=cpu, numel=2, contiguous=true)
data=[(-0.1600+0.1200j), (0.0000+0.0000j)]
```

## Gotchas And Failure Modes

- The range is `(-pi, pi]`. `angle()` of a negative real is `pi`, not `-pi`.
- A raw subtraction of wrapped phases is unsafe across the negative-real branch
  cut; values near `+pi` and `-pi` can represent nearby directions.
- Result dtype tracks the input precision (`Complex64 -> float32`).
- Integral and low-precision real inputs produce `float32` radians; `float64`
  and `Complex128` produce `float64`.
- On a real input the gradient is `0`, not an error — do not treat a zero
  sensitivity here as a broken graph.
- For the polar decomposition pair with [`abs()`](abs.md) for the magnitude;
  rebuild with [`pluss::series::polar`](../series_factory/polar.md).

## Summary

`angle()` is the phase of each element in radians:

- `atan2(imag, real)` for complex, `atan2(0, x)` for real; range `(-pi, pi]`
- differentiable: non-zero complex sensitivity on the complex path, exact zero
  on the real path (no raise)
- pairs with [`abs()`](abs.md) for polar form
- preserves shape and returns radians, with a branch cut on the negative real
  axis

## See Also

- [`Series::abs`](abs.md): the magnitude — the other half of polar form.
- [`Series::real`](real.md) / [`Series::imag`](imag.md): the Cartesian
  components `angle()` is built from.
- [`pluss::series::polar`](../series_factory/polar.md) /
  [`pluss::series::complex`](../series_factory/complex.md): build a complex
  series from magnitude/angle or real/imaginary parts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aminmax.html">
    <span class="label">Previous</span>
    <span class="title">aminmax</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
