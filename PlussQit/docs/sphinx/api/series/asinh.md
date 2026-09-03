<!-- pluss-hand-authored -->

# Series::asinh

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::asinh() const -> Series
```

## What It Does

`asinh()` applies the inverse hyperbolic sine elementwise. It recovers `y` from
`x=sinh(y)` and is defined for every finite real input.

## An Unbounded Inverse With Logarithmic Growth

Unlike `asin()`, `asinh()` has no `[-1,1]` restriction. Hyperbolic sine maps the
whole real line onto the whole real line, so its inverse does the same.
`asinh()` is odd, passes through zero, behaves approximately like `x` near
zero, and grows only logarithmically at large magnitude.

Those properties make it useful for compressing signed heavy-tailed data while
preserving sign and retaining more detail around zero than a blunt clipping
operation.

## Parameters

None. Every element is transformed independently and shape is preserved.

## Returns

`Series`
    A new `Series` of inverse-hyperbolic values. Integral, bool, and float8
    inputs promote to floating output rather than truncating fractional values.

## Gradient

```text
d asinh(x) / dx = 1 / sqrt(1 + x^2)
```

The derivative equals one at zero and smoothly decreases toward zero as the
input magnitude grows. It is finite everywhere on the real line.

## Examples

Example 1. Known `sinh()` values recover `-1`, `0`, and `1`.

```cpp
auto values = pluss::make_series({
    -1.1752011936438014,
    0.0,
    1.1752011936438014,
});
std::cout << values.asinh() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 1]
```

Example 2. Integral input promotes instead of losing the fractional result.

```cpp
auto integers = pluss::make_series({-1, 0, 1}, pluss::ScalarType::Int64);
std::cout << integers.asinh() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[-0.88137358, 0, 0.88137358]
```

Example 3. Reverse mode is strongest near zero and weaker away from it.

```cpp
auto x = pluss::make_series({0.0, 0.75});
x.tracks_slope_(true);
x.asinh().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 0.8]
```

## Gotchas And Failure Modes

- `asinh()` and `asin()` are different functions; only `asin()` has a bounded
  real input domain and returns an angle.
- Large magnitudes are compressed, so equal output differences do not represent
  equal input differences.
- The method is out-of-place; `asinh_()` is the mutating sibling.

## Summary

Use `asinh()` for an invertible, sign-preserving compression of unrestricted
real values. It is smooth over the complete real line and promotes integral
input to floating output.

## See Also

- [`Series::sinh`](sinh.md): forward hyperbolic sine.
- [`Series::acosh`](acosh.md): inverse hyperbolic cosine with domain `x>=1`.
- [`Series::atanh`](atanh.md): inverse hyperbolic tangent with domain `|x|<=1`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asin.html"><span class="label">Previous</span><span class="title">asin</span></a>
  <a class="next-link" href="atan.html"><span class="label">Next</span><span class="title">atan</span></a>
</div>
