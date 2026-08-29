<!-- pluss-hand-authored -->

# Series::tanh

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::tanh() const -> Series
```

## What It Does

Applies the hyperbolic tangent elementwise.

`tanh()` is the bounded ratio-style hyperbolic transform. For real inputs, the
output always stays in `(-1, 1)`, with large negative values saturating toward
`-1` and large positive values saturating toward `1`.

That makes it useful whenever you want a smooth signed squashing transform
rather than the unbounded growth of [`Series::sinh`](sinh.md) or
[`Series::cosh`](cosh.md).

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its hyperbolic tangent.

## Gradient

`tanh(x)` is differentiable for all real inputs, with derivative
`1 - tanh(x)^2`.

So:

- the derivative is largest at zero
- it shrinks as the output saturates toward `-1` or `1`
- very large magnitudes can leave you with very small sensitivities

The verified reverse-mode example below reflects that behavior on the current
surface.

## Examples

Example 1. Small signed inputs show the familiar bounded squashing behavior.

```cpp
std::cout << pluss::series::print(
    pluss::make_series({-2.0, 0.0, 2.0}).tanh());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.9640, 0.0000, 0.9640]
```

Example 2. Shape is preserved on 2D inputs as well.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        {-2.0, -0.5},
        {0.5, 2.0},
    }).tanh());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -0.9640 -0.4621 ⎤
⎣  0.4621  0.9640 ⎦
```

Example 3. Reverse-mode sensitivity is largest near zero and much smaller in
the saturated outer region.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.tracks_slope_(true);

auto out = x.tanh();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.96402758, 0, 0.96402758]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.070650825, 1, 0.070650825]
```

Example 4. Larger magnitudes push the output very close to `-1` and `1`.

```cpp
std::cout << pluss::make_series({-8.0, -3.0, 3.0, 8.0}).tanh();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.99999977, -0.99505475, 0.99505475, 0.99999977]
```

## Gotchas And Failure Modes

- `tanh()` is bounded in value, but not in informational loss; large-magnitude
  inputs become hard to distinguish after saturation.
- Saturation also shrinks sensitivities, which matters in gradient-based
  workflows.
- Inputs are real numbers, not percentages; the `[-1, 1]` range belongs to the
  output, not the required input.

## Summary

Use `tanh()` when you need a smooth signed squashing transform with bounded
output and verified reverse-mode behavior on the current surface.

## See Also

- [`Series::sinh`](sinh.md): unbounded odd hyperbolic companion.
- [`Series::cosh`](cosh.md): unbounded even hyperbolic companion.
- [`Series::sign`](sign.md): hard sign extraction instead of smooth squashing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tan.html">
    <span class="label">Previous</span>
    <span class="title">tan</span>
  </a>
  <a class="next-link" href="tracks_slope.html">
    <span class="label">Next</span>
    <span class="title">tracks_slope</span>
  </a>
</div>
