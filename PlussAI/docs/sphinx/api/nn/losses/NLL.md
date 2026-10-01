# `pluss::ai::nn::losses::NLL`

Negative log-likelihood from log-probabilities and class-index targets.

## Signature

```cpp
Series nn::losses::NLL(const Series& log_probs, const Series& target,
                        const nn::losses::NLLOptions& options = {});
```

## What It Does

Gathers, for each element, `log_probs` at `target`'s own class index
along `options.class_dim`, negates it, then reduces per
`options.reduction`. `log_probs` must already be log-probabilities --
typically the direct output of a `log_softmax` call -- this function
performs no softmax or normalization of its own; feeding it raw
(unnormalized) logits computes something other than negative
log-likelihood. `target` holds integer class indices (`Int32`/`Int64`),
not one-hot or soft-probability vectors -- for the latter, see
[`CrossEntropy`](CrossEntropy.md), which accepts both target kinds and
delegates to this function for the class-index case when
`label_smoothing == 0`.

## Parameters

`const Series& log_probs`
: Log-probabilities, any rank `>= 1`. The axis named by
  `options.class_dim` holds `num_classes` values.

`const Series& target`
: Integer class indices, rank exactly one less than `log_probs`, every
  other axis matching `log_probs`'s own.

`const NLLOptions& options`
: See [`NLLOptions`](NLLOptions.md).

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a
`Series` shaped like `target` when `None`.

## Errors

- `log_probs`/`target` undefined, a non-index target dtype,
  `class_dim` out of range, or a `target` shape that does not match
  `log_probs` (minus the class axis) -- throws.
- `options.class_weight`, when defined, not rank-1, a size mismatch
  against `num_classes`, or a platform mismatch -- throws.

## Practical Examples

Four examples: a hand-verified basic gather-and-negate result, the
weighted-mean arithmetic `class_weight` actually performs (dividing by
the sum of gathered weights, not the raw element count), `ignore_index`
zeroing out specific elements and excluding them from the reduction's
denominator, and `class_dim` set to something other than the last axis.

### Example 1

A `[batch=2, classes=3]` `log_probs` tensor -- each row's loss is
exactly the negative of its own target class's log-probability, nothing
else from that row is consulted.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // batch=2, 3 classes -- already log-probabilities (e.g. from log_softmax)
  const pluss::Series log_probs = pluss::make_series(
      {-0.1, -2.0, -3.0,
       -1.5, -0.3, -2.5},
      {2, 3});
  const pluss::Series target = pluss::make_series({0LL, 1LL}, {2}).to(pluss::ScalarType::Int64);

  nn::losses::NLLOptions none_opts{};
  none_opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::NLL(log_probs, target, none_opts);
  std::cout << "per-element NLL (-log_probs[target]): " << per_element << "\n";
  std::cout << "row 0 picks class 0 (-(-0.1)=0.1), row 1 picks class 1 (-(-0.3)=0.3)\n";

  const pluss::Series loss_mean = nn::losses::NLL(log_probs, target);
  std::cout << "NLL (mean, default): " << loss_mean.item<double>() << " -- (0.1+0.3)/2 = 0.2\n";

  return 0;
}
```

Output

```text
per-element NLL (-log_probs[target]): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.1, 0.3]
row 0 picks class 0 (-(-0.1)=0.1), row 1 picks class 1 (-(-0.3)=0.3)
NLL (mean, default): 0.2 -- (0.1+0.3)/2 = 0.2
```

### Example 2

With `class_weight=[2, 1, 1]`, row 0's loss (class 0) doubles before the
reduction, and the `Mean` reduction divides by the **sum of the gathered
weights** (`2 + 1 = 3`), not by the raw element count (`2`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series log_probs = pluss::make_series(
      {-0.1, -2.0, -3.0,
       -1.5, -0.3, -2.5},
      {2, 3});
  const pluss::Series target = pluss::make_series({0LL, 1LL}, {2}).to(pluss::ScalarType::Int64);

  nn::losses::NLLOptions opts{};
  opts.class_weight = pluss::make_series({2.0, 1.0, 1.0});  // double-weight class 0
  const pluss::Series loss = nn::losses::NLL(log_probs, target, opts);

  std::cout << "NLL with class_weight=[2,1,1]: " << loss.item<double>() << "\n";
  std::cout << "row 0 (class 0, weight 2): 0.1*2=0.2; row 1 (class 1, weight 1): 0.3*1=0.3\n";
  std::cout << "weighted mean = (0.2+0.3) / (2+1) = 0.5/3 = 0.16667 -- divides by the SUM OF WEIGHTS, not the raw count\n";

  return 0;
}
```

