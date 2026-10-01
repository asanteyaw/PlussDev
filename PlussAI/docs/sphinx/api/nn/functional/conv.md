# `pluss::ai::nn::conv`

Stateless N-D convolution; `conv2d`/`conv3d` are the fixed-rank forms
[`Conv`](../cnn/Conv.md)/[`Conv3d`](../cnn/Conv3d.md) call internally.

## Signature

```cpp
Series nn::conv(const Series& input, const Series& weight,
                const Series& bias = pluss::series::undefined(),
                std::array<std::int64_t, 2> stride = {1, 1},
                std::array<std::int64_t, 2> padding = {0, 0},
                std::array<std::int64_t, 2> dilation = {1, 1},
                std::int64_t groups = 1);
Series nn::conv(const Series& input, const Series& weight, const Series& bias,
                std::int64_t stride, std::int64_t padding = 0,
                std::int64_t dilation = 1, std::int64_t groups = 1);

Series nn::conv2d(/* identical signature to nn::conv, both overloads */);

Series nn::conv3d(const Series& input, const Series& weight,
                  const Series& bias = pluss::series::undefined(),
                  std::array<std::int64_t, 3> stride = {1, 1, 1},
                  std::array<std::int64_t, 3> padding = {0, 0, 0},
                  std::array<std::int64_t, 3> dilation = {1, 1, 1},
                  std::int64_t groups = 1);
Series nn::conv3d(const Series& input, const Series& weight, const Series& bias,
                  std::int64_t stride, std::int64_t padding = 0,
                  std::int64_t dilation = 1, std::int64_t groups = 1);
```

## What It Does

Performs an ordinary 2-D convolution (`conv`/`conv2d`, genuinely the same
function -- `conv2d` is implemented as a direct call to `conv`) or 3-D
convolution (`conv3d`) over `input`, correlating `weight` against every
valid window. `input` accepts `[N, C, H, W]` (2-D) / `[N, C, D, H, W]`
(3-D), or the unbatched form with the leading `N` axis omitted; `weight`
is `[out_channels, in_channels / groups, kH, kW]` (2-D) or `[out_channels,
in_channels / groups, kD, kH, kW]` (3-D). Each overload comes in two
forms: an `std::array` form for asymmetric stride/padding/dilation per
axis, and a scalar form that applies the same value to every spatial
axis (see Example 2). This is exactly what
[`Conv`](../cnn/Conv.md#methods)/[`Conv3d`](../cnn/Conv3d.md#methods)
call internally with their own `weight()`/`bias()`. `groups` splits both
`in_channels` and `out_channels` into that many independent groups, each
convolving only within its own slice -- see Example 3.

## Parameters

`const Series& input`
: `[N, C, *spatial]` or `[C, *spatial]` (unbatched).

`const Series& weight`
: `[out_channels, in_channels / groups, *kernel_spatial]`.

`const Series& bias`
: `[out_channels]`, or undefined (the default) to skip the addition.

`stride` / `padding` / `dilation`
: Per-axis (array form) or uniform (scalar form) geometry. Padding is
  symmetric on both sides of each axis in this function -- for
  asymmetric (left-only, causal) padding on a single spatial axis, see
  [`conv1d`](conv1d.md)'s `Padding1d` form instead, which this function
  has no equivalent for.

`std::int64_t groups`
: How many independent groups `in_channels`/`out_channels` split into.
  Must evenly divide both. `1` (the default) is an ordinary, ungrouped
  convolution.

## Returns

`[N, out_channels, *out_spatial]` (or unbatched, matching `input`), where
each output spatial extent is `(padded_extent - dilation * (kernel - 1) -
1) / stride + 1`.

## Errors

- `in_channels`/`out_channels` not divisible by `groups`, a kernel larger
  than the padded input, or a rank/shape mismatch between `input` and
  `weight` -- throws.

## Practical Examples

Five examples: a hand-verified small kernel, confirming `conv2d` is
genuinely the same function as `conv` and the scalar/array stride-padding
forms agree, `groups` keeping channels independent, `conv3d` on a small
volume, and confirming this function computes bit-for-bit the same
result [`Conv::forward`](../cnn/Conv.md#methods) does.

### Example 1

A `2x2` kernel picking out exactly the top-left and bottom-right cells of
each window (`{1,0,0,1}`), over a `3x3` input -- each output value is
hand-verifiable as the sum of those two specific input cells.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // input: [N=1, C=1, H=3, W=3], weight: [out=1, in=1, kH=2, kW=2]
  const pluss::Series input = pluss::make_series(
      {1.0, 2.0, 3.0,
       4.0, 5.0, 6.0,
       7.0, 8.0, 9.0},
      {1, 1, 3, 3});
  const pluss::Series weight = pluss::make_series({1.0, 0.0, 0.0, 1.0}, {1, 1, 2, 2});

  const pluss::Series y = nn::conv(input, weight);

  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2] << ", " << y.sizes()[3] << "]\n";
  std::cout << "output: " << y << "\n";
  std::cout << "each output = top-left + bottom-right of its window (kernel picks those two cells): "
               "[1+5, 2+6, 4+8, 5+9] = [6, 8, 12, 14]\n";

  return 0;
}
```

Output

```text
output shape: [1, 1, 2, 2]
output: pluss::Series(shape=[1,1,2,2], strides=[4,1,2,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[[[6, 8],
  [12, 14]]]]
each output = top-left + bottom-right of its window (kernel picks those two cells): [1+5, 2+6, 4+8, 5+9] = [6, 8, 12, 14]
```

### Example 2

`conv2d` computes exactly the same result as `conv` (it calls `conv`
directly); the scalar-form `stride`/`padding` and the equivalent
symmetric array form also agree exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series input = pluss::make_series(
      {1.0, 2.0, 3.0,
       4.0, 5.0, 6.0,
       7.0, 8.0, 9.0},
      {1, 1, 3, 3});
  const pluss::Series weight = pluss::make_series({1.0, 0.0, 0.0, 1.0}, {1, 1, 2, 2});

  const pluss::Series via_conv = nn::conv(input, weight);
  const pluss::Series via_conv2d = nn::conv2d(input, weight);
  std::cout << "conv and conv2d identical: " << std::boolalpha << via_conv.allclose(via_conv2d) << "\n";

  // scalar-form stride/padding/dilation are shorthand for the symmetric array form
  const pluss::Series via_array = nn::conv(input, weight, pluss::series::undefined(),
      std::array<std::int64_t, 2>{2, 2}, std::array<std::int64_t, 2>{1, 1});
  const pluss::Series via_scalar = nn::conv(input, weight, pluss::series::undefined(), 2, 1);
  std::cout << "array-form stride=[2,2],padding=[1,1] == scalar-form stride=2,padding=1: "
            << via_array.allclose(via_scalar) << "\n";

  return 0;
}
```

Output

```text
conv and conv2d identical: true
array-form stride=[2,2],padding=[1,1] == scalar-form stride=2,padding=1: true
```

### Example 3

`groups=2` on a 2-channel input/output keeps each output channel
strictly within its own corresponding input channel -- output channel 0
never sees channel 1's values, and vice versa.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // 2 input channels, 2 output channels, groups=2 -- each output channel
  // sees only its own corresponding input channel, never the other
  const pluss::Series input = pluss::make_series(
      {1.0, 1.0,  // channel 0, 1x2 spatial
       10.0, 10.0}, // channel 1, 1x2 spatial
      {1, 2, 1, 2});
  const pluss::Series weight = pluss::make_series(
      {1.0, 1.0,  // out channel 0 <- in channel 0 only (groups=2)
       1.0, 1.0}, // out channel 1 <- in channel 1 only
      {2, 1, 1, 2});

  const pluss::Series y = nn::conv(input, weight, pluss::series::undefined(),
      std::array<std::int64_t, 2>{1, 1}, std::array<std::int64_t, 2>{0, 0},
      std::array<std::int64_t, 2>{1, 1}, /*groups=*/2);

  std::cout << "output: " << y << "\n";
  std::cout << "out channel 0 = sum of channel 0's own window (1+1=2), unaffected by channel 1's values (10s)\n";
  std::cout << "out channel 1 = sum of channel 1's own window (10+10=20), unaffected by channel 0's values (1s)\n";

  return 0;
}
```

Output

```text
output: pluss::Series(shape=[1,2,1,1], strides=[2,1,2,2], dtype=float64, platform=cpu, numel=2, contiguous=false)
data=[[[[2]],
  [[20]]]]
