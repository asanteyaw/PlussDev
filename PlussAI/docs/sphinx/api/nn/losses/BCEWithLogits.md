# `pluss::ai::nn::losses::BCEWithLogits`

Binary cross-entropy computed directly from logits, with an optional
positive-class weight and per-element weight.

## Signature

```cpp
Series nn::losses::BCEWithLogits(const Series& logits, const Series& target,
                                  const nn::losses::BCEWithLogitsOptions& options = {});
```

## What It Does

Computes the binary cross-entropy between `target` and `sigmoid(logits)`
-- but never actually evaluates `sigmoid` or `log` directly on
`logits`, computing an algebraically equivalent, numerically stable
expression built from `softplus` instead, avoiding the
`log(0)`/overflow issues a naive `-target*log(sigmoid(x)) -
(1-target)*log(1-sigmoid(x))` would hit for large `|logits|`). The
result is elementwise-equal to that naive formula everywhere it is
numerically well-behaved (see Example 1), and additionally correct where
it is not. `target` need not be exactly `0`/`1` -- any value in `[0, 1]`
(a soft label) is accepted, since nothing in the formula requires an
integer target.

## Parameters

`const Series& logits`
: The model's raw (pre-sigmoid) output. Any shape.

`const Series& target`
: The label, same shape as `logits`. Ordinarily `0`/`1`, but any value
  in `[0, 1]` is accepted.

`const BCEWithLogitsOptions& options`
: See [`BCEWithLogitsOptions`](BCEWithLogitsOptions.md).

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a `Series`
the same shape as `logits` when `None`.

## Errors

- `logits`/`target` undefined, mismatched shapes, a non-float dtype, or
  different platforms -- throws.
- `options.weight`/`options.positive_class_weight`, when defined, on a
  different platform than `logits`, or a non-float dtype -- throws.

## Practical Examples

Three examples: a hand-verified per-element result cross-checked against
the naive sigmoid-based formula directly, `positive_class_weight`
scaling a hard positive example's loss by an exact factor, and confirming
that scaling touches only the positive-class element in a mixed batch,
leaving the negative-class element untouched.

### Example 1

Three logit/target pairs, cross-checked against the ordinary (numerically
fragile) sigmoid-based binary cross-entropy formula computed by hand --
`BCEWithLogits` itself never evaluates that formula directly, but the two
agree everywhere it is well-behaved.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <cmath>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series({0.0, 2.0, -2.0});
  const pluss::Series target = pluss::make_series({1.0, 0.0, 1.0});

  nn::losses::BCEWithLogitsOptions opts{};
  opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::BCEWithLogits(logits, target, opts);

  std::cout << "per-element BCEWithLogits: " << per_element << "\n";

  // manual check via the ordinary (numerically fragile) BCE formula, for
  // comparison only -- BCEWithLogits itself never computes sigmoid()
  // directly for exactly this reason
  auto sigmoid = [](double x) { return 1.0 / (1.0 + std::exp(-x)); };
  auto bce = [&](double logit, double t) {
    const double p = sigmoid(logit);
    return -(t * std::log(p) + (1.0 - t) * std::log(1.0 - p));
  };
  std::cout << "manual sigmoid-based BCE: [" << bce(0.0, 1.0) << ", " << bce(2.0, 0.0) << ", " << bce(-2.0, 1.0) << "]\n";

  return 0;
}
```

Output

```text
per-element BCEWithLogits: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.69314718, 2.126928, 2.126928]
manual sigmoid-based BCE: [0.693147, 2.12693, 2.12693]
```

### Example 2

A hard positive example (`target=1`, but `logits` strongly predicts
"negative") with `positive_class_weight=5` produces exactly `5x` the
unweighted loss; `weight=0.1` on the same example instead scales it down
to exactly `0.1x` -- the two fields act independently, and each scales
linearly and exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series({-2.0});   // predicts "negative" strongly
  const pluss::Series target = pluss::make_series({1.0});    // but the true label is positive -- a hard positive

  const pluss::Series baseline = nn::losses::BCEWithLogits(logits, target);
  std::cout << "baseline (no pos_weight): " << baseline.item<double>() << "\n";

  nn::losses::BCEWithLogitsOptions opts{};
  opts.positive_class_weight = pluss::make_series({5.0});
  const pluss::Series weighted = nn::losses::BCEWithLogits(logits, target, opts);
  std::cout << "positive_class_weight=5:  " << weighted.item<double>()
            << " -- exactly 5x the baseline, since target=1 activates the weight fully\n";

  // per-element weight scales the final per-element loss multiplicatively,
  // regardless of target
  nn::losses::BCEWithLogitsOptions per_elem_opts{};
  per_elem_opts.weight = pluss::make_series({0.1});
  const pluss::Series down_weighted = nn::losses::BCEWithLogits(logits, target, per_elem_opts);
  std::cout << "weight=0.1:               " << down_weighted.item<double>()
            << " -- exactly 0.1x the baseline\n";

  return 0;
}
```

Output

```text
baseline (no pos_weight): 2.12693
positive_class_weight=5:  10.6346 -- exactly 5x the baseline, since target=1 activates the weight fully
weight=0.1:               0.212693 -- exactly 0.1x the baseline
```

### Example 3

In a batch with one `target=0` and one `target=1` element,
`positive_class_weight` scales only the `target=1` element -- the
`target=0` element's loss is completely unaffected, confirming
`positive_class_weight` genuinely targets the positive class only, not
every element in the batch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series({2.0, -2.0});
  const pluss::Series target = pluss::make_series({0.0, 1.0});  // one negative example, one positive example

  const pluss::Series baseline = nn::losses::BCEWithLogits(logits, target,
      nn::losses::BCEWithLogitsOptions{.reduction = nn::LossReduction::None});

  nn::losses::BCEWithLogitsOptions opts{};
  opts.reduction = nn::LossReduction::None;
  opts.positive_class_weight = pluss::make_series({5.0});
  const pluss::Series weighted = nn::losses::BCEWithLogits(logits, target, opts);

  std::cout << "baseline:                " << baseline << "\n";
  std::cout << "positive_class_weight=5: " << weighted << "\n";
  std::cout << "only the target=1 (positive) element scales by 5x -- the target=0 element is untouched\n";

  return 0;
}
```

Output

```text
baseline:                pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.126928, 2.126928]
positive_class_weight=5: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.126928, 10.63464]
only the target=1 (positive) element scales by 5x -- the target=0 element is untouched
```

## See Also

- [`BCEWithLogitsOptions`](BCEWithLogitsOptions.md) -- the options
  struct.
- [`CrossEntropy`](CrossEntropy.md) -- the multi-class generalization
  (softmax cross-entropy) of the binary case this function covers.
