# `pluss::ai::nn::Conv1d`

A 1-D convolution over a length axis: a learned kernel slides across the
input, producing one output value per window position and output channel.
`weight` has shape `[out_channels, in_channels / groups, kernel_size]`;
`bias`, when enabled, has shape `[out_channels]`. `forward` accepts input
shaped `[N, C, L]` (batched) or `[C, L]` (a single unbatched sample,
detected by rank) and returns the same rank with `C` mapped from
`in_channels` to `out_channels` and `L` shrunk according to
`kernel_size`/`stride`/`padding`/`dilation`.

## Signature

```cpp
explicit nn::Conv1d::Conv1d(nn::Conv1dOptions options);
nn::Conv1d::Conv1d(std::int64_t in_channels, std::int64_t out_channels, std::int64_t kernel_size,
                    std::int64_t stride = 1, std::int64_t padding = 0, std::int64_t dilation = 1,
                    std::int64_t groups = 1, bool bias = true, pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::Conv1d::reset_parameters();
```

## Construction

Two equivalent forms: pass a [`Conv1dOptions`](Conv1dOptions.md) directly,
or spell out the geometry fields as positional arguments:

```cpp
nn::Conv1d a(nn::Conv1dOptions{1, 4, 3});  // Conv1dOptions form
nn::Conv1d b(1, 4, 3);                     // positional form, everything else defaulted
```

The positional constructor has no way to set `padding_lr` -- it always
constructs with `padding_lr = {0, 0}` and relies on the plain `padding`
field. Reach for the `Conv1dOptions` form when asymmetric padding matters.

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned kernel, shape `[out_channels, in_channels / groups,
  kernel_size]`. Registered as a parameter.

`bias()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned per-output-channel bias, shape `[out_channels]`, when
  `options().bias == true`. Registered as a parameter in that case; an
  **undefined** `Series` when `options().bias == false`.

`options()` (mutable and `const` overloads) returns the
[`Conv1dOptions`](Conv1dOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Runs the convolution and returns the result. Throws if `input`'s channel
  axis does not equal `in_channels`, or if the (padded) input is narrower
  than the effective kernel width.

`padding()` -- `pluss::ai::Padding1d`
: The effective `{left, right}` padding actually used by `forward` --
  `options().padding_lr` when either side is non-zero, otherwise
  `{options().padding, options().padding}`.

`reset_parameters()` -- `void`
: Re-fills `weight()` (and `bias()`, when enabled) in place with a fresh
  uniform draw in `[-1/sqrt(fan_in), 1/sqrt(fan_in)]`, where `fan_in =
  (in_channels / groups) * kernel_size`, using `options().seed` for
  `weight` and `options().seed + 1` for `bias`. Called once during
  construction; call it again to reinitialize an already-trained layer.

## Practical Examples

Five examples: a hand-verified convolution confirming the output-length
formula, `padding_lr` overriding `padding`, grouped (depthwise-style)
convolution, the batched-vs-unbatched input forms, and confirming
`Conv1d` runs on the MPS backend.

### Example 1

A `[1, 1]` moving-sum kernel over a length-5 input, verified against the
output-length formula `(L - dilation*(kernel_size-1) - 1) / stride + 1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // in_channels=1, out_channels=1, kernel_size=2, stride=1 (defaults)
  nn::Conv1d conv(1, 1, 2);
  conv->weight().copy_(pluss::make_series({1.0, 1.0}, {1, 1, 2}));  // simple moving-sum kernel
  conv->bias().copy_(pluss::make_series({0.0}, {1}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // batch=1, channels=1, length=5
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {1, 1, 5}, f64);
  const pluss::Series y = conv.forward(x);

  std::cout << "input length: " << x.sizes()[2] << "\n";
  std::cout << "output length: " << y.sizes()[2]
            << " -- (5 - 1*(2-1) - 1) / 1 + 1 = 4\n";
  std::cout << "y (moving sum of adjacent pairs): " << y << "\n";

  return 0;
}
```

Output

```text
input length: 5
output length: 4 -- (5 - 1*(2-1) - 1) / 1 + 1 = 4
y (moving sum of adjacent pairs): pluss::Series(shape=[1,1,4], strides=[4,4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[3, 5, 7, 9]]]
```

### Example 2

`padding_lr={1,0}` overrides `padding=5` entirely -- the effective padding
is `(1, 0)`, not `(5, 5)` and not any combination of the two.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv1dOptions opts{};
  opts.in_channels = 1;
  opts.out_channels = 1;
  opts.kernel_size = 2;
  opts.padding = 5;             // ignored once padding_lr is non-zero
  opts.padding_lr = {1, 0};     // pad 1 on the left, 0 on the right -- causal-style
  nn::Conv1d conv(opts);
  conv->weight().copy_(pluss::make_series({1.0, 1.0}, {1, 1, 2}));
  conv->bias().copy_(pluss::make_series({0.0}, {1}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 1, 3}, f64);
  const pluss::Series y = conv.forward(x);

  std::cout << "padding_lr={1,0} wins over padding=5: effective padding is (1,0)\n";
  std::cout << "output length: " << y.sizes()[2] << " -- (3+1 - 1*(2-1) - 1) / 1 + 1 = 3\n";
  std::cout << "y: " << y << " -- first output uses the zero pad: 0+1=1\n";

  return 0;
}
```

