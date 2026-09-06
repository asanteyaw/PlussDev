<!-- pluss-hand-authored -->

# Series::float_power

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::float_power(const Series& other) const
Series Series::float_power(double other) const
Series& Series::float_power_(const Series& other)
```

## What It Does

Raises each element to a power, **always in floating point**:

```
float_power(a, b) = a ** b
```

The one difference from [`pow()`](pow.md) is dtype. `pow()` follows the usual
type-promotion rules — two integer operands give an integer result.
`float_power()` promotes both operands to `float64` first (or `complex128` if
either side is complex) and returns that. Use it when you want a real-valued
power regardless of how the inputs are typed — fractional exponents,
`2 ** -1`, or just guarding against a surprise integer truncation.

## Parameters

**other** (`const Series&` or `double`): the exponent `b`. A `Series`
broadcasts against the receiver; a `double` applies the same exponent
everywhere.

## Returns

`Series` or `Series&`
    `a ** b` as `float64` (or `complex128` for complex inputs). Because an
    in-place call cannot change dtype, `float_power_()` requires a `Float64`
    or `Complex128` receiver and a Series exponent.

## Gradient

`float_power()` is differentiable, with the same rules as [`pow()`](pow.md)
evaluated in double precision:

```
d/da (a ** b) = b * a ** (b - 1)
d/db (a ** b) = a ** b * ln(a)
```

## Examples

Example 1. Integer operands still produce a `float64` result.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2, 3, 4}).float_power(pluss::make_series({3, 2, 0})));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[8.0000, 9.0000, 1.0000]
```

The same call through [`pow()`](pow.md) would return an integer series.

Example 2. Fractional and large exponents.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({4.0, 9.0, 2.0})
        .float_power(pluss::make_series({0.5, 0.5, 10.0})));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0000, 3.0000, 1024.0000]
```

Example 3. A scalar exponent.

```cpp
pluss::series::print(std::cout, pluss::make_series({2, 3}).float_power(2.0));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4.0000, 9.0000]
```

## Gotchas And Failure Modes

- The result dtype is always `float64` (or `complex128`), never `float32` and
  never integer — even when both inputs are `float32`. Cast down with
  [`to()`](to.md) if you need `float32`.
- A negative base with a non-integer exponent gives `NaN` on the real path,
  just as with [`pow()`](pow.md).
- For integer-preserving exponentiation, use [`pow()`](pow.md) instead.
- `float_power_()` cannot promote an integer or `Float32` receiver. Use the
  out-of-place result when promotion is required.

## Summary

`float_power(a, b)` is `a ** b` forced into floating point:

- both operands promote to `float64` (`complex128` if complex); result matches
- differentiable, with the [`pow()`](pow.md) rules in double precision
- `float_power_()` is the in-place form

## See Also

- [`Series::pow`](pow.md): exponentiation with ordinary type promotion.
- [`Series::exp2`](exp2.md) / [`Series::ldexp`](ldexp.md): the base-2 special
  cases.
- [`Series::sqrt`](sqrt.md): `float_power(x, 0.5)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flipud.html">
    <span class="label">Previous</span>
    <span class="title">flipud</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
