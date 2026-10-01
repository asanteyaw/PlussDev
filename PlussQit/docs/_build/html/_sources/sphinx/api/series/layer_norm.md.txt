<!-- pluss-hand-authored -->

# Series::layer_norm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::layer_norm(
    pluss::msharp::RangeView::Int normalized_shape,
    const Series& weight,
    const Series& bias,
    double eps = 1e-5) const
```

## Normalizing Complete Feature Blocks

`layer_norm()` standardizes each trailing feature block independently, then
applies a learned elementwise scale and offset. If `normalized_shape` contains
`k` extents, the final `k` axes of the receiver form one normalization block;
all leading axes identify independent observations.

For a block containing values $x_i$, the method computes

$$
\mu = \frac{1}{N}\sum_i x_i, \qquad
\sigma^2 = \frac{1}{N}\sum_i (x_i-\mu)^2,
$$

then returns

$$
y_i = \frac{x_i-\mu}{\sqrt{\sigma^2+\varepsilon}}\,w_i+b_i.
$$

The variance is the population variance: its denominator is `N`, not `N-1`.
This is the appropriate normalization for a complete feature block rather than
an estimate of a population statistic.

## Shape Contract

`normalized_shape` is both a declaration and a validation rule. It must be
nonempty and exactly equal the receiver's trailing extents. `weight` and `bias`
must each have that exact shape as well; they are reused across every leading
observation. They are not arbitrary broadcast operands.

For an input shaped `[batch, time, feature]`:

- `{feature}` normalizes every feature vector independently, producing
  `batch * time` normalization blocks;
- `{time, feature}` normalizes one complete time-by-feature block per batch;
- `{batch, time, feature}` normalizes the entire receiver as one block.

The result always has the same shape, dtype, and platform as the receiver. The
input, `weight`, and `bias` must be floating-point Series with matching dtype
and platform. `eps` must be finite and non-negative.

## Reverse Mode

The operation records one analytic reverse node. Sensitivity flows to all three
Series arguments:

- receiver sensitivity accounts for the shared block mean and variance;
- weight sensitivity sums `upstream * normalized_input` across every leading
  axis;
- bias sensitivity sums the upstream sensitivity across every leading axis.

The reverse formula retains its dependencies for higher-order differentiation.
The forward implementation avoids recording a chain of disposable mean,
centering, variance, reciprocal-square-root, multiply, and add nodes.

## Examples

### 1. Normalize each row over its final axis

```cpp
const auto x = pluss::make_series({{1.0, 2.0, 3.0},
                                   {2.0, 4.0, 6.0}});
const auto weight = pluss::make_series({1.0, 1.0, 1.0});
const auto bias = pluss::make_series({0.0, 0.0, 0.0});

pluss::series::print(std::cout, x.layer_norm({3}, weight, bias));
```

#### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -1.2247 0.0000 1.2247 ⎤
⎣ -1.2247 0.0000 1.2247 ⎦
```

Each row has its own mean and variance. Multiplying the second row by two does
not change its normalized coordinates.

### 2. Apply feature-specific scale and bias

```cpp
const auto x = pluss::make_series({{1.0, 2.0, 3.0},
                                   {2.0, 4.0, 6.0}});
const auto weight = pluss::make_series({1.0, 2.0, 0.5});
const auto bias = pluss::make_series({0.0, 1.0, -1.0});

pluss::series::print(std::cout, x.layer_norm({3}, weight, bias));
```

#### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -1.2247 1.0000 -0.3876 ⎤
⎣ -1.2247 1.0000 -0.3876 ⎦
```

The second normalized coordinate is zero, so its output is exactly its bias,
`1`. Affine parameters do not participate in the mean or variance calculation;
they are applied afterward.

### 3. Normalize over two trailing axes

```cpp
const auto blocks = pluss::make_series({
    {{1.0, 2.0}, {3.0, 4.0}},
    {{2.0, 4.0}, {6.0, 8.0}},
});
const auto weight = pluss::series::ones({2, 2}, pluss::ScalarType::Float64);
const auto bias = pluss::series::zeros({2, 2}, pluss::ScalarType::Float64);

pluss::series::print(std::cout,
                     blocks.layer_norm({2, 2}, weight, bias));
```

#### Output

```text
Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data[0, :, :] =
⎡ -1.3416 -0.4472 ⎤
⎣  0.4472  1.3416 ⎦

data[1, :, :] =
⎡ -1.3416 -0.4472 ⎤
⎣  0.4472  1.3416 ⎦
```

Here each `[2,2]` slab is one four-value block. The leading extent `2` remains
the observation axis.

### 4. Stabilize a zero-variance block with `eps`

```cpp
const auto constant = pluss::make_series({{1.0, 1.0, 1.0}});
const auto weight = pluss::make_series({1.0, 1.0, 1.0});
const auto bias = pluss::make_series({0.0, 0.0, 0.0});

pluss::series::print(
    std::cout, constant.layer_norm({3}, weight, bias, 1e-3));
```

#### Output

```text
Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=
[ 0.0000 0.0000 0.0000 ]
```

The centered numerator is zero. A positive `eps` keeps the reciprocal standard
deviation finite even when the block variance is exactly zero.

### 5. Differentiate the receiver, weight, and bias

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0},
                             {2.0, 4.0, 6.0}});
auto weight = pluss::make_series({1.0, 2.0, 0.5});
auto bias = pluss::make_series({0.0, 1.0, -1.0});
x.tracks_slope_(true);
weight.tracks_slope_(true);
bias.tracks_slope_(true);

x.layer_norm({3}, weight, bias).sum().reverse();

pluss::series::print(std::cout, x.sensitivity()) << '\n';
pluss::series::print(std::cout, weight.sensitivity()) << '\n';
pluss::series::print(std::cout, bias.sensitivity());
```

#### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -0.5103 1.0206 -0.5103 ⎤
⎣ -0.2552 0.5103 -0.2552 ⎦

Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2.4495, 0.0000, 2.4495]

Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0000, 2.0000, 2.0000]
```

Bias receives one unit from each of the two rows. Weight receives the sum of
the normalized coordinates. Receiver sensitivity is coupled within each row,
so it is not merely the elementwise weight.

### 6. Reject a declaration that does not match trailing shape

```cpp
try {
  const auto x = pluss::make_series({{1.0, 2.0, 3.0},
                                     {2.0, 4.0, 6.0}});
  const auto weight =
      pluss::series::ones({2}, pluss::ScalarType::Float64);
  const auto bias =
      pluss::series::zeros({2}, pluss::ScalarType::Float64);
  (void)x.layer_norm({2}, weight, bias);
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
layer_norm: normalized_shape must match the input's trailing dimensions
```

The final input extent is `3`, not `2`. Validation occurs before normalization
so an accidentally broadcastable affine tensor cannot conceal the mistake.

## Performance

Work and temporary storage are linear in `numel()`. The receiver is
materialized contiguously for the normalization pass; already contiguous input
avoids that copy. A fused analytic reverse node reduces tape size compared with
manually composing the formula. Reuse persistent weight and bias Series rather
than rebuilding them at every call.

## See Also

- [`mean()`](mean.md) computes the block center without normalization.
- [`standardize()`](standardize.md) provides statistical standardization
  without learned affine parameters.
- [`reshape()`](reshape.md) can arrange feature axes before normalization.
