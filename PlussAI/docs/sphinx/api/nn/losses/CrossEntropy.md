# `pluss::ai::nn::losses::CrossEntropy`

Softmax cross-entropy from logits, accepting either class-index or
soft-probability targets, with label smoothing.

## Signature

```cpp
Series nn::losses::CrossEntropy(const Series& logits, const Series& target,
                                 const nn::losses::CrossEntropyOptions& options = {});
```

## What It Does

Computes `log_softmax(logits, options.class_dim)` internally, then
combines it with `target` one of two ways, decided entirely by
`target`'s own dtype:

- **Class-index target** (an integer dtype): with `label_smoothing == 0`,
  delegates directly to [`NLL`](NLL.md) with the computed log-probabilities
  -- bit-for-bit the same result as calling `NLL` by hand (see Example
  1). With `label_smoothing > 0`, blends that `NLL` result with a
  uniform-distribution term instead (see
  [`label_smoothing`](CrossEntropyOptions.md#label_smoothing) and Example
  3).
- **Soft-probability target** (a training-float dtype, same shape as
  `logits`): computes `-sum(target * log_softmax(logits), class_dim)`
  directly -- the general cross-entropy formula, with no requirement
  that `target` be one-hot. A one-hot `target` in this form produces
  exactly the same result as the equivalent class-index target (Example
  2); a genuinely soft (non-one-hot) `target` has no class-index
  equivalent at all. `use_ignore_index`/`ignore_index` are **not**
  supported in this form -- setting `use_ignore_index=true` alongside a
  soft-probability `target` throws (Example 4), since there is no single
  index value to compare against `ignore_index`.

Unlike [`BCEWithLogits`](BCEWithLogits.md), which is specifically the
two-class (binary) case, this function handles any number of classes.

## Parameters

`const Series& logits`
: The model's raw (pre-softmax) output. The axis named by
  `options.class_dim` holds `num_classes` values.

`const Series& target`
: Either integer class indices (rank one less than `logits`) or a
  soft-probability distribution (same shape and rank as `logits`) --
  which one is decided by `target`'s dtype, not by any explicit flag.

`const CrossEntropyOptions& options`
: See [`CrossEntropyOptions`](CrossEntropyOptions.md).

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a
`Series` shaped like the reduced-away class axis when `None`.

## Errors

- `logits`/`target` undefined, a `target` dtype that is neither a
  training-float nor an integer index type, or a shape mismatch for the
  target kind actually supplied -- throws.
- `options.label_smoothing` outside `[0, 1]` or non-finite -- throws.
- `options.use_ignore_index == true` together with a soft-probability
  `target` -- throws.

## Practical Examples

Four examples: confirming the class-index, no-smoothing path is
bit-for-bit identical to calling `NLL` by hand, a one-hot
soft-probability target matching its class-index equivalent exactly
(and a genuinely soft target that has no such equivalent), the
`label_smoothing` formula verified against its own manual computation,
and the `ignore_index`-with-soft-target error.

### Example 1

With `label_smoothing=0` (the default) and a class-index target, this
function computes precisely `log_softmax` followed by `NLL` -- confirmed
here by computing that same pipeline by hand and comparing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series(
      {2.0, 1.0, 0.1,
       0.5, 2.5, 0.2},
      {2, 3});
  const pluss::Series target = pluss::make_series({0LL, 1LL}, {2}).to(pluss::ScalarType::Int64);

  const pluss::Series ce = nn::losses::CrossEntropy(logits, target);

  const pluss::Series log_probs = logits.log_softmax(-1);
  const pluss::Series nll = nn::losses::NLL(log_probs, target);

  std::cout << "CrossEntropy (class-index target, label_smoothing=0): " << ce.item<double>() << "\n";
  std::cout << "NLL(log_softmax(logits), target):                     " << nll.item<double>() << "\n";
  std::cout << "identical: " << std::boolalpha << ce.allclose(nll) << "\n";

  return 0;
}
```

Output

```text
CrossEntropy (class-index target, label_smoothing=0): 0.314291
NLL(log_softmax(logits), target):                     0.314291
identical: true
```

### Example 2

A one-hot probability target produces exactly the same result as its
class-index equivalent; a genuinely soft (non-one-hot) target -- which
has no class-index form at all -- computes a different, but equally
well-defined, value.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series(
      {2.0, 1.0, 0.1,
       0.5, 2.5, 0.2},
      {2, 3});

  const pluss::Series index_target = pluss::make_series({0LL, 1LL}, {2}).to(pluss::ScalarType::Int64);
  const pluss::Series ce_index = nn::losses::CrossEntropy(logits, index_target);

  // the equivalent one-hot probability target
  const pluss::Series soft_target = pluss::make_series(
      {1.0, 0.0, 0.0,
       0.0, 1.0, 0.0},
      {2, 3});
  const pluss::Series ce_soft = nn::losses::CrossEntropy(logits, soft_target);

  std::cout << "class-index target:      " << ce_index.item<double>() << "\n";
  std::cout << "one-hot probability target: " << ce_soft.item<double>() << "\n";
  std::cout << "identical: " << std::boolalpha << ce_index.allclose(ce_soft) << "\n";

  // a genuinely soft (non-one-hot) target -- no class-index equivalent exists
  const pluss::Series really_soft_target = pluss::make_series(
      {0.7, 0.2, 0.1,
       0.1, 0.8, 0.1},
      {2, 3});
  const pluss::Series ce_really_soft = nn::losses::CrossEntropy(logits, really_soft_target);
  std::cout << "genuinely soft target (0.7/0.2/0.1 mix): " << ce_really_soft.item<double>() << "\n";

  return 0;
}
```

