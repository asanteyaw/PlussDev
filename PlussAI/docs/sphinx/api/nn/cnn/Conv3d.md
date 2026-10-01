# `pluss::ai::nn::Conv3d`

A 3-D convolution over a `{depth, height, width}` volume -- a learned
kernel slides across all three spatial axes, producing one output value
per window position and output channel. `weight` has shape
`[out_channels, in_channels / groups, kernel_d, kernel_h, kernel_w]`;
`bias`, when enabled, has shape `[out_channels]`. `forward` accepts input
shaped `[N, C, D, H, W]` (batched) or `[C, D, H, W]` (a single unbatched
sample, detected by rank).

## Signature

```cpp
explicit nn::Conv3d::Conv3d(nn::Conv3dOptions options);
nn::Conv3d::Conv3d(std::int64_t in_channels, std::int64_t out_channels,
                    std::array<std::int64_t, 3> kernel_size,
                    std::array<std::int64_t, 3> stride = {1, 1, 1},
                    std::array<std::int64_t, 3> padding = {0, 0, 0},
                    std::array<std::int64_t, 3> dilation = {1, 1, 1},
                    std::int64_t groups = 1, bool bias = true,
                    pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::Conv3d::reset_parameters();
```

## Construction

Two equivalent forms -- with the same construction gotcha
[`Conv`](Conv.md) documents for its own array-shaped fields:

```cpp
// Conv3dOptions form -- aggregate initialization handles nested {d, h, w} braces naturally
nn::Conv3d a(nn::Conv3dOptions{1, 4, {2, 2, 2}});

// positional form -- a bare {2, 2, 2} brace literal does not deduce through
// the generic forwarding constructor; an explicit std::array<std::int64_t, 3>
// is required here
nn::Conv3d b(1, 4, std::array<std::int64_t, 3>{2, 2, 2});
```

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned kernel, shape `[out_channels, in_channels / groups,
  kernel_d, kernel_h, kernel_w]`. Registered as a parameter.

`bias()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned per-output-channel bias, shape `[out_channels]`, when
  `options().bias == true`. An **undefined** `Series` otherwise.

`options()` (mutable and `const` overloads) returns the
[`Conv3dOptions`](Conv3dOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Runs the convolution and returns the result. Throws if `input`'s channel
  axis does not equal `in_channels`, or if the (padded) input is smaller
  than the effective kernel in any spatial axis.

`reset_parameters()` -- `void`
: Re-fills `weight()` (and `bias()`, when enabled) in place with a fresh
  uniform draw in `[-1/sqrt(fan_in), 1/sqrt(fan_in)]`, where `fan_in =
  (in_channels / groups) * kernel_d * kernel_h * kernel_w`, using
  `options().seed` for `weight` and `options().seed + 1` for `bias`.

## Practical Examples

Four examples: a hand-verified sum kernel over a single window, the
output-shape formula with stride/padding combined, grouped
(depthwise-style) convolution, and confirming `Conv3d` runs on the MPS
backend.

### Example 1

A kernel of all-`1`s exactly the size of the input volume -- the single
output value is the sum of every input value in the one window.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <array>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv3d conv(1, 1, std::array<std::int64_t, 3>{2, 2, 2});  // in=1, out=1, kernel=2x2x2

  // a "sum everything in the 2x2x2 window" kernel
  conv->weight().copy_(pluss::Series::full({1, 1, 2, 2, 2}, 1.0, pluss::ScalarType::Float32));
  conv->bias().copy_(pluss::make_series({0.0}, {1}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // batch=1, channels=1, a 2x2x2 volume filled with 1.0
  const pluss::Series x = pluss::Series::full({1, 1, 2, 2, 2}, 1.0, pluss::ScalarType::Float64);
  const pluss::Series y = conv.forward(x);

  std::cout << "input shape:  [1,1,2,2,2]\n";
  std::cout << "output shape: [" << y.sizes()[0] << "," << y.sizes()[1] << "," << y.sizes()[2]
            << "," << y.sizes()[3] << "," << y.sizes()[4] << "]\n";
  std::cout << "y (sum of all 8 ones in the one window): " << y << "\n";

  return 0;
}
```

Output

