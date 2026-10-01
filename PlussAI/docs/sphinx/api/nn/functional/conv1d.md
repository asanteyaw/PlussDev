# `pluss::ai::nn::conv1d`

Stateless 1-D convolution; what Conv1d's forward calls internally.

## Signature

```cpp
Series nn::conv1d(const Series& input, const Series& weight,
                   const Series& bias = pluss::series::undefined(),
                   nn::Padding1d padding = {0, 0},
                   std::int64_t stride = 1, std::int64_t dilation = 1,
                   std::int64_t groups = 1);
Series nn::conv1d(const Series& input, const Series& weight, const Series& bias,
                   std::int64_t stride = 1, std::int64_t padding = 0,
                   std::int64_t dilation = 1, std::int64_t groups = 1);
```

## What It Does

Performs a 1-D convolution over `input` (`[N, C, L]`, or `[C, L]`
unbatched), correlating `weight` (`[out_channels, in_channels / groups,
kernel_size]`) against every valid window along the length axis --
exactly what [`Conv1d::forward`](../cnn/Conv1d.md#methods) calls
internally with that instance's own `weight()`/`bias()`. Two overloads,
distinguished by their fourth parameter's type: `Padding1d`
(`std::array<std::int64_t, 2>`, an `{left, right}` pair, for asymmetric
padding -- most notably a causal convolution, built via
[`causal_padding1d`](causal_padding1d.md); see Example 2) or a plain
`std::int64_t` (symmetric padding on both sides, applied via the
`padding` parameter in that overload's own, different position). Because
both overloads have every remaining
parameter defaulted, calling with **exactly three** arguments (`input`,
`weight`, `bias`) is ambiguous -- the compiler cannot tell which overload
is meant. Always supply at least the fourth argument (`padding` as a
`Padding1d`, or `stride` as a plain integer) to disambiguate.

## Parameters

`const Series& input`
: `[N, C, L]` or `[C, L]` (unbatched).

`const Series& weight`
: `[out_channels, in_channels / groups, kernel_size]`.

`const Series& bias`
: `[out_channels]`, or undefined (the default) to skip the addition.

`nn::Padding1d padding` (first overload)
: `{left, right}` -- independently how many zeros to add before and
  after the length axis. `causal_padding1d(kernel_size, dilation)`
  builds exactly the `{left, 0}` pair that makes this convolution
  causal (see [`causal_padding1d`](causal_padding1d.md)).

`std::int64_t padding` (second overload)
: The same amount of zero-padding on both sides -- shorthand for
  `Padding1d{padding, padding}`.

`std::int64_t stride` / `std::int64_t dilation` / `std::int64_t groups`
: Uniform along the single spatial axis; identical in role to
  [`conv`](conv.md)'s own `stride`/`dilation`/`groups`.

## Returns

`[N, out_channels, out_length]` (or unbatched), where `out_length =
(padded_length - dilation * (kernel_size - 1) - 1) / stride + 1`.

## Errors

- `in_channels`/`out_channels` not divisible by `groups`, a kernel
  larger than the padded input, or a rank/shape mismatch -- throws.

## Practical Examples

Four examples: a hand-verified small kernel, a causal convolution built
from `causal_padding1d` confirming no output position ever sees a future
input position, the scalar and `Padding1d` forms agreeing on symmetric
padding, and confirming this function computes bit-for-bit the same
result `Conv1d::forward` does.

### Example 1

A length-3 kernel `{1, 0, 1}` that skips its own middle position -- each
output is hand-verifiable as the sum of its window's first and last
cells.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // input: [N=1, C=1, L=5], weight: [out=1, in=1, K=3]
  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {1, 1, 5});
  const pluss::Series weight = pluss::make_series({1.0, 0.0, 1.0}, {1, 1, 3});

  const pluss::Series y = nn::conv1d(input, weight);

  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2] << "]\n";
  std::cout << "output: " << y << "\n";
  std::cout << "each output = window[0] + window[2] (kernel skips the middle): [1+3, 2+4, 3+5] = [4, 6, 8]\n";

  return 0;
}
```

Output

```text
output shape: [1, 1, 3]
output: pluss::Series(shape=[1,1,3], strides=[3,1,1], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[[4, 6, 8]]]
each output = window[0] + window[2] (kernel skips the middle): [1+3, 2+4, 3+5] = [4, 6, 8]
```

### Example 2

`causal_padding1d(3)` builds `{2, 0}` -- pad only on the left. With a
sum-of-window kernel over `{1, 2, 3, 4}`, output position `0` sees only
`input[0]` (the two left-pad zeros contribute nothing), and output
position `3` (the last) sees `input[1..3]` -- never anything beyond
position `3`, confirming no output position ever looks ahead of itself.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // kernel_size=3 -- causal_padding1d gives {2, 0}: pad 2 zeros on the
  // left, none on the right, so output position t never sees input
  // positions after t
  const nn::Padding1d causal_pad = nn::causal_padding1d(3);
  std::cout << "causal_padding1d(3) = {" << causal_pad[0] << ", " << causal_pad[1] << "}\n";

  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 1, 4});
  const pluss::Series weight = pluss::make_series({1.0, 1.0, 1.0}, {1, 1, 3});

  const pluss::Series y = nn::conv1d(input, weight, pluss::series::undefined(), causal_pad);

  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2]
            << "] -- same length as input, one output per input position\n";
  std::cout << "output: " << y << "\n";
  std::cout << "output[0]=1 (only sees input[0], the two left-pad zeros contribute nothing)\n";
  std::cout << "output[3]=1+2+3+4's last 3 = 2+3+4=9 (sees input[1..3], never input beyond position 3)\n";

  return 0;
}
```

