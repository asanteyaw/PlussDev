<!-- pluss-hand-authored -->

# Series::hardshrink

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::hardshrink() const -> Series
Series::hardshrink(double lambd) const -> Series
```

## What It Does

`hardshrink()` zeroes every value whose magnitude is within a threshold and
passes the rest through unchanged:

```
hardshrink(x) = x   if |x| > lambda
              = 0   if |x| <= lambda
```

The default `lambda` is `0.5`. It is a hard "dead zone" around the origin — a
denoising / sparsifying step that removes small-magnitude coefficients (in a
wavelet shrinkage sense) while leaving large ones exactly as they were.

Unlike the shrinkage variant that also subtracts `lambda` from the survivors,
`hardshrink()` does not shift the values it keeps.

An integer input is promoted to a floating-point result; a `float32` input
stays `float32`.

## Parameters

**lambd** (`double`, optional)

The half-width of the dead zone. Defaults to `0.5`. The comparison uses `|x| >
lambda`, so a value exactly at `+/-lambda` is zeroed. Negative `lambd` is
treated as its absolute value.

## Returns

`Series`
    The shrunk values, same shape as the input.

## Gradient

`hardshrink()` is differentiable. Its backward is a straight-through gate:

```
d/dx hardshrink(x) = 1   if |x| > lambda
                   = 0   if |x| <= lambda
```

Outside the dead zone the value is passed through untouched, so the gradient is
`1`; inside, the output is a constant `0`, so the gradient is `0`.

## Examples

Example 1. The default `lambda = 0.5` dead zone.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -0.3, 0.3, 2.0}).hardshrink());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-2.0000, 0.0000, 0.0000, 2.0000]
```

`-0.3` and `0.3` are within `0.5` of zero, so they are removed; `-2` and `2`
pass through unchanged.

Example 2. A wider threshold removes more.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -0.7, 0.7, 2.0}).hardshrink(1.0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-2.0000, 0.0000, 0.0000, 2.0000]
```

With `lambda = 1.0`, `+/-0.7` now falls inside the dead zone.

Example 3. The backward is `1` outside the dead zone and `0` inside — a
straight-through gate.

```cpp
auto x = pluss::make_series({-2.0, -0.3, 0.3, 2.0});
x.tracks_slope_(true);

x.hardshrink().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 0.0000, 0.0000, 1.0000]
```

Example 4. `float32` in, `float32` out.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0f, 0.3f, 2.0f}, pluss::ScalarType::Float32).hardshrink());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[-2.0000, 0.0000, 0.0000]
```

## Gotchas And Failure Modes

- The dead-zone test is strict on the outside (`|x| > lambda`), so a value
  exactly at `+/-lambda` is zeroed.
- `hardshrink()` keeps survivors unchanged; it does not subtract `lambda` from
  them.
- Integer input is promoted to floating point.

## Summary

`hardshrink(x, lambda)` is a hard dead zone around the origin:

- `0` for `|x| <= lambda` (default `0.5`), `x` unchanged otherwise
- promotes integer input; keeps `float32`
- differentiable: straight-through gradient `1` outside the dead zone, `0`
  inside

## See Also

- [`Series::relu`](relu.md): a one-sided hard gate rather than a symmetric dead
  zone.
- [`Series::clamp_min`](clamp_min.md) / [`Series::clamp_max`](clamp_max.md): for
  bounding magnitudes instead of zeroing small ones.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="has_sensitivity.html">
    <span class="label">Next</span>
    <span class="title">has_sensitivity</span>
  </a>
</div>
