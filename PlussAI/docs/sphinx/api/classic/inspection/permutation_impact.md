# `pluss::ai::classic::permutation_impact`

Per-feature model-agnostic importance: fits one model from `spec`, then for
each feature, repeatedly shuffles that feature's own column on held-out
data and measures how much the chosen metric gets worse — a feature whose
values carry real signal hurts the model when scrambled; a feature the
model ignores does not. Unlike [`rank_features`](../features/rank_features.md)
(a pre-training, label-correlation-only score), this measures the
**fitted model's own** sensitivity to each feature, after training.

## Signature

```cpp
classic::StatusOr<classic::PermuteImpactReport> classic::permutation_impact(
    const classic::ModelSpec& spec,
    const classic::LabeledBatch& train,
    const classic::LabeledBatch& eval,
    classic::MetricKind metric = classic::MetricKind::Accuracy,
    std::int64_t repeats = 5,
    classic::RandomState random = {});
```

## What It Does

Fits a model from `spec` on `train`, records its baseline `metric` on
`eval`, then for each feature in turn, shuffles just that feature's column
in `eval` (`repeats` times, re-scoring after each shuffle) and measures how
much `metric` degrades relative to the baseline. A feature whose shuffled
column tanks the score is one the fitted model actually relies on; a
feature whose shuffling barely moves the score is one the model
effectively ignores -- an after-the-fact, model-specific importance signal,
not a pre-training statistical one.

## Parameters

`spec`
: A fresh classifier or regressor is built from `spec` and fit on `train`
  internally — there is no overload accepting an already-fitted model.
  Must have `task == Classification` or `Regression`.

`train` / `eval`
: Both must pass [`validate_labeled`](../data/LabeledBatch.md) and have
  the same `feature_count()`. The model is fit on `train` once; every
  permutation and the baseline score are both measured on `eval` — a
  genuinely different split is the ordinary case, the same way a
  train/test split is for any other evaluation.

