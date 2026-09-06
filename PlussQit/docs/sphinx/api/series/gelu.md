<!-- pluss-hand-authored -->

# Series::gelu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::gelu() const -> Series
Series::gelu(bool approximate) const -> Series
```

## What It Does

`gelu()` applies the Gaussian Error Linear Unit activation elementwise:

```
gelu(x) = x * Phi(x)
```

where `Phi` is the standard-normal cumulative distribution function. Instead of
the hard on/off gate of [`relu()`](relu.md), each input is scaled by the
probability that a standard-normal draw falls below it — so a large positive `x`
passes through almost unchanged, `x = 0` maps to `0`, and negative `x` is
softly suppressed with a small dip below zero.

`Phi(x)` has no elementary form, so there are two evaluations:

- **`gelu()`** and **`gelu(true)`** use the fast `tanh` approximation
  `x * 0.5 * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))`.
- **`gelu(false)`** uses the exact form built on [`erf()`](erf.md),
  `x * 0.5 * (1 + erf(x / sqrt(2)))`.

The two agree to about three decimal places everywhere.

`gelu()` needs a floating-point input (it is built on `tanh` / `erf`).

## Parameters

**approximate** (`bool`, optional)

`true` (the default) selects the `tanh` approximation; `false` selects the exact
`erf`-based form.

## Returns

`Series` or `Series&`
    The activated values, same shape and floating-point dtype as the input.

## Gradient

`gelu()` is differentiable. Its derivative is

```
d/dx gelu(x) = Phi(x) + x * phi(x)
```

where `phi` is the normal density. It equals `0.5` at `x = 0`, rises above `1`
for moderate positive `x` (the `x * phi(x)` bump), and tends to `1` for large
positive `x` and `0` for large negative `x`. Unlike `relu()`, the gradient is
smooth and nonzero for small negative inputs, so `gelu()` units do not
"die".

## Examples

Example 1. The soft gate: near-identity on the right, a small negative dip on
the left.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).gelu());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.0036, -0.1588, 0.0000, 0.8412, 2.9964]
```

Example 2. The exact `erf`-based form differs only in the last digits.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).gelu(false));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.0040, -0.1587, 0.0000, 0.8413, 2.9960]
```

Example 3. The backward is `Phi(x) + x * phi(x)` — `0.5` at the origin and
above `1` for moderate positive `x`.

```cpp
auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
x.tracks_slope_(true);

x.gelu().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.0116, -0.0830, 0.5000, 1.0830, 1.0116]
```

The negative entries at `x = -3` and `x = -1` are the smooth non-zero gradient
that keeps the unit trainable where `relu()` would give exactly `0`.

## Gotchas And Failure Modes

- The default is the `tanh` approximation. Pass `gelu(false)` if you need the
  exact CDF form.
- Floating-point input only; an integer series raises (the error names the
  internal `tanh`).
- The gradient can slightly exceed `1` for moderate positive `x`; that is the
  `x * phi(x)` term, not a bug.

## Summary

`gelu(x)` is `x * Phi(x)` — a smooth, probabilistic gate:

- `gelu()` / `gelu(true)` use the `tanh` approximation; `gelu(false)` the exact
  `erf` form
- floating-point input only
- differentiable, smooth and nonzero for small negatives, so units do not die

## See Also

- [`Series::silu`](silu.md): `x * sigmoid(x)`, the closely related smooth gate.
- [`Series::relu`](relu.md): the hard-gate baseline `gelu()` softens.
- [`Series::erf`](erf.md): the primitive behind the exact form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ger.html">
    <span class="label">Previous</span>
    <span class="title">ger</span>
  </a>
  <a class="next-link" href="grouped_count.html">
    <span class="label">Next</span>
    <span class="title">grouped_count</span>
  </a>
</div>
