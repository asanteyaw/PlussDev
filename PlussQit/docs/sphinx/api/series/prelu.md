<!-- pluss-hand-authored -->

# Series::prelu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::prelu(const Series& weight) const -> Series
Series::prelu(double weight) const -> Series
```

## What It Does

`prelu()` is the *parametric* ReLU — [`leaky_relu()`](leaky_relu.md) whose
negative-side slope is a value you can learn:

```
prelu(x) = x           if x > 0
         = weight * x   if x <= 0
```

The difference from `leaky_relu()` is that `weight` is a `Series`, so it can
carry a gradient and be trained, and it can be a **per-channel** vector that
broadcasts against `x` — one slope per feature.

Passing a plain `double` is a convenience overload that wraps it in a scalar
`Series`.

An integer input is promoted to a floating-point result.

## Parameters

**weight** (`const Series&` or `double`)

The negative-side slope. A scalar applies one slope everywhere; a vector
broadcasts against `x` (typically one entry per channel).

## Returns

`Series`
    The activated values, floating point.

## Gradient

`prelu()` is differentiable in **both** arguments.

With respect to `x`:

```
d/dx prelu(x) = 1        if x > 0
              = weight   if x <= 0
```

The boundary `x = 0` takes the `<= 0` branch, so its gradient is `weight`.

With respect to `weight`, each element contributes `x` where `x <= 0` and `0`
where `x > 0`; a scalar weight therefore accumulates the sum of the negative
inputs.

## Examples

Example 1. A scalar slope of `0.25`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).prelu(0.25));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.7500, -0.2500, 0.0000, 1.0000, 3.0000]
```

Example 2. A per-channel slope broadcasts against the last axis.

```cpp
auto x = pluss::make_series({{-2.0, -4.0, 6.0}});
auto slope = pluss::make_series({0.1, 0.2, 0.3});

std::cout << x.prelu(slope);
```

### Output

```text
pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-0.2, -0.8, 6]]
```

Channel `0` uses slope `0.1` (`-2 * 0.1 = -0.2`), channel `1` uses `0.2`, and
channel `2` is positive so its slope is not applied.

Example 3. Backward with respect to `x`: `1` where positive, `weight` where not.

```cpp
auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
auto weight = pluss::make_series({0.25});
x.tracks_slope_(true);
weight.tracks_slope_(true);

x.prelu(weight).sum().reverse();

std::cout << "dx=";
pluss::series::print(std::cout, x.sensitivity());
std::cout << "\ndweight=";
pluss::series::print(std::cout, weight.sensitivity());
```

### Output

```text
dx=Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.2500, 0.2500, 0.2500, 1.0000, 1.0000]
dweight=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-4.0000
```

`dweight` is `(-3) + (-1) + 0 = -4`, the sum of the non-positive inputs.

## Gotchas And Failure Modes

- The boundary `x = 0` is on the negative branch, so its gradient is `weight`
  (this is where `prelu()` and `leaky_relu()` differ from `relu()`, and from
  each other only in whether the slope is learnable).
- A vector `weight` must broadcast against `x`; a length mismatch fails.
- Integer input is promoted to floating point.

## Summary

`prelu(x, weight)` is `leaky_relu()` with a learnable, optionally per-channel
slope:

- `x` for `x > 0`, `weight * x` for `x <= 0`
- differentiable in both `x` and `weight`
- promotes integer input to floating point

## See Also

- [`Series::leaky_relu`](leaky_relu.md): the same shape with a fixed scalar
  slope.
- [`Series::relu`](relu.md): the `weight = 0` case.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pow.html">
    <span class="label">Previous</span>
    <span class="title">pow</span>
  </a>
  <a class="next-link" href="prime.html">
    <span class="label">Next</span>
    <span class="title">prime</span>
  </a>
</div>
