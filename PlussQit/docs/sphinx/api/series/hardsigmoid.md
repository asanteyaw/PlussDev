<!-- pluss-hand-authored -->

# Series::hardsigmoid

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::hardsigmoid() const
Series& Series::hardsigmoid_() const
```

## A Clipped Linear Gate

`hardsigmoid()` applies a piecewise-linear approximation to a smooth sigmoid:

```text
hardsigmoid(x) = 0             when x <= -3
                 x / 6 + 1/2  when -3 < x < 3
                 1             when x >= 3
```

It is intended for gates and bounded activations where the strict `[0, 1]`
range matters but an exponential is unnecessary. Values at or below `-3`
are closed off, values at or above `3` are fully open, and the six-unit middle
interval changes at constant slope. Use [`sigmoid()`](sigmoid.md) when a smooth
curve and non-zero tails matter more than the cheaper linear middle branch.

The operation is elementwise, out of place, and shape preserving. Real
floating input retains its dtype. Integral and Boolean input promote to
`float32`; this is essential because interior values such as `1` map to
`2/3`, not to an integer. Complex input is invalid because the branch tests
require an ordered real value. Empty and higher-rank inputs follow the same
elementwise contract without special flattening or reduction.

The result is newly allocated on the receiver's platform. Saturated values are
exactly zero or one in the output dtype; they are not merely close to those
bounds.

## Reverse Mode And Boundaries

The local derivative is

```text
1 / 6    when -3 < x < 3
0        otherwise
```

Both endpoints belong to the saturated branches, so the recorded derivative
at exactly `-3` and exactly `3` is zero. The second derivative is zero wherever
the first derivative is classically defined; the two corners remain
non-differentiable branch boundaries.

## In-Place Form

`hardsigmoid_()` is the in-place sibling of [`hardsigmoid()`](hardsigmoid.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `hardsigmoid_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-4.0, 0.0, 4.0});
x.hardsigmoid_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0.5, 1]
```

## Examples

Example 1. This vector includes both saturated regions, both exact boundaries,
and three interior points.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0})
        .hardsigmoid());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[0.0000, 0.0000, 0.2500, 0.5000, 0.7500, 1.0000, 1.0000]
```

Example 2. Integer matrices preserve shape and promote to `float32` so the
interior fraction survives.

```cpp
auto y = pluss::make_series({{-6, -3, 0}, {1, 3, 6}}).hardsigmoid();
pluss::series::print(std::cout, y);
std::cout << "dtype=" << y.scalar_type() << " dim=" << y.dim()
          << " shape=[" << y.size(0) << ',' << y.size(1) << ']';
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0.0000 0.0000 0.5000 ⎤
⎣ 0.6667 1.0000 1.0000 ⎦
dtype=float32 dim=2 shape=[2,3]
```

Example 3. Reverse mode makes the open-interval boundary rule explicit.

```cpp
auto x = pluss::make_series({-4.0, -3.0, -1.0, 0.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.hardsigmoid().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[0.0000, 0.0000, 0.1667, 0.1667, 0.1667, 0.0000, 0.0000]
```

Example 4. Chaining the result as a gate keeps the bounded factor in the Series
graph.

```cpp
auto logits = pluss::make_series({-6.0, 0.0, 6.0});
auto payload = pluss::make_series({10.0, 20.0, 30.0});
pluss::series::print(std::cout, payload.mul(logits.hardsigmoid()));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 10.0000, 30.0000]
```

## See Also

- [`sigmoid()`](sigmoid.md) provides the smooth logistic curve.
- [`hardswish()`](hardswish.md) multiplies the input by this gate.
- [`clamp_min()`](clamp_min.md) and [`clamp_max()`](clamp_max.md) provide the
  two one-sided bounds used to build a clamp without applying the middle
  rescaling.
