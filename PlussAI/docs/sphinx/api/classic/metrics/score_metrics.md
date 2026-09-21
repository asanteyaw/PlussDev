# `pluss::ai::classic::score_metrics`

Compute classification metrics directly from a score/probability `Series`
you already hold — no model, no `infer` call. The one function in
[`metrics`](../metrics.md) that accepts both hard-label metrics (`F1`,
`Mcc`, …) and score-based metrics (`RocAuc`, `Gini`, `AveragePrecision`,
`Brier`, `LogLoss`) in the same call, the same mix
[`EstimatorBase::assess`](../estimators/EstimatorBase/assess.md) accepts —
reach for this when the predictions came from somewhere other than a
`ClassifierBase::infer` call (a model outside this reference, a
manually-computed ensemble of several models' scores, scores recovered from
a serialized report).

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::score_metrics(
    const pluss::Series& scores,
    const pluss::Series& labels,
    const std::vector<classic::MetricKind>& requested);
```

## Parameters

`scores`
: Either a `[n]` vector of positive-class probabilities (binary) or a
  `[n, k]` row-stochastic probability matrix, one row per sample, `k >= 2`
  columns (multiclass) — which shape you pass determines which path runs;
  there is no separate flag.

`labels`
: `[n]` — `0`/`1` (binary) or class indices `0..k-1` (multiclass), matching
  whichever shape `scores` has.

`requested`
: Which `MetricKind`s to compute, in order. No default — pass at least one.
  Every regression `MetricKind` (`Mse`, `Rmse`, `Mae`, `R2`,
  `QuantileLoss`) is rejected outright, checked before either path runs.

## Returns

A `StatusOr<Series>`, `Float64`, length `requested.size()`, in request
order. Score-based metrics (`RocAuc`, `Gini`, `AveragePrecision`, `Brier`,
`LogLoss`) are computed directly from `scores`. Every other (hard-label)
metric is scored against a prediction derived from `scores` first: the
binary path thresholds at `0.5`; the multiclass path takes the `argmax`
column per row — the same conventions
[`InferOptions::positive_threshold`](../core/InferOptions.md#positive_threshold)'s
default and a multiclass classifier's own `infer` use, respectively.
Multiclass hard-label metrics are the macro one-vs-rest average, mirroring
a classifier's own `assess()`.

## Errors

- Any requested `MetricKind` is a regression metric — `Status::invalid_argument`,
  checked before either path runs.
- `scores`/`labels` shape or length mismatch for the resolved (binary or
  multiclass) path — `Status::shape_mismatch`.

## Practical Examples

The binary path computing a hard-label and a score-based metric together,
and the multiclass path from a row-stochastic probability matrix.

### Example 1

Binary scores: `RocAuc` (from the raw scores) alongside `Accuracy` (from a
`0.5`-thresholded prediction), in one call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.3, 0.1}, {4}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);

  const pluss::Series values =
      cls::score_metrics(scores, labels, {cls::MetricKind::RocAuc, cls::MetricKind::Accuracy}).value();
  std::cout << "[RocAuc, Accuracy] = " << values << "\n";

  return 0;
}
```

Output

```text
[RocAuc, Accuracy] = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

### Example 2

A `[4, 3]` multiclass probability matrix: `LogLoss` from the raw
probabilities, `Accuracy` from each row's `argmax`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 4 rows, 3 classes -- each row's own argmax matches its true label.
  const pluss::Series scores = pluss::make_series(
      {0.7, 0.2, 0.1,
       0.1, 0.8, 0.1,
       0.2, 0.2, 0.6,
       0.6, 0.3, 0.1}, {12}, f64).reshape({4, 3});
  const pluss::Series labels = pluss::make_series({0.0, 1.0, 2.0, 0.0}, {4}, f64);

  const pluss::Series values =
      cls::score_metrics(scores, labels, {cls::MetricKind::LogLoss, cls::MetricKind::Accuracy}).value();
  std::cout << "[LogLoss, Accuracy] = " << values << "\n";

  return 0;
}
```

Output

```text
[LogLoss, Accuracy] = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.40036744, 1]
```

### Example 3

A regression metric is rejected outright.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.1}, {2}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 0.0}, {2}, f64);

  const cls::StatusOr<pluss::Series> values = cls::score_metrics(scores, labels, {cls::MetricKind::Mse});
  std::cout << "ok = " << std::boolalpha << values.ok() << "\n";
  std::cout << "message = \"" << values.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "score_metrics: regression metric requested on a classification score/label pair."
```

## Changelog

- **0.1** — binary and multiclass paths, hard-label and score-based
  metrics.

## See Also

- [`threshold_curve`](threshold_curve.md) — `average_precision`, one of the
  metrics this function can also compute via `MetricKind::AveragePrecision`.
- [`classification_metrics`](classification_metrics.md) — the hard-label-only
  counterpart, from already-thresholded predictions.
- [`EstimatorBase::assess`](../estimators/EstimatorBase/assess.md) — the
  model-attached path accepting the same metric mix.
