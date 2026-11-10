# `pluss::ai::classic::resolve_weight_plan` / `resolve_training_weights`

Turns [`TrainOptions`](../core/TrainOptions.md)' weighting fields
(`sample_weights`, `class_weight`, `class_weights`) plus a batch's own
`sample_weights` into one final, mean-normalized per-row weight vector —
what every estimator's `learn()` actually multiplies its loss by
internally. `resolve_weight_plan` returns the full
[`WeightPlan`](#returns) (every intermediate quantity, for inspection);
`resolve_training_weights` is a convenience narrowing to just the final
`weights` vector, calling `resolve_weight_plan` internally and reading
one field back.

## Signature

```cpp
classic::StatusOr<classic::WeightPlan> classic::resolve_weight_plan(
    const pluss::Series& labels,
    const pluss::Series& batch_sample_weights,
    const classic::TrainOptions& options);

classic::StatusOr<pluss::Series> classic::resolve_training_weights(
    const pluss::Series& labels,
    const pluss::Series& batch_sample_weights,
    const classic::TrainOptions& options);
```

## What It Does

Combines every source of per-row weighting -- a batch's own
`sample_weights`, `TrainOptions`' class-weighting fields, and the class
distribution `class_mix` computes from `labels` -- into one final,
mean-normalized weight vector, one entry per row. `resolve_weight_plan`
hands back every intermediate quantity that went into that computation, for
inspection or debugging; `resolve_training_weights` skips straight to the
final vector when the intermediates aren't needed.

## Parameters

`labels`
: Passed straight to [`class_mix`](class_mix.md) — same contract
  (defined, non-empty, finite).

`batch_sample_weights`
: Optional, `[n_samples]` — typically a
  [`LabeledBatch::sample_weights`](../data/LabeledBatch.md) field.
  `pluss::series::undefined()` (the common case) means every row starts at
  weight `1.0` before anything else is applied. If defined: length must
  match `labels`, every value finite and strictly positive.

`options` (`TrainOptions`)
: Only three fields are read: `options.sample_weights` (a **second**,
  independent optional per-row weight source, same validity contract as
  `batch_sample_weights` — see [Example 4](#example-4) for both combined),
  `options.class_weight` (`None`/`Balanced`/`Custom` — see `## Returns`
  below), and `options.class_weights` (consulted only when
  `class_weight == Custom`).

## Returns

`resolve_weight_plan` returns a `StatusOr<WeightPlan>`:

### `classic::WeightPlan` fields

| Field | Type | Default |
|---|---|---|
| `weights` | `pluss::Series` | — |
| `class_weights` | `pluss::Series` | — |
| `sample_weights` | `pluss::Series` | — |
| `mix` | `classic::ClassMix` | — |
| `mean_weight` | `double` | — |

The final `weights` is always computed as `batch_sample_weights *
options.sample_weights * class_weight[row's class]`, then divided by its
own mean — so `weights.mean()` is always exactly `1.0`, regardless of
what any of the three inputs were. `class_weights` resolves by
`options.class_weight`:

- `None` (the default) — every class weighted `1.0`.
- `Balanced` — `class_weights` is exactly
  [`ClassMix::inverse_frequency`](ClassMix.md#state) — see
  [Example 2](#example-2).
- `Custom` — `class_weights` is `options.class_weights` verbatim, after
  validating its length and values.

`resolve_training_weights` returns just `weights` above, as a
`StatusOr<pluss::Series>` — see [Example 5](#example-5) for the exact
equivalence.

## Errors

- Everything [`class_mix`](class_mix.md#errors) can already fail with,
  applied to `labels`.
- `batch_sample_weights` (or `options.sample_weights`) defined with a
  length not matching `labels`, a non-finite value, or a
  non-strictly-positive value — `Status::shape_mismatch` /
  `Status::invalid_argument`.
- `options.class_weight == Custom` and `options.class_weights` is
  undefined — `Status::invalid_argument`.
- `options.class_weights` defined with a length not matching the
  detected class count, a non-finite value, or a non-positive value —
  `Status::shape_mismatch` / `Status::invalid_argument`.

## Practical Examples

The default (no weighting at all) lifecycle, `Balanced` matched by hand
against `inverse_frequency`, `Custom` per-class weights applied per row,
`batch_sample_weights` and `options.sample_weights` both combined by
multiplication, the `resolve_training_weights` narrowing proven
identical, the always-mean-`1.0` normalization fact, and an error
roundup covering both weight sources and both class-weight failure
modes.

### Example 1

No weighting configured at all — every row ends at exactly `1.0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), cls::TrainOptions{}).value();
  std::cout << "weights = " << plan.weights << "\n";
  std::cout << "class_weights = " << plan.class_weights << "\n";
  std::cout << "mean_weight = " << plan.mean_weight << "\n";

  return 0;
}
```

Output

```text
weights = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 1]
class_weights = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
mean_weight = 1
```

### Example 2

`ClassWeightMode::Balanced` — `class_weights` matches
[`ClassMix::inverse_frequency`](ClassMix.md#state) exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 6 rows, class 1: 2 rows.
  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Balanced;
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value();
  std::cout << "class_weights = " << plan.class_weights << "\n";
  std::cout << "mix.inverse_frequency (expect identical) = " << plan.mix.inverse_frequency << "\n";
  std::cout << "weights (minority rows weigh more) = " << plan.weights << "\n";

  return 0;
}
```

Output

```text
class_weights = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 2]
mix.inverse_frequency (expect identical) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 2]
weights (minority rows weigh more) = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0.66666667, 0.66666667, 0.66666667, 0.66666667, 0.66666667, 0.66666667, 2, 2]
```

### Example 3

`ClassWeightMode::Custom` — explicit per-class weights, applied per row
by that row's own class.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Custom;
  options.class_weights = pluss::make_series({1.0, 5.0}, {2}, f64);  // class 0 -> 1.0, class 1 -> 5.0
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value();
  std::cout << "weights (final, mean-normalized -- raw ratios were [1,1,5,5]) = " << plan.weights << "\n";

  return 0;
}
```

Output

```text
weights (final, mean-normalized -- raw ratios were [1,1,5,5]) = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.33333333, 0.33333333, 1.6666667, 1.6666667]
```

### Example 4

Both `batch_sample_weights` and `options.sample_weights` supplied
together — combined by multiplication, before class weighting is ever
applied.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series batch_weights = pluss::make_series({2.0, 2.0, 2.0, 2.0}, {4}, f64);

  cls::TrainOptions options{};
  options.sample_weights = pluss::make_series({1.0, 3.0, 1.0, 3.0}, {4}, f64);
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, batch_weights, options).value();
  std::cout << "sample_weights (2*[1,3,1,3] = [2,6,2,6]) = " << plan.sample_weights << "\n";

  return 0;
}
```

Output

```text
sample_weights (2*[1,3,1,3] = [2,6,2,6]) = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2, 6, 2, 6]
```

### Example 5

`resolve_training_weights` is exactly `resolve_weight_plan(...).weights`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 0.0, 1.0}, {4}, f64);
  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Balanced;

  const pluss::Series via_plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value().weights;
  const pluss::Series via_narrow = cls::resolve_training_weights(labels, pluss::series::undefined(), options).value();
  const double diff = (via_plan - via_narrow).abs().amax(0, false).item<double>();
  std::cout << "max abs diff = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff = 0
```

### Example 6

`weights.mean()` is always exactly `1.0`, no matter how skewed the
inputs — `mean_weight` reports the pre-normalization scale that was
divided out.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Custom;
  options.class_weights = pluss::make_series({1.0, 20.0}, {2}, f64);

  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value();
  std::cout << "mean_weight (pre-normalization mean) = " << plan.mean_weight << "\n";
  std::cout << "weights.mean() (always 1.0) = " << plan.weights.mean().item<double>() << "\n";

  return 0;
}
```

Output

```text
mean_weight (pre-normalization mean) = 5.75
weights.mean() (always 1.0) = 1
```

### Example 7

An error roundup: a length-mismatched `batch_sample_weights`, a
non-positive `options.sample_weights` value, `Custom` with
`class_weights` left undefined, and a length-mismatched
`options.class_weights`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  const pluss::Series wrong_length = pluss::make_series({1.0, 1.0}, {2}, f64);
  std::cout << "length-mismatched batch_sample_weights ok = "
            << std::boolalpha << cls::resolve_weight_plan(labels, wrong_length, cls::TrainOptions{}).ok() << "\n";

  cls::TrainOptions zero_weight{};
  zero_weight.sample_weights = pluss::make_series({1.0, 0.0, 1.0, 1.0}, {4}, f64);
  std::cout << "non-positive options.sample_weights ok = "
            << cls::resolve_weight_plan(labels, pluss::series::undefined(), zero_weight).ok() << "\n";

  cls::TrainOptions missing_custom{};
  missing_custom.class_weight = cls::ClassWeightMode::Custom;
  std::cout << "Custom with class_weights undefined ok = "
            << cls::resolve_weight_plan(labels, pluss::series::undefined(), missing_custom).ok() << "\n";

  cls::TrainOptions wrong_custom_length{};
  wrong_custom_length.class_weight = cls::ClassWeightMode::Custom;
  wrong_custom_length.class_weights = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  std::cout << "Custom class_weights length mismatch ok = "
            << cls::resolve_weight_plan(labels, pluss::series::undefined(), wrong_custom_length).ok() << "\n";

  return 0;
}
```

Output

```text
length-mismatched batch_sample_weights ok = false
non-positive options.sample_weights ok = false
Custom with class_weights undefined ok = false
Custom class_weights length mismatch ok = false
```

## Changelog

- **0.1** — `resolve_weight_plan`, `resolve_training_weights`,
  `WeightPlan`.

## See Also

- [`ClassMix`](ClassMix.md) — embedded in `WeightPlan::mix`.
- [`TrainOptions`](../core/TrainOptions.md) — `sample_weights`,
  `class_weight`, `class_weights`, the three fields this function reads.
- [`plan_balance`](plan_balance.md) — the resampling/synthesis
  counterpart; rebalances rows instead of reweighting them.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