out channel 0 = sum of channel 0's own window (1+1=2), unaffected by channel 1's values (10s)
out channel 1 = sum of channel 1's own window (10+10=20), unaffected by channel 0's values (1s)
```

### Example 4

`conv3d` over a `2x2x2` volume with a same-sized, all-ones kernel --
one output value, exactly the sum of all 8 input cells.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // input: [N=1, C=1, D=2, H=2, W=2], weight: [out=1, in=1, kD=2, kH=2, kW=2]
  // -- a kernel that sums the entire volume into one output value
  const pluss::Series input = pluss::Series::full({1, 1, 2, 2, 2}, 1.0, pluss::ScalarType::Float64);
  const pluss::Series weight = pluss::Series::full({1, 1, 2, 2, 2}, 1.0, pluss::ScalarType::Float64);

  const pluss::Series y = nn::conv3d(input, weight);

  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2]
            << ", " << y.sizes()[3] << ", " << y.sizes()[4] << "]\n";
  std::cout << "output value: " << y.item<double>() << " -- sum of all 8 input cells (each 1.0)\n";

  return 0;
}
```

Output

```text
output shape: [1, 1, 1, 1, 1]
output value: 8 -- sum of all 8 input cells (each 1.0)
```

### Example 5

A live [`Conv`](../cnn/Conv.md) instance's `forward` compared directly
against this function called on that same instance's own
`weight()`/`bias()` -- identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv layer(1, 1, std::array<std::int64_t, 2>{2, 2});
  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  layer.to(pluss::ScalarType::Float64);

  const pluss::Series input = pluss::make_series(
      {1.0, 2.0, 3.0,
       4.0, 5.0, 6.0,
       7.0, 8.0, 9.0},
      {1, 1, 3, 3}, f64);

  const pluss::Series via_module = layer.forward(input);
  const pluss::Series via_function = nn::conv(input, layer->weight(), layer->bias());

  std::cout << "Conv::forward(x): " << via_module << "\n";
  std::cout << "nn::conv(x, w, b): " << via_function << "\n";
  std::cout << "identical: " << std::boolalpha << via_module.allclose(via_function) << "\n";

  return 0;
}
```

Output

```text
Conv::forward(x): pluss::Series(shape=[1,1,2,2], strides=[4,4,2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[[0.19559169, -0.094499916],
  [-0.67468312, -0.96477473]]]]
nn::conv(x, w, b): pluss::Series(shape=[1,1,2,2], strides=[4,4,2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[[0.19559169, -0.094499916],
  [-0.67468312, -0.96477473]]]]
identical: true
```

## See Also

- [`conv1d`](conv1d.md) -- the 1-D counterpart, with an additional
  asymmetric-padding form for causal convolutions.
- [`Conv`](../cnn/Conv.md) / [`Conv3d`](../cnn/Conv3d.md) -- the stateful
  `Module` wrappers this function powers.