`metric` (`MetricKind`, default `Accuracy`)
: The one metric compared before and after each shuffle. Its
  maximize/minimize direction (the same fixed table
  [`TuneReport::objective_policy`](../tune/TuneReport.md#state)
  documents) decides how a "drop" is computed — see `## Returns` below.
  Must resolve to exactly one value from [`assess`](../estimators/EstimatorBase/assess.md).

`repeats` (`std::int64_t`, default `5`)
: How many independent shuffles each feature gets. Must be `> 0`.
  `std_drop` is the (population) standard deviation *across* those
  repeats — a property of the underlying drop distribution, not a
  quantity that shrinks as `repeats` grows. What more `repeats` actually
  buys is a **more reliable estimate** of both `mean_drop` and
  `std_drop` themselves (the standard error of `mean_drop` is
  `std_drop / sqrt(repeats)`, which *does* shrink) — a small `repeats`
  can easily under- or over-estimate the true spread by chance. See
  [Example 5](#example-5), where `repeats = 30`'s own `std_drop` turns
  out **larger** than `repeats = 3`'s, because the smaller sample
  happened to undersample the feature's real repeat-to-repeat
  variability.

`random` (`RandomState`, default `{}`)
: Seeds every feature's shuffle sequence. `deterministic = true` (the
  default): the same `seed` reproduces the identical shuffles, and
  therefore the identical `PermuteImpactReport`, every time — see
  [Example 2](#example-2). `deterministic = false`: a fresh, genuinely
  different shuffle sequence is drawn from real entropy on every call,
  even with an otherwise-identical `RandomState{}` — see
  [Example 3](#example-3).

## Returns

A `StatusOr<PermuteImpactReport>`:

```cpp
struct classic::PermuteImpactReport {
  pluss::Series mean_drop;         // Float64, one per feature
  pluss::Series std_drop;          // Float64, one per feature (population std across repeats)
  pluss::Series per_repeat_drops;  // Float64, [repeats, features]
  classic::MetricKind metric;      // echoes the metric argument back
};
```

Every "drop" is framed so **positive always means the feature mattered**,
regardless of whether `metric` prefers larger or smaller values: for a
maximize-preferred metric (`Accuracy`, `F1`, ...) `drop = baseline -
permuted`; for a minimize-preferred metric (`LogLoss`, `Mse`, ...) `drop =
permuted - baseline`. See [Example 4](#example-4) for the same underlying
data scored both ways.

## Errors

- `repeats <= 0` — `Status::invalid_argument`.
- `train` or `eval` fails [`validate_labeled`](../data/LabeledBatch.md) —
  whatever that validation returns.
- `train.features.feature_count() != eval.features.feature_count()` —
  `Status::shape_mismatch`.
- `spec` fails to build a classifier/regressor, or `model.learn(train)`
  fails — whatever that call returns.
- The baseline `assess` call, or any permuted `assess` call, fails or
  produces a non-finite value — the failing `Status`, or
  `Status::numerical_failure`.

## Practical Examples

The default lifecycle distinguishing a real signal from noise,
reproducibility under `deterministic = true`, genuine non-determinism
under `deterministic = false`, the drop-sign convention proven identical
across a maximize- and a minimize-preferred metric on the same data,
what `repeats` actually changes (not what it might look like it changes),
a regression target, and an error roundup.

### Example 1

Two features: one cleanly separates the two classes, one is pure noise —
`mean_drop` reflects exactly that.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 separates the classes cleanly; feature 1 is unrelated noise.
  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 0.0, 4.0,
       10.0, 2.0, 10.1, 8.0, 10.2, 3.0, 10.3, 6.0},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 3;
  const cls::PermuteImpactReport report = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, random).value();
  std::cout << "mean_drop = " << report.mean_drop << "\n";
  std::cout << "std_drop  = " << report.std_drop << "\n";

  return 0;
}
```

Output

```text
mean_drop = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.3, 0]
std_drop  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.18708287, 0]
```

### Example 2

`deterministic = true` (the default) with the same `seed` reproduces the
identical `per_repeat_drops` every time.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 0.0, 4.0,
       10.0, 2.0, 10.1, 8.0, 10.2, 3.0, 10.3, 6.0},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 11;
  const cls::PermuteImpactReport a = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, random).value();
  const cls::PermuteImpactReport b = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, random).value();
  const double diff = (a.per_repeat_drops - b.per_repeat_drops).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (same seed, deterministic=true) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (same seed, deterministic=true) = 0
```

### Example 3

`deterministic = false` draws genuinely different shuffles on every call,
even with an otherwise-identical `RandomState{}`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 0.0, 4.0,
       10.0, 2.0, 10.1, 8.0, 10.2, 3.0, 10.3, 6.0},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  cls::RandomState non_deterministic{};
  non_deterministic.deterministic = false;
  const cls::PermuteImpactReport a = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, non_deterministic).value();
  const cls::PermuteImpactReport b = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, non_deterministic).value();
  const double diff = (a.per_repeat_drops - b.per_repeat_drops).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (deterministic=false, two calls) > 0 -> " << std::boolalpha << (diff > 0.0) << "\n";

  return 0;
}
```

Output

```text
max abs diff (deterministic=false, two calls) > 0 -> true
```

### Example 4

The drop-sign convention: a maximize-preferred metric (`Accuracy`) and a
minimize-preferred one (`LogLoss`) both report a **positive** `mean_drop`
for the same informative feature, on the same fitted model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 0.0, 4.0,
       10.0, 2.0, 10.1, 8.0, 10.2, 3.0, 10.3, 6.0},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 5;
  const cls::PermuteImpactReport via_accuracy = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, random).value();
  const cls::PermuteImpactReport via_logloss = cls::permutation_impact(spec, batch, batch, cls::MetricKind::LogLoss, 5, random).value();
  std::cout << "mean_drop via Accuracy (maximize) = " << via_accuracy.mean_drop << "\n";
  std::cout << "mean_drop via LogLoss  (minimize) = " << via_logloss.mean_drop << "\n";

  return 0;
}
```

