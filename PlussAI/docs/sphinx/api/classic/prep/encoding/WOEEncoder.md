# `pluss::ai::classic::prep::WOEEncoder`

Weight-of-evidence encoding for a **binary `0`/`1` target**: replace each category with the log-odds of it
being associated with the positive class relative to the whole dataset —
`ln(((pos_in_category + reg) / (total_pos + 2·reg)) / ((neg_in_category +
reg) / (total_neg + 2·reg)))`, a Laplace-regularized log ratio. Positive
values mean the category is over-represented among positives relative to
the base rate; negative values mean it is under-represented; `0` means
either exactly the base rate or (see `## Methods`) a degenerate ratio this
encoder declines to take the log of. This is a **supervised** encoder — it
needs a strictly binary target at fit time — the standard feature-encoding
choice in credit-scoring-style modeling, where the log-odds scale composes
naturally with a logistic model's own linear-in-log-odds form.

## Signature

```cpp
explicit classic::prep::WOEEncoder(classic::prep::WOEEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`WOEEncoderOptions`](WOEEncoderOptions.md). Inherits its entire public
interface from `SupervisedEncoderBase` — `WOEEncoder` itself adds only the
constructor and the internal fitting formula.

## State

The fitted per-feature category list and matching WOE values live inside
the object, set by `learn_supervised`, read back only through `map` /
`dump_state`. `trained()` reports whether fitting has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn_supervised` has succeeded at least once.

`needs_target()` — `bool`
: `true`, always — see [`TargetEncoder::needs_target`](TargetEncoder.md#methods)
  for what this enables inside a `Flow`.

`learn(batch, options = {})`
: **Always fails** with `Status::invalid_argument` — call
  `learn_supervised` instead.

`learn_supervised(batch, target, options = {})`
: Fits a vocabulary per feature the same way
  [`OneHotEncoder::learn`](OneHotEncoder.md) does, then requires every
  non-`NaN` target value to be exactly `0.0` or `1.0` — any other value
  fails the call outright, before any per-category computation. Per fitted
  category, computes the regularized log-odds ratio
  above; if either the numerator or the denominator is not strictly
  positive (only possible when `regularization = 0` and a category is
  purely positive or purely negative), the fitted value is `0.0` instead of
  `ln(0)`/`ln(inf)`. The unseen-category fallback is always `0.0` (neutral —
  "no evidence either way"), never a computed prior the way
  [`TargetEncoder`](TargetEncoder.md) uses.

`map(batch, options = {})`
: Replaces each fitted category with its computed WOE value. As with
  `TargetEncoder`, **there is no `unknown_categorical`-driven branch** — an
  unseen or `overflow`-dropped category always maps to `0.0`.

`learn_map(batch, ...)`
: **Always fails**, the same as `learn`. Use `learn_supervised` then `map`
  separately.

`dump_state()` / `load_state(state)`
: `[metadata, key_sizes, flat_keys, flat_values, unknown_values]`, shared
  verbatim with [`TargetEncoder`](TargetEncoder.md).

## Errors

- The target has a non-`NaN` value other than `0.0` or `1.0` —
  `Status::invalid_argument`, `"WOEEncoder requires a binary 0/1 target."`,
  from `learn_supervised`.
- `max_categories < 0` — `Status::invalid_argument` from `learn_supervised`.
- `overflow = Error` and a feature's cardinality exceeds `max_categories` —
  `Status::invalid_argument` from `learn_supervised`.
- `target.numel()` not equal to the batch's sample count —
  `Status::shape_mismatch` from `learn_supervised`.
- `learn(batch)` / `learn_map(batch, ...)` called directly (no target) —
  `Status::invalid_argument` naming `learn_supervised` as the required call.
- `map` before a successful `learn_supervised` — `Status::precondition`.

## Practical Examples

The log-odds formula on a category skewed toward the positive class versus
one skewed toward the negative, the rejection of a non-binary target, and
`regularization` softening an otherwise-degenerate all-positive category.

### Example 1

Category 1 is all-positive, category 2 all-negative — opposite-signed WOE
values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1: targets 1, 1 (both positive). category 2: targets 0, 0 (both negative).
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64).reshape({4, 1});
  const pluss::Series target = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);

  cls::prep::WOEEncoder encoder;
  encoder.learn_supervised(train, target);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  std::cout << "map([category 1, category 2]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([category 1, category 2]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1.0986123],
  [-1.0986123]]
```

### Example 2

A target with a value other than `0`/`1` is rejected outright, before any
per-category work.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  const pluss::Series target = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64);  // 2.0 is not binary

  cls::prep::WOEEncoder encoder;
  const cls::Status fit = encoder.learn_supervised(train, target);
  std::cout << "ok = " << std::boolalpha << fit.ok() << "\n";
  std::cout << "message = \"" << fit.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "WOEEncoder requires a binary 0/1 target."
```

### Example 3

`regularization` softens an all-positive category's otherwise-undefined
ratio — with `regularization = 0` the category is purely positive
(`denominator = 0`) and the fitted value falls back to `0.0`; a positive
`regularization` gives it a large-but-finite value instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1 is purely positive; category 2 provides the only negative rows.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64).reshape({4, 1});
  const pluss::Series target = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});

  cls::prep::WOEEncoderOptions zero_reg{};
  zero_reg.regularization = 0.0;
  cls::prep::WOEEncoder degenerate(zero_reg);
  degenerate.learn_supervised(train, target);
  std::cout << "regularization=0:   " << degenerate.map(probe).value() << "\n";

  cls::prep::WOEEncoderOptions with_reg{};
  with_reg.regularization = 0.5;
  cls::prep::WOEEncoder regularized(with_reg);
  regularized.learn_supervised(train, target);
  std::cout << "regularization=0.5: " << regularized.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
regularization=0:   pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
regularization=0.5: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1.6094379]]
```

## Changelog

- **0.1** — `learn_supervised`/`map` (via `SupervisedEncoderBase`),
  `regularization`/`max_categories`/`overflow`, `dump_state`/`load_state`.

## See Also

- [`WOEEncoderOptions`](WOEEncoderOptions.md) — the options struct.
- [`TargetEncoder`](TargetEncoder.md) — the any-target alternative, prior
  (not `0.0`) as the unseen-category fallback.
- [`compose::Flow::learn`](../../compose/Flow/learn.md) — how a supervised
  encoder stage actually receives its target in practice.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

WOEEncoderOptions
```
<!-- apiref:toctree:end -->
