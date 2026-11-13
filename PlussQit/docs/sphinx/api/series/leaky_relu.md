<!-- pluss-hand-authored -->

# Series::leaky_relu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::leaky_relu(double negative_slope) const
Series& Series::leaky_relu_(double negative_slope)
```

## What It Does

`leaky_relu()` is [`relu()`](relu.md) with a small non-zero slope on the
negative side:

```
leaky_relu(x) = x            if x >= 0
              = slope * x    if x < 0
```

`relu()` zeroes every negative input, and a unit that outputs `0` also has
gradient `0`, so it can get stuck ("die") and never recover. `leaky_relu()`
lets a small fraction of the negative signal — and its gradient — through, so
the unit keeps learning.

Typical `negative_slope` values are `0.01` or `0.1`. The slope is a fixed
hyperparameter here; for a *learned* per-channel slope use
[`prelu()`](prelu.md).

An integer input is promoted to a floating-point result, since a fractional
slope produces non-integers.

## Parameters

**negative_slope** (`double`)

The multiplier applied to negative inputs. Required (there is no default).

## Returns

`Series` or `Series&`
    The activated values. `leaky_relu_()` overwrites the current storage.

## Gradient

`leaky_relu()` is differentiable:

```
d/dx leaky_relu(x) = 1      if x >= 0
                   = slope  if x < 0
```

The boundary at `x = 0` takes the `>= 0` branch, so its gradient is `1`. Unlike
`relu()`, the negative-side gradient is `slope` rather than `0`.

## In-Place Form

`leaky_relu_(negative_slope)` is the in-place sibling of `leaky_relu(negative_slope)`:
it computes the same result and writes it back into `self` directly — mutating
it (and any other handle sharing the same storage) instead of allocating a
new `Series` — then returns a reference to `self`.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.leaky_relu_(0.1);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.2, 0, 2]
```

## Examples

Example 1. A `0.1` slope on the negative half, identity on the positive half.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).leaky_relu(0.1));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.3000, -0.1000, 0.0000, 1.0000, 3.0000]
```

Example 2. A smaller slope leaks less.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).leaky_relu(0.01));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.0300, -0.0100, 0.0000, 1.0000, 3.0000]
```

Example 3. The backward is `1` on the non-negative side and `slope` on the
negative side.

```cpp
auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
x.tracks_slope_(true);

x.leaky_relu(0.1).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.1000, 0.1000, 1.0000, 1.0000, 1.0000]
```

The negative positions keep a `0.1` gradient — the whole point of the "leak".

Example 4. An integer input is promoted rather than truncating the slope.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2, -1, 0, 3}, pluss::ScalarType::Int64).leaky_relu(0.1));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[-0.2000, -0.1000, 0.0000, 3.0000]
```

## Gotchas And Failure Modes

- `negative_slope` has no default; you must pass it.
- The gradient at exactly `x = 0` is `1` (the non-negative branch).
- For a learned slope, use [`prelu()`](prelu.md); `leaky_relu()`'s slope is a
  fixed constant.

## Summary

`leaky_relu(x, slope)` is `relu()` with a leak:

- `x` for `x >= 0`, `slope * x` for `x < 0`
- promotes integer input to floating point
- differentiable: gradient `1` on the non-negative side, `slope` on the
  negative side

## See Also

- [`Series::relu`](relu.md): the `slope = 0` case, prone to dying units.
- [`Series::prelu`](prelu.md): the same shape with a learnable slope.
- [`Series::gelu`](gelu.md) / [`Series::silu`](silu.md): smooth alternatives.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="lerp.html">
    <span class="label">Next</span>
    <span class="title">lerp</span>
  </a>
</div>