Output

```text
mean_drop via Accuracy (maximize) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 0]
mean_drop via LogLoss  (minimize) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6.9448394, 2.4614299e-08]
```

### Example 5

`std_drop` does **not** shrink as `repeats` grows — it estimates the same
underlying repeat-to-repeat spread either way, and a small `repeats` can
easily undersample it. Here `repeats = 3`'s `std_drop` (from only 3
shuffles) happens to come out *smaller* than `repeats = 30`'s own more
reliable estimate of the same quantity — what actually improves with
`repeats` is confidence in the estimate, not its magnitude.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 0.0, 4.0,
       10.0, 2.0, 10.1, 8.0, 10.2, 3.0, 10.3, 6.0},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 9;
  const cls::PermuteImpactReport few = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 3, random).value();
  const cls::PermuteImpactReport many = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 30, random).value();
  std::cout << "repeats=3  std_drop = " << few.std_drop << "\n";
  std::cout << "repeats=30 std_drop = " << many.std_drop << "\n";
  std::cout << "repeats=3  per_repeat_drops shape = [" << few.per_repeat_drops.sizes()[0] << ", " << few.per_repeat_drops.sizes()[1] << "]\n";
  std::cout << "repeats=30 per_repeat_drops shape = [" << many.per_repeat_drops.sizes()[0] << ", " << many.per_repeat_drops.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
repeats=3  std_drop = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.11785113, 0]
repeats=30 std_drop = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.17950549, 0]
repeats=3  per_repeat_drops shape = [3, 2]
repeats=30 per_repeat_drops shape = [30, 2]
```

### Example 6

A regression target — `metric` defaults to `Accuracy`, which does not
apply to a regressor, so it must be set explicitly to something
regression-appropriate (`Mse` here).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 = y*2 (informative); feature 1 is unrelated noise.
  const cls::Series x = pluss::make_series(
      {2.0, 5.0, 4.0, 1.0, 6.0, 9.0, 8.0, 4.0, 10.0, 2.0, 12.0, 8.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::LeastSquares;
  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 2;
  const cls::PermuteImpactReport report = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Mse, 5, random).value();
  std::cout << "mean_drop (feature 0 informative, feature 1 noise) = " << report.mean_drop << "\n";

  return 0;
}
```

Output

```text
mean_drop (feature 0 informative, feature 1 noise) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6.1333332, 1.1696933e-16]
```

### Example 7

An error roundup: `repeats <= 0` and a train/eval feature-count mismatch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 10.0, 2.0, 10.1, 8.0, 0.1, 1.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  std::cout << "repeats=0 ok = " << std::boolalpha << cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 0).ok() << "\n";

  cls::LabeledBatch mismatched{};
  mismatched.features.x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64).reshape({4, 1});
  mismatched.y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4}, f64);
  std::cout << "train/eval feature-count mismatch ok = " << cls::permutation_impact(spec, batch, mismatched).ok() << "\n";

  return 0;
}
```

Output

```text
repeats=0 ok = false
train/eval feature-count mismatch ok = false
```

## Changelog

- **0.1** — `permutation_impact`, `PermuteImpactReport`.
- **0.2** — Fixed `random.deterministic = false` to draw real entropy
  (clock + `std::random_device`, matching every other `RandomState`
  consumer in this reference) instead of deterministically hashing
  `random.seed` — two calls with `deterministic = false` now genuinely
  differ, as [`RandomState`](../core/RandomState.md) itself documents.

## See Also

- [`dependence_curve`](dependence_curve.md) / [`dependence_surface`](dependence_surface.md)
  — the other model-inspection tools in this family.
- [`rank_features`](../features/rank_features.md) — a pre-training,
  label-correlation-only feature score, not a fitted model's own
  sensitivity.
- [`RandomState`](../core/RandomState.md) — the seed/determinism contract
  `random` follows.
- [`MetricKind`](../metrics.md) — every metric `metric` can name, and
  which direction each one prefers.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
