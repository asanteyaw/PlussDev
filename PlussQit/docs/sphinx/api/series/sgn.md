<!-- pluss-hand-authored -->

# Series::sgn

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sgn() const
```

## What It Does

Returns the sign of each element, generalized to complex numbers.

- For a **real** input `sgn()` is `-1`, `0`, or `+1` — identical to
  [`sign()`](sign.md).
- For a **complex** input `sgn()` is the unit-modulus projection `z / |z|`, the
  point on the unit circle in the direction of `z`, and `0` at `z = 0`.

The complex case is why `sgn()` exists as a separate name:
[`sign()`](sign.md) has no ordering to fall back on for complex numbers and
rejects them, while `sgn()` extends cleanly.

## Magnitude Versus Direction

For every non-zero complex value, `sgn()` discards magnitude and preserves
direction:

```
z = abs(z) * sgn(z)
abs(sgn(z)) = 1
angle(sgn(z)) = angle(z)
```

Multiplying `z` by any positive scale therefore leaves `sgn(z)` unchanged.
This makes the method useful when only orientation matters, such as normalizing
phasors or separating amplitude from phase before aggregation. Unlike dividing
by `abs()` manually, `sgn()` defines the zero case explicitly and returns zero
there instead of producing `0/0`.

## Parameters

None.

## Returns

`Series`
    Same shape and dtype as the input. Real in, real out (`-1` / `0` / `+1`);
    complex in, complex out on the unit circle (or `0`).

## Gradient

- The **real** path is a step function — piecewise constant with a jump at `0`.
  It carries no useful gradient, so an objective that reaches its input only
  through `sgn()` of a real series is disconnected and a reverse pass raises
  (the same contract as [`sign()`](sign.md)).
- The **complex** path, `z / |z|`, is a smooth composition of
  [`abs()`](abs.md) and division away from the origin, so it is differentiable
  where `z != 0`.

## Examples

Example 1. On a real input `sgn()` matches [`sign()`](sign.md).

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, 0.0, 2.5}).sgn()
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.0000, 0.0000, 1.0000]
```

Example 2. On a complex input `sgn()` is `z / |z|`, with `0` mapped to `0`.

```cpp
auto z = pluss::make_series(
    {std::complex<float>{3, 4}, {-1, 0}, {0, 2}, {0, 0}});

pluss::series::print(std::cout, z.sgn()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=[(0.6000+0.8000j), (-1.0000+0.0000j), (0.0000+1.0000j), (0.0000+0.0000j)]
```

`(3 + 4j) / 5 = 0.6 + 0.8j`; `(0 + 2j) / 2 = 1j`; `0` stays `0`.

Example 3. Every non-zero result has modulus `1`. Here `z` is assembled from
its parts with [`pluss::series::complex`](../series_factory/complex.md).

```cpp
auto z = pluss::series::complex(
    pluss::make_series({3.0f, -1.0f, 0.0f, 0.0f}),
    pluss::make_series({4.0f,  0.0f, 2.0f, 0.0f}));

pluss::series::print(std::cout, z.sgn().abs()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 0.0000]
```

Example 4. [`sign()`](sign.md) rejects complex input; `sgn()` is the
generalization to reach for.

```cpp
auto z = pluss::make_series(
    {std::complex<float>{3, 4}, {-1, 0}, {0, 2}, {0, 0}});

try {
  std::cout << z.sign();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
sign: not supported for complex dtypes; use sgn()
```

Example 5. The complex path remains differentiable away from zero. Here the
objective keeps only the real coordinate of the unit direction. At `3 + 4j`,
the resulting source sensitivity is `0.128 - 0.096j`.

```cpp
auto z = pluss::series::complex(pluss::make_series({3.0f}),
                                pluss::make_series({4.0f}));
z.tracks_slope_(true);
z.sgn().real().sum().reverse();
pluss::series::print(std::cout, z.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=complex64, platform=cpu, numel=1, contiguous=true)
data=[(0.1280-0.0960j)]
```

The derivative depends on direction because normalization couples the real and
imaginary coordinates. It is not the identity gradient.

## Gotchas And Failure Modes

- On a real input `sgn()` and [`sign()`](sign.md) are the same function; the
  difference only shows up for complex dtypes.
- `sgn(0)` is `0` in both the real and complex cases — it is not projected to a
  point on the unit circle.
- `sgn()` preserves shape and dtype. It normalizes each element independently;
  it does not normalize a vector or matrix as a whole.
- The real path is non-differentiable (a reverse pass through it alone raises);
  the complex path is differentiable away from the origin.

## Summary

`sgn()` is the sign, generalized to the complex plane:

- real: `-1` / `0` / `+1`, exactly like [`sign()`](sign.md)
- complex: `z / |z|` (unit modulus), `0` at the origin
- real path non-differentiable; complex path differentiable where `z != 0`

## See Also

- [`Series::sign`](sign.md): the real-only sign; raises on complex input.
- [`Series::abs`](abs.md): the magnitude that `sgn()` divides out.
- [`Series::angle`](angle.md): the phase — `sgn(z)` is `polar(1, angle(z))`.
- [`pluss::series::complex`](../series_factory/complex.md) /
  [`pluss::series::polar`](../series_factory/polar.md): build a complex series.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="setxor_along.html">
    <span class="label">Previous</span>
    <span class="title">setxor_along</span>
  </a>
  <a class="next-link" href="sigmoid.html">
    <span class="label">Next</span>
    <span class="title">sigmoid</span>
  </a>
</div>
