<!-- pluss-hand-authored -->

# Series::exp2

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::exp2() const
Series& Series::exp2_()
Series& Series::exp2_() const
```

## What It Does

`exp2()` computes `2^x` elementwise — the base-2 exponential.

It is the inverse of [`log2()`](log2.md), and the base-2 sibling of
[`exp()`](exp.md). Use it when your exponents are counts of bits or octaves:
converting a log2-scaled value back to linear, expanding a floating-point
exponent field, or reconstructing a magnitude from a base-2 logarithm.

Anchor values: `exp2(0) = 1`, `exp2(1) = 2`, `exp2(-1) = 0.5`, `exp2(10) =
1024`.

An integer input is promoted to a floating-point result.

## Parameters

None.

## Returns

`Series` or `Series&`
    `2^x` elementwise. Large positive `x` overflows to `+inf`; large negative
    `x` underflows to `0`. `exp2_()` overwrites the current storage.

## Gradient

`exp2()` is differentiable. Writing `y = exp2(x)`:

```
d/dx 2^x = 2^x * ln 2 = y * ln 2
```

So the backward multiplier is the output value scaled by `ln 2` (about
`0.6931`). As with [`exp()`](exp.md), the same rapid growth that affects the
forward value also affects the gradient.

## In-Place Form

`exp2_()` is the in-place sibling of [`exp2()`](exp2.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `exp2_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 1.0, 3.0});

auto& ref = x.exp2_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 8]
same object: true
```

## Examples

Example 1. Powers of two.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 1.0, 3.0}).exp2());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5000, 1.0000, 2.0000, 8.0000]
```

Example 2. The backward multiplier is `y * ln 2`.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0});
x.tracks_slope_(true);

x.exp2().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.69314718, 1.3862944, 2.7725887]
```

The values are `1 * ln 2`, `2 * ln 2`, and `4 * ln 2`.

## Gotchas And Failure Modes

- `exp2()` overflows to `+inf` for large positive `x` (past about `1024` in
  `float64`) and underflows to `0` for large negative `x`.
- The result is floating point even for an integer input.
- `NaN` propagates.

## Summary

`exp2(x)` is `2^x`:

- inverse of [`log2()`](log2.md); base-2 form of [`exp()`](exp.md)
- promotes integer input to floating point
- differentiable, backward multiplier `2^x * ln 2`

## See Also

- [`Series::log2`](log2.md): the inverse.
- [`Series::exp`](exp.md): the natural-base exponential.
- [`Series::pow`](pow.md): a general base raised to a series exponent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="expand.html">
    <span class="label">Next</span>
    <span class="title">expand</span>
  </a>
</div>