```text
input shape:  [1,1,2,2,2]
output shape: [1,1,1,1,1]
y (sum of all 8 ones in the one window): pluss::Series(shape=[1,1,1,1,1], strides=[1,1,1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[[[8]]]]]
```

### Example 2

The output-shape formula, `(padded - dilation*(kernel-1) - 1) / stride +
1`, applied per axis.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <array>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv3dOptions opts{};
  opts.in_channels = 1;
  opts.out_channels = 1;
  opts.kernel_size = {3, 3, 3};
  opts.stride = {2, 2, 2};
  opts.padding = {1, 1, 1};
  nn::Conv3d conv(opts);

  // batch=1, channels=1, 5x5x5 volume
  const pluss::Series x = pluss::Series::full({1, 1, 5, 5, 5}, 1.0, pluss::ScalarType::Float32);
  const pluss::Series y = conv.forward(x);

  // padded = 5 + 2*1 = 7; out = (7 - 1*2 - 1) / 2 + 1 = 4/2 + 1 = 3
  std::cout << "input 5x5x5, kernel=3x3x3, stride=2, padding=1\n";
  std::cout << "output shape: [" << y.sizes()[2] << ", " << y.sizes()[3] << ", " << y.sizes()[4]
            << "] -- (7-2-1)/2+1 = 3 per axis\n";

  return 0;
}
```

Output

```text
input 5x5x5, kernel=3x3x3, stride=2, padding=1
output shape: [3, 3, 3] -- (7-2-1)/2+1 = 3 per axis
```

### Example 3

`groups=2` on `in_channels=2`/`out_channels=2` -- each output channel only
sees its own input channel.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <array>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv3dOptions opts{};
  opts.in_channels = 2;
  opts.out_channels = 2;
  opts.kernel_size = {1, 1, 1};
  opts.groups = 2;
  nn::Conv3d conv(opts);

  std::cout << "weight shape: [" << conv->weight().sizes()[0] << ", " << conv->weight().sizes()[1]
            << ", " << conv->weight().sizes()[2] << ", " << conv->weight().sizes()[3]
            << ", " << conv->weight().sizes()[4] << "] -- in_channels/groups=1 per group\n";

  conv->weight().copy_(pluss::make_series({3.0, 10.0}, {2, 1, 1, 1, 1}));
  conv->bias().copy_(pluss::make_series({0.0, 0.0}, {2}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // channel 0: all 1s, channel 1: all 1s, single 1x1x1 voxel
  const pluss::Series x = pluss::make_series({1.0, 1.0}, {1, 2, 1, 1, 1}, f64);
  const pluss::Series y = conv.forward(x);

  std::cout << "y: " << y << " -- channel 0 scaled by 3, channel 1 scaled by 10\n";

  return 0;
}
```

Output

```text
weight shape: [2, 1, 1, 1, 1] -- in_channels/groups=1 per group
y: pluss::Series(shape=[1,2,1,1,1], strides=[2,1,1,1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[[[[3]]],
  [[[10]]]]] -- channel 0 scaled by 3, channel 1 scaled by 10
```

### Example 4

`Conv3d` runs on the MPS backend -- unlike [`Conv`](Conv.md) (2-D), this
layer has a genuine native MPS kernel it attempts first, only falling back
to a CPU-computed path if that native routing declines the call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <array>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Conv3d conv(1, 1, std::array<std::int64_t, 3>{2, 2, 2});
  conv.to(pluss::pMPS);

  std::cout << "weight platform after .to(pMPS): " << static_cast<int>(conv->weight().platform()) << "\n";

  const Series x = pluss::Series::full({1, 1, 3, 3, 3}, 1.0, pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series y = conv.forward(x);
    std::cout << "conv3d on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "conv3d on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight platform after .to(pMPS): 2
conv3d on MPS: OK, platform=2
```

## Changelog

- **0.1** — 3-D convolution, `in_channels`/`out_channels`/`kernel_size`/`stride`/
  `padding`/`dilation`/`groups`/`bias`/`cfg`/`seed` options.

## See Also

- [`Conv3dOptions`](Conv3dOptions.md) -- the options struct.
- [`Conv`](Conv.md) -- the 2-D case (also `Conv2d`/`ConvNd`).
- [`Conv1d`](Conv1d.md) -- the 1-D case.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Conv3dOptions
```
<!-- apiref:toctree:end -->