Output

```text
causal_padding1d(3) = {2, 0}
output shape: [1, 1, 4] -- same length as input, one output per input position
output: pluss::Series(shape=[1,1,4], strides=[4,1,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[[1, 3, 6, 9]]]
output[0]=1 (only sees input[0], the two left-pad zeros contribute nothing)
output[3]=1+2+3+4's last 3 = 2+3+4=9 (sees input[1..3], never input beyond position 3)
```

### Example 3

The scalar-form `padding=1` and the array-form `Padding1d{1, 1}` --
symmetric padding on both sides either way -- agree exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {1, 1, 5});
  const pluss::Series weight = pluss::make_series({1.0, 1.0, 1.0}, {1, 1, 3});

  // scalar-form padding=1 is shorthand for the symmetric Padding1d{1, 1}
  const pluss::Series via_scalar = nn::conv1d(input, weight, pluss::series::undefined(), /*stride=*/1, /*padding=*/1);
  const pluss::Series via_array = nn::conv1d(input, weight, pluss::series::undefined(), nn::Padding1d{1, 1});

  std::cout << "scalar-form padding=1 == array-form Padding1d{1,1}: " << std::boolalpha << via_scalar.allclose(via_array) << "\n";
  std::cout << "output: " << via_scalar << "\n";

  return 0;
}
```

Output

```text
scalar-form padding=1 == array-form Padding1d{1,1}: true
output: pluss::Series(shape=[1,1,5], strides=[5,1,1], dtype=float64, platform=cpu, numel=5, contiguous=false)
data=[[[3, 6, 9, 12, 9]]]
```

### Example 4

A live [`Conv1d`](../cnn/Conv1d.md) instance's `forward` compared
directly against this function called on that same instance's own
`weight()`/`bias()` -- identical. Note the explicit `Padding1d{0, 0}`
fourth argument, required here to disambiguate the two overloads (see
[What It Does](#what-it-does)).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Conv1d layer(1, 1, 3);
  layer.to(pluss::ScalarType::Float64);

  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {1, 1, 5}).to(pluss::ScalarType::Float64);

  const pluss::Series via_module = layer.forward(input);
  const pluss::Series via_function = nn::conv1d(input, layer->weight(), layer->bias(), nn::Padding1d{0, 0});

  std::cout << "Conv1d::forward(x): " << via_module << "\n";
  std::cout << "nn::conv1d(x, w, b): " << via_function << "\n";
  std::cout << "identical: " << std::boolalpha << via_module.allclose(via_function) << "\n";

  return 0;
}
```

Output

```text
Conv1d::forward(x): pluss::Series(shape=[1,1,3], strides=[3,3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[[-2.1341209, -3.0204264, -3.9067319]]]
nn::conv1d(x, w, b): pluss::Series(shape=[1,1,3], strides=[3,3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[[-2.1341209, -3.0204264, -3.9067319]]]
identical: true
```

## See Also

- [`causal_padding1d`](causal_padding1d.md) -- builds exactly the
  `Padding1d` value that makes this convolution causal.
- [`conv`](conv.md) -- the 2-D/3-D counterpart, with only the symmetric
  padding form.
- [`Conv1d`](../cnn/Conv1d.md) -- the stateful `Module` wrapper this
  function powers.
