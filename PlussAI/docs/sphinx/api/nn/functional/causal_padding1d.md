# `pluss::ai::nn::causal_padding1d`

Computes the left-only padding width that makes a 1-D convolution
causal.

## Signature

```cpp
nn::Padding1d nn::causal_padding1d(std::int64_t kernel_size, std::int64_t dilation = 1);
```

## What It Does

Returns `{dilation * (kernel_size - 1), 0}` -- a
[`Padding1d`](conv1d.md#what-it-does) pair with padding only on the
left, zero on the right. Passed as [`conv1d`](conv1d.md)'s `padding`
argument, this guarantees output position `t` depends only on input
positions `<= t`, never any position after it -- unlike ordinary
symmetric ("same") padding, which keeps the output the same length but
lets every output position see a symmetric window straddling both past
*and* future input positions (see Example 3). This is the standard
padding choice for autoregressive 1-D convolutions (e.g. WaveNet-style
architectures), where a position must never be computed from information
that would not yet be available at inference time.

## Parameters

`std::int64_t kernel_size`
: The convolution's kernel size. Must be `> 0`.

`std::int64_t dilation`
: The convolution's dilation. Must be `> 0`. Defaults to `1`
  (undilated).

## Returns

A [`Padding1d`](conv1d.md#what-it-does) (`{dilation * (kernel_size - 1),
0}`).

## Errors

- `kernel_size`/`dilation` non-positive -- throws.

## Practical Examples

Three examples: the left-padding formula across a few `kernel_size`/
`dilation` combinations, using the result with a dilated `conv1d` call
to confirm the output stays the same length with no future leakage, and
a direct side-by-side against ordinary symmetric padding showing exactly
what "causal" prevents.

### Example 1

The left-padding amount scales with both `kernel_size` and `dilation`;
`kernel_size=1` has nothing to pad at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const nn::Padding1d p1 = nn::causal_padding1d(3);
  std::cout << "causal_padding1d(3): {" << p1[0] << ", " << p1[1] << "}\n";

  const nn::Padding1d p2 = nn::causal_padding1d(3, /*dilation=*/2);
  std::cout << "causal_padding1d(3, dilation=2): {" << p2[0] << ", " << p2[1] << "}\n";
  std::cout << "left padding = dilation*(kernel_size-1) = 2*2 = 4\n";

  const nn::Padding1d p3 = nn::causal_padding1d(1);
  std::cout << "causal_padding1d(1) (kernel_size=1, nothing to pad): {" << p3[0] << ", " << p3[1] << "}\n";

  return 0;
}
```

Output

```text
causal_padding1d(3): {2, 0}
causal_padding1d(3, dilation=2): {4, 0}
left padding = dilation*(kernel_size-1) = 2*2 = 4
causal_padding1d(1) (kernel_size=1, nothing to pad): {0, 0}
```

### Example 2

A dilated (`dilation=2`), causally-padded `conv1d` call keeps the output
the same length as the input, with each output position's window
correctly spanning only earlier (or equal) positions.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // kernel_size=2, dilation=2 -- effective receptive field spans 3 positions
  const nn::Padding1d pad = nn::causal_padding1d(2, /*dilation=*/2);
  std::cout << "causal_padding1d(2, dilation=2): {" << pad[0] << ", " << pad[1] << "}\n";

  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {1, 1, 5});
  const pluss::Series weight = pluss::make_series({1.0, 1.0}, {1, 1, 2});

  const pluss::Series y = nn::conv1d(input, weight, pluss::series::undefined(), pad,
      /*stride=*/1, /*dilation=*/2);

  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2]
            << "] -- same length as input\n";
  std::cout << "output: " << y << "\n";
  std::cout << "output[0]=1 (position -2 is left-pad zero, dilated window sees only input[0])\n";

  return 0;
}
```

Output

```text
causal_padding1d(2, dilation=2): {2, 0}
output shape: [1, 1, 5] -- same length as input
output: pluss::Series(shape=[1,1,5], strides=[5,1,1], dtype=float64, platform=cpu, numel=5, contiguous=false)
data=[[[1, 2, 4, 6, 8]]]
output[0]=1 (position -2 is left-pad zero, dilated window sees only input[0])
```

### Example 3

The same kernel, same input, once with `causal_padding1d`'s left-only
padding and once with ordinary symmetric `{1, 1}` padding -- the causal
form's `output[0]` depends only on `input[0]`; the symmetric form's
`output[0]` also pulls in `input[1]`, a position that would not exist
yet at inference time in a genuinely autoregressive setting.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 1, 4});
  const pluss::Series weight = pluss::make_series({1.0, 1.0, 1.0}, {1, 1, 3});

  // causal padding: output[0] must depend only on input[0] (padded with
  // two leading zeros), never on input[1] (a future position)
  const nn::Padding1d causal_pad = nn::causal_padding1d(3);
  const pluss::Series causal_out = nn::conv1d(input, weight, pluss::series::undefined(), causal_pad);

  // ordinary symmetric "same" padding {1,1}: output[0] DOES depend on
  // input[1], a position in the future relative to position 0
  const nn::Padding1d same_pad{1, 1};
  const pluss::Series same_out = nn::conv1d(input, weight, pluss::series::undefined(), same_pad);

  std::cout << "causal output[0] (only input[0] contributes): " << causal_out.reshape({4}).slice(0, 0, 1) << "\n";
  std::cout << "same-pad output[0] (input[0]+input[1] contribute -- peeks ahead): "
            << same_out.reshape({4}).slice(0, 0, 1) << "\n";

  return 0;
}
```

Output

```text
causal output[0] (only input[0] contributes): pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
same-pad output[0] (input[0]+input[1] contribute -- peeks ahead): pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3]
```

## See Also

- [`conv1d`](conv1d.md) -- the only intended consumer of this function's
  result, as its `Padding1d`-form `padding` argument.
