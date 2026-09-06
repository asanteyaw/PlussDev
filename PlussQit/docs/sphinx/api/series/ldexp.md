<!-- pluss-hand-authored -->

# Series::ldexp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ldexp(const Series& other) const
Series Series::ldexp(double other) const
Series& Series::ldexp_(const Series& other)
```

## What It Does

Scales each element by a power of two:

```
ldexp(x, e) = x * 2^e
```

`x` is the receiver; the argument supplies the exponent `e`. When `e` is an
integer this is an exact binary shift of the exponent field — no rounding of
the significand — which is why `ldexp` is the exact inverse of
[`frexp()`](frexp.md): `mantissa.ldexp(exponent)` rebuilds the original value.
A non-integer `e` is allowed and gives the continuous `x * 2^e`.

The result is floating-point: an integer input is promoted (`Float64` for
integral or `Float64` input, `Float32` for `Float32`).

## Parameters

**other** (`const Series&` or `double`): the exponent `e`. A `Series` broadcasts
against the receiver; a `double` applies the same exponent everywhere.

## Returns

`Series` or `Series&`
    `x * 2^e`, floating-point. `ldexp_()` writes the result back into the
    receiver.

## Gradient

`ldexp()` is differentiable with respect to `x`:

```
d/dx ldexp(x, e) = 2^e
```

For a Series exponent, both operands are differentiable:

```
d/dx ldexp(x, e) = 2^e
d/de ldexp(x, e) = x * 2^e * ln(2)
```

The scalar overload has no Series exponent to receive sensitivity.

## Examples

Example 1. Positive, zero, and negative exponents.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 1.5, 3.0}).ldexp(pluss::make_series({3, 0, -1})));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[8.0000, 1.5000, 1.5000]
```

`1 * 2^3 = 8`, `1.5 * 2^0 = 1.5`, `3 * 2^-1 = 1.5`.

Example 2. A scalar exponent applies to every element.

```cpp
pluss::series::print(std::cout, pluss::make_series({1.0, 2.0, 3.0}).ldexp(2.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4.0000, 8.0000, 12.0000]
```

Example 3. The backward scales the sensitivity by `2^e`.

```cpp
auto x = pluss::make_series({3.0, 5.0});
x.tracks_slope_(true);
x.ldexp(pluss::make_series({1, 2})).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 4]
```

`d/dx (x * 2^1) = 2` and `d/dx (x * 2^2) = 4`.

Example 4. A fractional Series exponent is continuous and receives sensitivity.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto e = pluss::make_series({0.5, -1.0});
x.tracks_slope_(true);
e.tracks_slope_(true);
x.ldexp(e).sum().reverse();
pluss::series::print(std::cout, x.sensitivity()) << '\n';
pluss::series::print(std::cout, e.sensitivity());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.4142, 0.5000]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.9605, 1.0397]
```

## Gotchas And Failure Modes

- The result is always floating-point; an integer `x` is promoted.
- A floating Series exponent carries gradient; an integer exponent cannot
  track slope.
- `ldexp_()` accepts a Series exponent, requires a floating receiver, and
  preserves the receiver's existing storage and shape.
- A very large `e` overflows to `+/-inf` and a very small one underflows to
  `0`, the same as any float multiply.
- `ldexp` is the inverse of [`frexp()`](frexp.md); round-tripping is exact for
  integer exponents.

## Summary

`ldexp(x, e)` is `x * 2^e`:

- exponent may be a `Series` or a scalar; integer exponents are exact shifts
- result is floating-point
- differentiable in `x` and a floating Series exponent
- the inverse of [`frexp()`](frexp.md); `ldexp_()` is the in-place form

## See Also

- [`Series::frexp`](frexp.md): the inverse — split `x` into `(mantissa,
  exponent)`.
- [`Series::exp2`](exp2.md): `2^x`, the continuous power-of-two.
- [`Series::mul`](mul.md): the underlying scale.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lcm.html">
    <span class="label">Previous</span>
    <span class="title">lcm</span>
  </a>
  <a class="next-link" href="le.html">
    <span class="label">Next</span>
    <span class="title">le</span>
  </a>
</div>