Output

```text
class-index target:      0.314291
one-hot probability target: 0.314291
identical: true
genuinely soft target (0.7/0.2/0.1 mix): 0.724291
```

### Example 3

`label_smoothing=0.1` verified against its own defining formula,
`(1 - label_smoothing) * NLL + label_smoothing * mean(-log_probs,
class_dim)`, computed independently by hand.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series(
      {2.0, 1.0, 0.1,
       0.5, 2.5, 0.2},
      {2, 3});
  const pluss::Series target = pluss::make_series({0LL, 1LL}, {2}).to(pluss::ScalarType::Int64);

  const pluss::Series ce_no_smoothing = nn::losses::CrossEntropy(logits, target);

  nn::losses::CrossEntropyOptions opts{};
  opts.label_smoothing = 0.1;
  const pluss::Series ce_smoothed = nn::losses::CrossEntropy(logits, target, opts);

  std::cout << "label_smoothing=0.0: " << ce_no_smoothing.item<double>() << "\n";
  std::cout << "label_smoothing=0.1: " << ce_smoothed.item<double>() << "\n";

  // manual formula: (1-ls)*NLL(log_softmax(logits),target) + ls*mean(-log_probs, class_dim)
  const pluss::Series log_probs = logits.log_softmax(-1);
  const pluss::Series nll = nn::losses::NLL(log_probs, target);
  const pluss::Series smooth_per = log_probs.mean(-1, false).neg();
  const pluss::Series manual = nll.mul(0.9).add(smooth_per.mean().mul(0.1));
  std::cout << "manual (1-ls)*NLL + ls*mean(-log_probs): " << manual.item<double>() << "\n";

  return 0;
}
```

Output

```text
label_smoothing=0.0: 0.314291
label_smoothing=0.1: 0.434291
manual (1-ls)*NLL + ls*mean(-log_probs): 0.434291
```

### Example 4

`use_ignore_index=true` together with a soft-probability `target`
throws immediately -- there is no single index value in a soft target to
compare against `ignore_index` at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <string>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series({2.0, 1.0, 0.1}, {1, 3});
  const pluss::Series soft_target = pluss::make_series({1.0, 0.0, 0.0}, {1, 3});

  nn::losses::CrossEntropyOptions opts{};
  opts.use_ignore_index = true;
  opts.ignore_index = 0;

  try {
    const pluss::Series ce = nn::losses::CrossEntropy(logits, soft_target, opts);
    std::cout << "should not reach here: " << ce.item<double>() << "\n";
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "ignore_index with a soft-probability target throws: "
              << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
ignore_index with a soft-probability target throws: pluss::nn::losses::CrossEntropy: ignore_index is only valid for class-index targets
```

## See Also

- [`CrossEntropyOptions`](CrossEntropyOptions.md) -- the options struct.
- [`NLL`](NLL.md) -- what this function delegates to directly for
  class-index targets with no label smoothing.
- [`BCEWithLogits`](BCEWithLogits.md) -- the two-class specialization.