Output

```text
padding_lr={1,0} wins over padding=5: effective padding is (1,0)
output length: 3 -- (3+1 - 1*(2-1) - 1) / 1 + 1 = 3
y: pluss::Series(shape=[1,1,3], strides=[3,3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[[1, 3, 5]]] -- first output uses the zero pad: 0+1=1
```

### Example 3

`groups=2` on `in_channels=2`/`out_channels=2` is depthwise convolution --
`weight`'s second dimension shrinks to `in_channels / groups = 1`, and
each output channel only ever sees its own input channel.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // depthwise-style: in_channels=2, groups=2 -- each output channel only sees its own input channel
  nn::Conv1dOptions opts{};
  opts.in_channels = 2;
  opts.out_channels = 2;
  opts.kernel_size = 2;
  opts.groups = 2;
  nn::Conv1d conv(opts);

  std::cout << "weight shape: [" << conv->weight().sizes()[0] << ", " << conv->weight().sizes()[1]
            << ", " << conv->weight().sizes()[2] << "] -- in_channels/groups=1 per group, not in_channels=2\n";

  conv->weight().copy_(pluss::make_series({1.0, 0.0, 0.0, 1.0}, {2, 1, 2}));  // group 0: [1,0], group 1: [0,1]
  conv->bias().copy_(pluss::make_series({0.0, 0.0}, {2}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // channel 0: [10, 20, 30], channel 1: [1, 2, 3]
  const pluss::Series x = pluss::make_series({10.0, 20.0, 30.0, 1.0, 2.0, 3.0}, {1, 2, 3}, f64);
  const pluss::Series y = conv.forward(x);

  std::cout << "y: " << y << " -- out channel 0 only sees in channel 0, out channel 1 only sees in channel 1\n";

  return 0;
}
```

Output

```text
weight shape: [2, 1, 2] -- in_channels/groups=1 per group, not in_channels=2
y: pluss::Series(shape=[1,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[10, 20],
  [2, 3]]] -- out channel 0 only sees in channel 0, out channel 1 only sees in channel 1
```

### Example 4

A `[C, L]` unbatched input is accepted directly, auto-detected by rank --
no leading batch axis in, none in the output either, and the values match
the batched form exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv1d conv(1, 1, 2);
  conv->weight().copy_(pluss::make_series({1.0, 1.0}, {1, 1, 2}));
  conv->bias().copy_(pluss::make_series({0.0}, {1}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  // batched: [N=1, C=1, L=4]
  const pluss::Series x_batched = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 1, 4}, f64);
  const pluss::Series y_batched = conv.forward(x_batched);
  std::cout << "batched input [1,1,4] -> output shape: [" << y_batched.sizes()[0] << ", "
            << y_batched.sizes()[1] << ", " << y_batched.sizes()[2] << "]\n";

  // unbatched: [C=1, L=4] -- the leading batch axis is simply omitted
  const pluss::Series x_unbatched = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 4}, f64);
  const pluss::Series y_unbatched = conv.forward(x_unbatched);
  std::cout << "unbatched input [1,4] -> output shape: [" << y_unbatched.sizes()[0] << ", "
            << y_unbatched.sizes()[1] << "] -- no batch axis in or out\n";

  std::cout << "same values either way: " << std::boolalpha
            << y_batched.reshape({3}).allclose(y_unbatched.reshape({3})) << "\n";

  return 0;
}
```

Output

```text
batched input [1,1,4] -> output shape: [1, 1, 3]
unbatched input [1,4] -> output shape: [1, 3] -- no batch axis in or out
same values either way: true
```

### Example 5

`Conv1d` runs on the MPS backend the same way it does on CPU. Internally
it shares its computation path with [`Conv`](Conv.md) (2-D) via a
dummy width-1 axis, which currently bridges through the CPU on MPS by
default rather than running a native MPS kernel -- see
[`Conv`](Conv.md)'s own MPS example for that note in full; the result is
still correct and still lands back on the MPS platform.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Conv1d conv(1, 1, 2);
  conv.to(pluss::pMPS);

  std::cout << "weight platform after .to(pMPS): " << static_cast<int>(conv->weight().platform()) << "\n";

  const Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series y = conv.forward(x);
    std::cout << "conv1d on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "conv1d on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight platform after .to(pMPS): 2
conv1d on MPS: OK, platform=2
```

## Changelog

- **0.1** — 1-D convolution, `in_channels`/`out_channels`/`kernel_size`/`stride`/
  `padding`/`padding_lr`/`dilation`/`groups`/`bias`/`cfg`/`seed` options.

## See Also

- [`Conv1dOptions`](Conv1dOptions.md) -- the options struct.
- [`Conv`](Conv.md) -- the 2-D generalization (also aliased `Conv2d`,
  `ConvNd`); `Conv1d` is implemented on top of it internally.
- [`Conv3d`](Conv3d.md) -- the 3-D generalization.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Conv1dOptions
```
<!-- apiref:toctree:end -->
