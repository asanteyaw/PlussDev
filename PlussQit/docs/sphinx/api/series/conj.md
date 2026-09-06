<!-- pluss-hand-authored -->

# Series::conj

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::conj() const
```

## What It Does

Returns the complex conjugate of each element: `real - imag * i`.

For a complex input the dtype is preserved (`Complex64 -> Complex64`,
`Complex128 -> Complex128`) and only the sign of the imaginary part flips. For
a real input `conj()` is the identity and returns a copy, so it is safe in
dtype-agnostic code.

Conjugation is the operation behind a Hermitian transpose, the modulus
(`z * conj(z) = |z|^2`), and the inner product on a complex vector space.

## Reflection Across The Real Axis

Writing a value as `z = x + yi` makes the operation concrete:

```
conj(z) = x - yi
```

The real coordinate and magnitude stay fixed; only the direction is reflected.
Consequently `angle(conj(z))` is `-angle(z)` away from the branch cut, and two
applications restore the original value. Conjugation is elementwise. It does
not transpose a matrix; use [`adjoint()`](adjoint.md) when both conjugation and
swapping the final two dimensions are required.

## Parameters

None.

## Returns

`Series`
    The conjugate, same shape and same dtype as the input. A real input yields
    an unchanged copy.

## Gradient

`conj()` is differentiable. Its backward is itself: a cotangent `g` flowing
back through `z.conj()` accumulates `conj(g)` into `z`. This holds on every
backend and for both `Complex64` and `Complex128`; on a real input it is the
plain identity.

## Examples

Example 1. Conjugating a complex series flips the imaginary sign.

```cpp
auto z = pluss::make_series(
    {std::complex<float>{3, 4}, {-1, 0}, {0, 2}, {0, 0}});

pluss::series::print(std::cout, z.conj()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=[(3.0000-4.0000j), (-1.0000-0.0000j), (0.0000-2.0000j), (0.0000-0.0000j)]
```

Example 2. On a real input `conj()` returns the values unchanged.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, 0.0, 2.5}).conj()
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-3.0000, 0.0000, 2.5000]
```

Example 3. Conjugation is its own inverse: `conj(conj(z)) == z`. This time the
series is built from separate real and imaginary parts with
[`pluss::series::complex`](../series_factory/complex.md).

```cpp
auto z = pluss::series::complex(
    pluss::make_series({3.0f, -1.0f, 0.0f, 0.0f}),
    pluss::make_series({4.0f,  0.0f, 2.0f, 0.0f}));

pluss::series::print(std::cout, z.conj().conj()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=[(3.0000+4.0000j), (-1.0000+0.0000j), (0.0000+2.0000j), (0.0000+0.0000j)]
```

Example 4. Multiplying by the conjugate removes phase and leaves squared
magnitude. The two independently written expressions agree elementwise.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
pluss::series::print(std::cout, z.mul(z.conj()).real()) << "\n";
pluss::series::print(std::cout, z.abs().square()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=
⎡ 25.0000 1.0000 ⎤
⎣  4.0000 0.0000 ⎦
Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=
⎡ 25.0000 1.0000 ⎤
⎣  4.0000 0.0000 ⎦
```

This identity is often preferable to manually squaring and adding the two
components because it states the complex-number intent directly.

Example 5. Reverse propagation is conjugated too. The imaginary component of
`conj(z)` is `-imag(z)`, so summing that component contributes `0 - 1j` to
every source element.

```cpp
auto z = pluss::series::complex(
    pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
    pluss::make_series({{4.0f,  0.0f}, {2.0f, 0.0f}}));
z.tracks_slope_(true);
z.conj().imag().sum().reverse();
pluss::series::print(std::cout, z.sensitivity()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=
⎡ (0.0000-1.0000j) (0.0000-1.0000j) ⎤
⎣ (0.0000-1.0000j) (0.0000-1.0000j) ⎦
```

## Gotchas And Failure Modes

- `conj()` keeps the complex dtype; it does not return real parts. Use
  [`real()`](real.md) for that.
- `conj()` is elementwise and leaves matrix dimensions in place. It is not a
  Hermitian transpose by itself.
- On a real input it is a copy, not a view.
- The result is materialized as an ordinary series. Mutating it does not alter
  the source.

## Summary

`conj()` is the complex conjugate `real - imag * i`:

- dtype-preserving on complex, identity on real
- self-inverse: `conj(conj(z)) == z`
- differentiable on every backend, with backward `conj(g)`

## See Also

- [`Series::real`](real.md) / [`Series::imag`](imag.md): the components; `conj`
  negates the second.
- [`Series::abs`](abs.md): `|z| = sqrt((z * conj(z)).real())`.
- [`Series::angle`](angle.md): `conj` negates the phase.
- [`Series::adjoint`](adjoint.md): conjugate and transpose the final two
  dimensions as one matrix operation.
- [`pluss::series::complex`](../series_factory/complex.md): build a complex
  series from real and imaginary parts (or pass `std::complex` values straight
  to [`pluss::make_series`](../series_factory/make.md)).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="comp.html">
    <span class="label">Previous</span>
    <span class="title">comp</span>
  </a>
  <a class="next-link" href="contiguous.html">
    <span class="label">Next</span>
    <span class="title">contiguous</span>
  </a>
</div>
