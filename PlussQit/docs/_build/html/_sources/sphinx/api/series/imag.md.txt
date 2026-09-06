<!-- pluss-hand-authored -->

# Series::imag

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::imag() const
```

## What It Does

Returns the imaginary part of each element.

For a complex input the result is a real-valued `Series` of the same shape:
`Complex64` yields `float32`, `Complex128` yields `float64`. For an input that
is already real, `imag()` returns a zero series of the matching real dtype and
shape — the imaginary part of a real number is `0`.

`imag()` is the partner of [`real()`](real.md);
[`pluss::series::complex`](../series_factory/complex.md) recombines the two.

## Reading The Result

`imag()` extracts the second Cartesian coordinate without changing shape. A
matrix remains a matrix and a scalar remains rank zero. The returned values are
real numbers; the `i` unit is not retained in the result. If an actual complex
term `i * imag(z)` is needed, combine a zero real component and `z.imag()` with
[`pluss::series::complex`](../series_factory/complex.md).

For an already-real input, the imaginary component is identically zero. Those
zeros preserve the input dtype, including integral dtypes, because no
floating-point calculation is required.

## Parameters

None.

## Returns

`Series`
    The imaginary component (all zeros for a real input). Shape is preserved;
    `Complex64` produces `float32` and `Complex128` produces `float64`. An
    already-real input keeps its dtype.

## Gradient

`imag()` is differentiable for a complex input: the backward lifts the real
cotangent into the complex plane as a pure imaginary part (`0 + grad * i`),
following the Wirtinger convention. For a real input the output is a constant
zero, so it carries no gradient — an objective that reaches its input only
through `imag()` of a real series is disconnected.

## Examples

Example 1. The imaginary part of a complex series. Building the series with
[`pluss::series::complex`](../series_factory/complex.md) makes the round trip
obvious — `imag()` returns exactly the second series that went in.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({3.0f, -1.0f, 0.0f, 0.0f}),   // real parts
    pluss::make_series({4.0f,  0.0f, 2.0f, 0.0f}));  // imaginary parts

pluss::series::print(std::cout, z.imag()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[4.0000, 0.0000, 2.0000, 0.0000]
```

Example 2. On a real input the imaginary part is zero everywhere.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, 0.0, 2.5}).imag()
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.0000, 0.0000]
```

Example 3. Integral input keeps its dtype. This makes the component operation
predictable in generic code even when the current batch contains no complex
values.

```cpp
pluss::series::print(std::cout, pluss::make_series({-3, 0, 2}).imag()) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. A matrix keeps its layout while the imaginary coordinate is
extracted cell by cell.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
pluss::series::print(std::cout, z.imag()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=
⎡ 4.0000 0.0000 ⎤
⎣ 2.0000 0.0000 ⎦
```

Example 5. The complex reverse rule lifts a real cotangent into the imaginary
coordinate. Summing `imag()` therefore contributes `0 + 1j` to every source
element.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
z.tracks_slope_(true);
z.imag().sum().reverse();
pluss::series::print(std::cout, z.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=
⎡ (0.0000+1.0000j) (0.0000+1.0000j) ⎤
⎣ (0.0000+1.0000j) (0.0000+1.0000j) ⎦
```

## Gotchas And Failure Modes

- On a real input `imag()` returns a fresh zero series, not the input — it is
  never a view.
- Result dtype tracks the input precision: `Complex64 -> float32`.
- For already-real input, the zero result preserves the original dtype.
- `imag()` of a real leaf is a constant; do not expect a gradient through it.

## Summary

`imag()` is the imaginary component of each element:

- complex in, paired real precision out; `0` everywhere with dtype preserved
  for an already-real input
- differentiable on the complex path (backward is a pure imaginary lift); no
  gradient on the real path
- the counterpart of [`real()`](real.md)

## See Also

- [`Series::real`](real.md): the real component.
- [`Series::conj`](conj.md): flips the sign of this component.
- [`Series::angle`](angle.md): the phase, `atan2(imag, real)`.
- [`pluss::series::complex`](../series_factory/complex.md): build a complex
  series from real and imaginary parts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="igammac.html">
    <span class="label">Previous</span>
    <span class="title">igammac</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
