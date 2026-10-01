# `pluss::ai::nn::Conv`

A 2-D convolution over a `{height, width}` spatial grid -- a learned
kernel slides across both spatial axes, producing one output value per
window position and output channel. `weight` has shape `[out_channels,
in_channels / groups, kernel_h, kernel_w]`; `bias`, when enabled, has
shape `[out_channels]`. `forward` accepts input shaped `[N, C, H, W]`
(batched) or `[C, H, W]` (a single unbatched sample, detected by rank).
Also spelled [`Conv2d`](#construction) and `ConvNd` -- both are plain type
aliases for this exact class, not separate implementations.

## Signature

```cpp
explicit nn::Conv::Conv(nn::ConvOptions options);
nn::Conv::Conv(std::int64_t in_channels, std::int64_t out_channels,
                std::array<std::int64_t, 2> kernel_size,
                std::array<std::int64_t, 2> stride = {1, 1},
                std::array<std::int64_t, 2> padding = {0, 0},
                std::array<std::int64_t, 2> dilation = {1, 1},
                std::int64_t groups = 1, bool bias = true,
                pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::Conv::reset_parameters();
```

## Construction

Two equivalent forms -- but unlike [`Fusion`](../fusion/Fusion.md) or
[`Conv1d`](Conv1d.md), the two forms are **not** equally easy to call
directly with a literal:

```cpp
// ConvOptions form -- aggregate initialization handles nested {h, w} braces naturally
nn::Conv a(nn::ConvOptions{1, 4, {3, 3}});

// positional form -- a bare {3, 3} brace literal cannot be deduced through
// the generic forwarding constructor every Module wrapper type shares; an
// explicit std::array<std::int64_t, 2> is required here
nn::Conv b(1, 4, std::array<std::int64_t, 2>{3, 3});
```

This is a real, narrow gap specific to array-shaped constructor
parameters (`kernel_size`/`stride`/`padding`/`dilation` here) -- it does
not affect [`Conv1d`](Conv1d.md), whose geometry fields are plain
`std::int64_t`. When in doubt, the `ConvOptions` form avoids the issue
entirely.

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned kernel, shape `[out_channels, in_channels / groups,
  kernel_h, kernel_w]`. Registered as a parameter.

`bias()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned per-output-channel bias, shape `[out_channels]`, when
  `options().bias == true`. An **undefined** `Series` otherwise.

`options()` (mutable and `const` overloads) returns the
[`ConvOptions`](ConvOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Runs the convolution and returns the result. Throws if `input`'s channel
  axis does not equal `in_channels`, or if the (padded) input is smaller
  than the effective kernel in either spatial axis.

`reset_parameters()` -- `void`
: Re-fills `weight()` (and `bias()`, when enabled) in place with a fresh
  uniform draw in `[-1/sqrt(fan_in), 1/sqrt(fan_in)]`, where `fan_in =
  (in_channels / groups) * kernel_h * kernel_w`, using `options().seed` for
  `weight` and `options().seed + 1` for `bias`.

## Practical Examples

Five examples: a hand-verified averaging kernel confirming the aliases,
the output-shape formula with stride/padding/dilation combined, grouped
(depthwise-style) convolution, the invalid-geometry error, and confirming
`Conv` runs on the MPS backend.

### Example 1

A 2x2 averaging kernel over a 3x3 input, verified by hand, plus confirming
`Conv2d`/`ConvNd` really are the same type as `Conv`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <array>
#include <iostream>
#include <type_traits>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv conv(1, 1, std::array<std::int64_t, 2>{2, 2});  // in_channels=1, out_channels=1, kernel_size=2x2

  // an averaging kernel
  conv->weight().copy_(pluss::make_series({0.25, 0.25, 0.25, 0.25}, {1, 1, 2, 2}));
  conv->bias().copy_(pluss::make_series({0.0}, {1}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // batch=1, channels=1, 3x3 image
  const pluss::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {1, 1, 3, 3}, f64);
  const pluss::Series y = conv.forward(x);

  std::cout << "input shape: [1,1,3,3], output shape: [" << y.sizes()[0] << "," << y.sizes()[1]
            << "," << y.sizes()[2] << "," << y.sizes()[3] << "]\n";
  std::cout << "y (2x2 averages): " << y << "\n";

  // nn::Conv2d and nn::ConvNd are the same type
  static_assert(std::is_same_v<nn::Conv2d, nn::Conv>);
  static_assert(std::is_same_v<nn::ConvNd, nn::Conv>);
  std::cout << "Conv2d and ConvNd are aliases of Conv\n";

  return 0;
}
```

Output

```text
input shape: [1,1,3,3], output shape: [1,1,2,2]
y (2x2 averages): pluss::Series(shape=[1,1,2,2], strides=[4,4,2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[[3, 4],
  [6, 7]]]]
Conv2d and ConvNd are aliases of Conv
```

### Example 2

The output-shape formula, `(padded - dilation*(kernel-1) - 1) / stride +
1`, applied per axis (both axes are equal here since the geometry is
square).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::ConvOptions opts{};
  opts.in_channels = 1;
  opts.out_channels = 1;
  opts.kernel_size = {3, 3};
  opts.stride = {2, 2};
  opts.padding = {1, 1};
  opts.dilation = {1, 1};
  nn::Conv conv(opts);

  const pluss::Series x = pluss::Series::full({1, 1, 7, 7}, 1.0, pluss::ScalarType::Float64);
  const pluss::Series y = conv.forward(x);

  // out = (padded - dilation*(kernel-1) - 1) / stride + 1, padded = 7 + 2*1 = 9
  // out = (9 - 1*2 - 1) / 2 + 1 = 6/2 + 1 = 4
  std::cout << "input 7x7, kernel=3x3, stride=2, padding=1, dilation=1\n";
  std::cout << "output shape: [" << y.sizes()[2] << ", " << y.sizes()[3] << "] -- (9-2-1)/2+1 = 4\n";

  return 0;
}
```

Output

```text
input 7x7, kernel=3x3, stride=2, padding=1, dilation=1
output shape: [4, 4] -- (9-2-1)/2+1 = 4
```

### Example 3

`groups=2` on `in_channels=2`/`out_channels=2` -- each output channel only
sees its own input channel, and `weight`'s second dimension shrinks to
`in_channels / groups = 1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::ConvOptions opts{};
  opts.in_channels = 2;
  opts.out_channels = 2;
  opts.kernel_size = {1, 1};
  opts.groups = 2;  // depthwise: each output channel only sees its own input channel
  nn::Conv conv(opts);

  std::cout << "weight shape: [" << conv->weight().sizes()[0] << ", " << conv->weight().sizes()[1]
            << ", " << conv->weight().sizes()[2] << ", " << conv->weight().sizes()[3]
            << "] -- in_channels/groups=1 per group, not in_channels=2\n";

  conv->weight().copy_(pluss::make_series({2.0, 0.5}, {2, 1, 1, 1}));
  conv->bias().copy_(pluss::make_series({0.0, 0.0}, {2}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // channel 0: all 10s, channel 1: all 4s, 2x2 spatial
  const pluss::Series x = pluss::make_series(
      {10.0, 10.0, 10.0, 10.0, 4.0, 4.0, 4.0, 4.0}, {1, 2, 2, 2}, f64);
  const pluss::Series y = conv.forward(x);

  std::cout << "y: " << y << " -- channel 0 scaled by 2, channel 1 scaled by 0.5\n";

  return 0;
}
```

Output

```text
weight shape: [2, 1, 1, 1] -- in_channels/groups=1 per group, not in_channels=2
y: pluss::Series(shape=[1,2,2,2], strides=[8,4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[[20, 20],
  [20, 20]],
  [[2, 2],
  [2, 2]]]] -- channel 0 scaled by 2, channel 1 scaled by 0.5
```

### Example 4

An invalid `kernel_size` throws at construction time.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  try {
    nn::ConvOptions opts{};
    opts.in_channels = 1;
    opts.out_channels = 1;
    opts.kernel_size = {0, 0};
    nn::Conv conv(opts);
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::ai::nn::Conv: kernel_size values must be > 0
```

### Example 5

`Conv` runs on the MPS backend and produces a correctly-placed MPS result
-- but, as of this writing, computation for this layer transparently
bridges through the CPU by default on MPS rather than running a native
MPS kernel (controlled by the `PLUSS_MPS_CONV_CPU_BRIDGE` environment
variable). The output still lands back on the MPS platform either way; the
bridging only affects performance, not correctness or the platform of the
result.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <array>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Conv conv(1, 1, std::array<std::int64_t, 2>{2, 2});
  conv.to(pluss::pMPS);

  std::cout << "weight platform after .to(pMPS): " << static_cast<int>(conv->weight().platform()) << "\n";

  const Series x = pluss::Series::full({1, 1, 4, 4}, 1.0, pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series y = conv.forward(x);
    std::cout << "conv2d on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "conv2d on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight platform after .to(pMPS): 2
conv2d on MPS: OK, platform=2
```

## Changelog

- **0.1** — 2-D convolution, `in_channels`/`out_channels`/`kernel_size`/`stride`/
  `padding`/`dilation`/`groups`/`bias`/`cfg`/`seed` options; also reachable
  as `Conv2d`/`ConvNd`.

## See Also

- [`ConvOptions`](ConvOptions.md) -- the options struct (also
  `Conv2dOptions`/`ConvNdOptions`).
- [`Conv1d`](Conv1d.md) -- the 1-D specialization; `Conv1d` is implemented
  on top of this class internally.
- [`Conv3d`](Conv3d.md) -- the 3-D generalization.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

ConvOptions
```
<!-- apiref:toctree:end -->
