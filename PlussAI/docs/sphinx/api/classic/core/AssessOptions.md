# `pluss::ai::classic::AssessOptions`

`AssessOptions` configures `assess(...)` — the scoring pass every estimator
and `Flow` exposes to turn a labeled batch straight into metric values,
without you calling `infer` and a [`metrics`](../metrics.md) function
separately.

## Fields

### `layout`

`ObservationLayout layout = ObservationLayout::Rows`

As [`TrainOptions::layout`](TrainOptions.md#layout).

### `metrics`

`std::vector<MetricKind> metrics{}`

Which metrics to compute, in order — the returned `Series` (or
[`CrossAssessReport`](../metrics/CrossAssessReport.md)'s per-fold rows) is
aligned to this list. An empty vector (the default) falls back to the task's
default set: `{Accuracy}`-style essentials for classification, `{Mse, Rmse,
R2}` for regression — see each metric function's own page for the exact
default. Score-only metrics (`RocAuc`, `Gini`, `AveragePrecision`, `Brier`)
are valid here because `assess` runs inference itself and has the scores
available; passing them to a metrics function that only has hard labels would
be rejected.

### `sample_weights`

`Series sample_weights = pluss::series::undefined()`

Per-row weights for the metric computation itself — independent of any
`sample_weights` used at fit time. Undefined (the default) weights every row
equally.

### `positive_threshold`

`double positive_threshold = 0.5`

As [`InferOptions::positive_threshold`](InferOptions.md#positive_threshold),
consulted by any requested metric that needs a hard label rather than a raw
score (`Accuracy`, `F1`, `Mcc`, …).

### `regression_quantile`

`double regression_quantile = 0.5`

The quantile `MetricKind::QuantileLoss` scores against, when that metric is
requested. Must be strictly between `0` and `1`. Ignored for every other
metric and for classification.

## Practical Example

One classifier, `assess` with an explicit metric list, and the same call at
two different thresholds.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Accuracy, cls::MetricKind::F1, cls::MetricKind::RocAuc};

  const cls::StatusOr<cls::Series> scored = made.value()->assess(batch, opts);
  std::cout << "assess ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "values = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
assess ok = true
values = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

## Changelog

- **0.1** — metric list, shape guard, weights, threshold, quantile.

## See Also

- [`TrainOptions`](TrainOptions.md) / [`InferOptions`](InferOptions.md) — the
  fit-time and prediction-time counterparts.
- [`metrics`](../metrics.md) — `cross_assess`, `classification_summary`, and
  the rest of the metric functions `assess` composes.
- [`estimators`](../estimators.md) — every `assess` signature that takes this.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
