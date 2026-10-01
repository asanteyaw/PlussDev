# `pluss::ai::nn::losses::MSE`

Mean squared error between prediction and target.

## Signature

```cpp
Series nn::losses::MSE(const Series& prediction, const Series& target,
                        const nn::losses::MSEOptions& options = {});
```

## What It Does

Computes `(prediction - target)^2` elementwise, then reduces it according
to `options.reduction` (see [`MSEOptions`](MSEOptions.md)). `prediction`
and `target` must have the exact same shape -- there is no broadcasting
here, and the reduction (when `Mean` or `Sum`) runs over every element in
that shape, not per-row or per-batch-item; a `[batch, features]` input
reduces across both axes together, not just `features`. Both must be a
training-float dtype (Float32/Float64/BFloat16/Float16) and share a
platform.

## Parameters

`const Series& prediction`
: The model's output. Any shape; must match `target`'s shape exactly.

`const Series& target`
: The ground-truth value. Same shape as `prediction`.

`const MSEOptions& options`
: See [`MSEOptions`](MSEOptions.md). Defaulted -- omit entirely for the
  common `Mean`-reduced case.

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a `Series`
the same shape as `prediction` when `None`.

## Errors

- `prediction`/`target` undefined, mismatched shapes, a non-float dtype,
  or different platforms -- throws.

## Practical Examples

Three examples: the three reduction modes compared against a hand-worked
formula, the same call on a 2-D batch confirming the reduction runs over
every element rather than per row, and reading `MSEOptions::reduction`
back through aggregate initialization.

### Example 1

Three predictions against three targets, reduced three different ways.
`Mean` is `Sum` divided by the element count (`3` here).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::Series target = pluss::make_series({1.5, 2.0, 2.0});

  const pluss::Series loss_mean = nn::losses::MSE(prediction, target);
  std::cout << "MSE (mean, default): " << loss_mean.item<double>() << "\n";

  nn::losses::MSEOptions sum_opts{};
  sum_opts.reduction = nn::LossReduction::Sum;
  const pluss::Series loss_sum = nn::losses::MSE(prediction, target, sum_opts);
  std::cout << "MSE (sum): " << loss_sum.item<double>() << "\n";

  nn::losses::MSEOptions none_opts{};
  none_opts.reduction = nn::LossReduction::None;
  const pluss::Series loss_none = nn::losses::MSE(prediction, target, none_opts);
  std::cout << "MSE (none, per-element): " << loss_none << "\n";

  return 0;
}
```

Output

```text
MSE (mean, default): 0.416667
MSE (sum): 1.25
MSE (none, per-element): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.25, 0, 1]
```

The three per-element errors are `(1-1.5)^2=0.25`, `(2-2)^2=0`,
`(3-2)^2=1` -- summing gives `1.25`, and dividing by `3` gives the
`0.41666...` the `Mean` (default) reduction reports.

### Example 2

A `[2, 3]` batch -- `Mean` divides by all `6` elements together, not by
`3` per row and then averaged again, confirmed against a manually
computed `mean((prediction - target)^2)` over the whole tensor.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // a "batch" of 2 rows, 3 features each
  const pluss::Series prediction = pluss::make_series({1.0, 2.0, 3.0, 0.0, 0.0, 0.0}, {2, 3});
  const pluss::Series target = pluss::make_series({1.5, 2.0, 2.0, 1.0, 1.0, 1.0}, {2, 3});

  const pluss::Series loss = nn::losses::MSE(prediction, target);
  std::cout << "shape: [" << prediction.sizes()[0] << ", " << prediction.sizes()[1] << "]\n";
  std::cout << "MSE (mean over all 6 elements, not per-row): " << loss.item<double>() << "\n";

  const pluss::Series manual_mean = prediction.sub(target).square().mean();
  std::cout << "manual mean((pred-target)^2) over all elements: " << manual_mean.item<double>() << "\n";

  return 0;
}
```

Output

```text
shape: [2, 3]
MSE (mean over all 6 elements, not per-row): 0.708333
manual mean((pred-target)^2) over all elements: 0.708333
```

### Example 3

`MSEOptions` aggregate-initializes directly at the call site -- no need
to spell out a named variable first for the common case of overriding
just `reduction`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({2.0, 4.0});
  const pluss::Series target = pluss::make_series({0.0, 0.0});

  const pluss::Series loss = nn::losses::MSE(prediction, target, {nn::LossReduction::Sum});
  std::cout << "MSE (sum, via brace-init options): " << loss.item<double>() << "\n";

  return 0;
}
```

Output

```text
MSE (sum, via brace-init options): 20
```

## See Also

- [`MSEOptions`](MSEOptions.md) -- the options struct.
- [`RelativeMSE`](RelativeMSE.md) -- the same squared-error formula,
  additionally scaled by a caller-supplied divisor.
- [`Huber`](Huber.md) / [`SmoothL1`](SmoothL1.md) -- regression losses
  that grow linearly rather than quadratically for large errors, less
  sensitive to outliers than `MSE`.
