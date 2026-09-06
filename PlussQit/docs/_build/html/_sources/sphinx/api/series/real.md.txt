<!-- pluss-hand-authored -->

# Series::real

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::real() const
```

## What It Does

Returns the real part of each element.

For a complex input the result is a real-valued `Series` of the same shape:
`Complex64` yields `float32`, `Complex128` yields `float64`. For an input that
is already real, `real()` returns the values unchanged (a plain copy), so it is
safe to call in dtype-agnostic code.

`real()` pairs with [`imag()`](imag.md): together they split a complex series
into its two real components, and [`pluss::series::complex`](../series_factory/complex.md)
recombines them.

## Reading The Result

`real()` changes the **number representation**, not the arrangement of the
data. A `[rows, columns]` complex series produces a `[rows, columns]` real
series with the same element positions. It does not flatten, reduce, compute a
magnitude, or discard rows whose imaginary part is zero.

This distinction matters when a complex calculation is followed by ordinary
data work. Extracting `real()` gives a standalone real series that can be
filtered, aggregated, or exported without carrying a complex dtype. The source
is not modified, and the result owns independent storage.

## Parameters

None.

## Returns

`Series`
    The real component. Shape is preserved. The dtype is the real counterpart
    of a complex input (`Complex64 -> float32`, `Complex128 -> float64`), or the
    input dtype unchanged when the input is already real.

## Gradient

`real()` is differentiable. For a real input it is the identity, so the
sensitivity passes straight through. For a complex input the backward lifts the
real cotangent back into the complex plane with a zero imaginary part (the
Wirtinger convention), so a reverse pass through `z.real()` accumulates
`grad + 0j` into `z`.

## Examples

Example 1. The real part of a complex series. Here the series is assembled from
its two real components with
[`pluss::series::complex`](../series_factory/complex.md), so `real()` simply
hands back the first of them.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({3.0f, -1.0f, 0.0f, 0.0f}),   // real parts
    pluss::make_series({4.0f,  0.0f, 2.0f, 0.0f}));  // imaginary parts

pluss::series::print(std::cout, z.real()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[3.0000, -1.0000, 0.0000, 0.0000]
```

The same series could be written directly from `std::complex` literals —
`pluss::make_series({std::complex<float>{3, 4}, {-1, 0}, {0, 2}, {0, 0}})` —
and either form is idiomatic.

Example 2. On a real input `real()` returns the values unchanged.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, 0.0, 2.5}).real()
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-3.0000, 0.0000, 2.5000]
```

Example 3. On a real leaf the backward is the identity.

```cpp
auto a = pluss::make_series({2.0, -1.0});
a.tracks_slope_(true);
a.real().sum().reverse();

pluss::series::print(std::cout, a.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 1.0000]
```

Example 4. Shape is preserved for matrices. Each output cell is the first
Cartesian coordinate of the complex value in the corresponding input cell.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
pluss::series::print(std::cout, z.real()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=
⎡  3.0000 -1.0000 ⎤
⎣  0.0000  0.0000 ⎦
```

Example 5. `real()` and [`imag()`](imag.md) form a lossless Cartesian
round-trip when they are recombined at the matching complex precision.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
auto rebuilt = pluss::series::complex(z.real(), z.imag());
pluss::series::print(std::cout, rebuilt) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=
⎡ (3.0000+4.0000j) (-1.0000+0.0000j) ⎤
⎣ (0.0000+2.0000j)  (0.0000+0.0000j) ⎦
```

Example 6. On a tracked complex series, summing the real component lifts each
unit output sensitivity back as `1 + 0j`. This shows that the imaginary input
coordinate does not influence a real-component objective.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
z.tracks_slope_(true);
z.real().sum().reverse();
pluss::series::print(std::cout, z.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=
⎡ (1.0000+0.0000j) (1.0000+0.0000j) ⎤
⎣ (1.0000+0.0000j) (1.0000+0.0000j) ⎦
```

## Gotchas And Failure Modes

- The result dtype changes for a complex input: `Complex64` gives `float32`,
  not `float64`. Convert with [`to()`](to.md) if you need a specific precision.
- `real()` on a real series is a copy, not a view; mutating the result does not
  touch the source.
- `real()` is not [`abs()`](abs.md). For `3 + 4j`, the real component is `3`,
  while the magnitude is `5`.
- To rebuild a complex series from parts, use
  [`pluss::series::complex`](../series_factory/complex.md), not arithmetic on
  `real()` / `imag()`.

## Summary

`real()` is the real component of each element:

- complex in, real out (`Complex64 -> float32`, `Complex128 -> float64`)
- identity on a real input
- differentiable; the backward carries a zero imaginary part on the complex
  path
- the counterpart of [`imag()`](imag.md)

## See Also

- [`Series::imag`](imag.md): the imaginary component.
- [`Series::conj`](conj.md): the complex conjugate, `real - imag * i`.
- [`Series::angle`](angle.md): the phase.
- [`pluss::series::complex`](../series_factory/complex.md): build a complex
  series from real and imaginary parts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ravel.html">
    <span class="label">Previous</span>
    <span class="title">ravel</span>
  </a>
  <a class="next-link" href="reciprocal.html">
    <span class="label">Next</span>
    <span class="title">reciprocal</span>
  </a>
</div>