Output

```text
NLL with class_weight=[2,1,1]: 0.166667
row 0 (class 0, weight 2): 0.1*2=0.2; row 1 (class 1, weight 1): 0.3*1=0.3
weighted mean = (0.2+0.3) / (2+1) = 0.5/3 = 0.16667 -- divides by the SUM OF WEIGHTS, not the raw count
```

### Example 3

Two of three rows have `target == 1`; setting `use_ignore_index=true`
with `ignore_index=1` zeroes those rows' losses and excludes them from
the reduction's denominator entirely -- the `Mean` result is exactly
row 0's own loss, not row 0's loss divided by 3 (or even by 1 with the
other two rows silently contributing `0` to the sum but still counted in
the denominator).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series log_probs = pluss::make_series(
      {-0.1, -2.0, -3.0,
       -1.5, -0.3, -2.5,
       -0.2, -0.4, -0.6},
      {3, 3});
  const pluss::Series target = pluss::make_series({0LL, 1LL, 1LL}, {3}).to(pluss::ScalarType::Int64);

  nn::losses::NLLOptions opts{};
  opts.use_ignore_index = true;
  opts.ignore_index = 1;  // rows whose target==1 are excluded entirely
  const pluss::Series loss = nn::losses::NLL(log_probs, target, opts);

  std::cout << "NLL with ignore_index=1 (rows 1 and 2 excluded): " << loss.item<double>()
            << " -- exactly row 0's own loss (0.1), since rows 1+2 contribute to neither the sum nor the count\n";

  nn::losses::NLLOptions none_opts{};
  none_opts.use_ignore_index = true;
  none_opts.ignore_index = 1;
  none_opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::NLL(log_probs, target, none_opts);
  std::cout << "per-element (None reduction): " << per_element << " -- rows 1+2 are zeroed, not skipped in shape\n";

  return 0;
}
```

Output

```text
NLL with ignore_index=1 (rows 1 and 2 excluded): 0.1 -- exactly row 0's own loss (0.1), since rows 1+2 contribute to neither the sum nor the count
per-element (None reduction): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.1, 0, 0] -- rows 1+2 are zeroed, not skipped in shape
```

### Example 4

A `[batch=1, classes=3, seq_len=2]` layout -- the class axis is in the
middle, not last, common for per-timestep classification. Setting
`class_dim=1` reads exactly the same values a last-axis layout would.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // [batch=1, classes=3, seq_len=2] -- class axis in the middle, common
  // for per-timestep classification (e.g. sequence tagging)
  const pluss::Series log_probs = pluss::make_series(
      {-0.1, -1.5,
       -2.0, -0.3,
       -3.0, -2.5},
      {1, 3, 2});
  // target: [batch=1, seq_len=2], one class index per timestep
  const pluss::Series target = pluss::make_series({0LL, 1LL}, {1, 2}).to(pluss::ScalarType::Int64);

  nn::losses::NLLOptions opts{};
  opts.class_dim = 1;  // classes live on axis 1, not the last axis
  const pluss::Series loss = nn::losses::NLL(log_probs, target, opts);

  std::cout << "NLL with class_dim=1: " << loss.item<double>() << " -- (0.1+0.3)/2 = 0.2, same values as the last-axis layout\n";

  return 0;
}
```

Output

```text
NLL with class_dim=1: 0.2 -- (0.1+0.3)/2 = 0.2, same values as the last-axis layout
```

## See Also

- [`NLLOptions`](NLLOptions.md) -- the options struct.
- [`CrossEntropy`](CrossEntropy.md) -- computes `log_softmax` internally
  first (so raw logits, not pre-computed log-probabilities, are the
  input), then delegates directly to this function for the
  class-index-target, `label_smoothing=0` case.
