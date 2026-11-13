<!-- pluss-hand-authored -->

# Series::silu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::silu() const
Series& Series::silu_()
Series& Series::silu_() const
```

## What It Does

`silu()` applies the Sigmoid Linear Unit (also called *swish*) elementwise:

```
silu(x) = x * sigmoid(x)
```

Like [`gelu()`](gelu.md) it is a smooth, self-gating activation: each input is
scaled by its own [`sigmoid()`](sigmoid.md), so large positive `x` passes
through nearly unchanged, `x = 0` maps to `0`, and negative `x` is softly
suppressed.

It is **not monotonic**. For `x` slightly negative the product `x * sigmoid(x)`
dips to a minimum of about `-0.278` near `x = -1.278` before rising back toward
`0`. That small non-monotonic region is a deliberate feature — it lets the
activation represent a mild negative response rather than a hard cutoff.

`silu()` needs a floating-point input.

## Parameters

None.

## Returns

`Series` or `Series&`
    The activated values, same shape and floating-point dtype as the input.
    `silu_()` overwrites the current storage.

## Gradient

`silu()` is differentiable. With `s = sigmoid(x)`:

```
d/dx silu(x) = s + x * s * (1 - s) = s * (1 + x * (1 - s))
```

It equals `0.5` at `x = 0`, goes slightly negative in the same region where the
forward value dips, and tends to `1` for large positive `x` and `0` for large
negative `x`.

## In-Place Form

`silu_()` is the in-place sibling of [`silu()`](silu.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `silu_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.silu_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0.73105858]
same object: true
```

## Examples

Example 1. The self-gated shape, with the small negative region on the left.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).silu());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.1423, -0.2689, 0.0000, 0.7311, 2.8577]
```

Example 2. The global minimum sits near `x = -1.278`.

```cpp
pluss::series::print(std::cout, pluss::make_series({-1.278}).silu());
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-0.2785]
```

Example 3. The backward is `s * (1 + x * (1 - s))` — `0.5` at the origin and
briefly negative on the left.

```cpp
auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
x.tracks_slope_(true);

x.silu().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.0881, 0.0723, 0.5000, 0.9277, 1.0881]
```

## Gotchas And Failure Modes

- `silu()` is not monotonic. If you rely on an activation being non-decreasing,
  this is not it.
- Floating-point input only; an integer series raises.
- The forward and the gradient both go slightly negative near `x = -1.3`.

## Summary

`silu(x)` is `x * sigmoid(x)` — a smooth, self-gating, non-monotonic
activation:

- minimum of about `-0.278` near `x = -1.278`
- floating-point input only
- differentiable, backward `sigmoid(x) * (1 + x * (1 - sigmoid(x)))`

## See Also

- [`Series::gelu`](gelu.md): `x * Phi(x)`, the very similar smooth gate.
- [`Series::sigmoid`](sigmoid.md): the gate `silu()` multiplies by.
- [`Series::relu`](relu.md): the hard-gate baseline.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="signbit.html">
    <span class="label">Previous</span>
    <span class="title">signbit</span>
  </a>
  <a class="next-link" href="sin.html">
    <span class="label">Next</span>
    <span class="title">sin</span>
  </a>
</div>
