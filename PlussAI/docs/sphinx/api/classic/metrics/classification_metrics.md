# `pluss::ai::classic::classification_metrics`

Compute a chosen subset of [`classification_summary`](classification_summary.md)'s
numbers, as one flat `Series` in requested order — the convenience form
when you want specific metric values rather than a full
[`ClassReport`](ClassReport.md). Every requested metric that has a per-class
and an aggregate form (`Precision`, `Recall`, `F1`, `Specificity`,
`GeometricMean`, `BalanceQuality`) reports its **macro** average here, never
the per-class breakdown or the micro/weighted variants — reach for
[`classification_summary`](classification_summary.md) directly when you
need those instead.

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::classification_metrics(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const std::vector<classic::MetricKind>& metrics = {},
    const classic::ClassReportOptions& options = {});

classic::StatusOr<pluss::Series> classic::classification_metrics(
    const classic::ClassifierBase& classifier,
    const classic::LabeledBatch& batch,
    const classic::AssessOptions& options = {});
```

## Parameters

`truth_labels` / `predicted_labels` (first overload)
: As [`classification_summary`](classification_summary.md)'s own.

`metrics` (first overload)
: Which `MetricKind`s to compute, in the order they should appear in the
  result. Empty (the default) requests
  `{Accuracy, BalancedAccuracy, Precision, Recall, F1}`. Supports
  `Accuracy`, `BalancedAccuracy` (macro recall — the standard meaning),
  `Precision`, `Recall`/`Sensitivity`, `F1`, `Specificity`, `GeometricMean`,
  `BalanceQuality`, `Mcc`, `CohenKappa` — every other `MetricKind`
  (regression metrics, or a score-based metric like `RocAuc`) is rejected,
  since this overload only ever sees hard labels.

`options` (`ClassReportOptions`, default `{}`, first overload)
: Forwarded to the underlying `classification_summary` call.

`classifier` / `batch` / `options` (second overload)
: Identical to `classifier.assess(batch, options)` — see
  [`EstimatorBase::assess`](../estimators/EstimatorBase/assess.md), which
  accepts every `MetricKind` including the score-based ones this overload's
  sibling rejects, since `assess` has the raw scores available from its own
  `infer` call.

## Returns

A `StatusOr<Series>`, `Float64`, length `metrics.size()` (or `5` for the
default list), one value per requested metric in request order.

## Errors

- A requested `MetricKind` outside the supported hard-label set (first
  overload only) — `Status::invalid_argument` naming `EstimatorBase::assess`
  as the path for score-based metrics.
- Everything [`classification_summary`](classification_summary.md#errors)
  can fail with.
- Second overload: whatever `classifier.assess` itself can fail with.

## Practical Examples

The default metric list, an explicit subset in a chosen order, and the
rejection of a score-based metric this overload cannot compute.

### Example 1

The default five metrics on a small 2-class problem.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 1.0, 1.0, 1.0}, {4}, f64);

  const pluss::Series values = cls::classification_metrics(truth, pred).value();
  std::cout << "[Accuracy, BalancedAccuracy, Precision, Recall, F1] = " << values << "\n";

  return 0;
}
```

Output

```text
[Accuracy, BalancedAccuracy, Precision, Recall, F1] = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.75, 0.75, 0.83333333, 0.75, 0.73333333]
```

### Example 2

A specific, ordered subset — `BalanceQuality` before `Mcc`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 1.0, 1.0, 1.0}, {4}, f64);

  const pluss::Series values =
      cls::classification_metrics(truth, pred, {cls::MetricKind::BalanceQuality, cls::MetricKind::Mcc}).value();
  std::cout << "[BalanceQuality, Mcc] = " << values << "\n";

  return 0;
}
```

Output

```text
[BalanceQuality, Mcc] = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 0.57735027]
```

### Example 3

`RocAuc` (a score-based metric) is rejected — the error names the path that
does support it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 1.0}, {2}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 1.0}, {2}, f64);

  const cls::StatusOr<pluss::Series> values =
      cls::classification_metrics(truth, pred, {cls::MetricKind::RocAuc});
  std::cout << "ok = " << std::boolalpha << values.ok() << "\n";
  std::cout << "message = \"" << values.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "classification_metrics from hard labels supports hard-label classification metrics only. Use classifier.assess(...) for probability-driven metrics (RocAuc, LogLoss, Gini, AveragePrecision, Brier)."
```

### Example 4

The classifier-attached overload — identical to calling
`classifier.assess(batch, options)` directly — accepts `RocAuc`, the exact
score-based metric Example 3's overload just rejected, since it has the
raw scores available from its own `infer` call.

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
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  cls::AssessOptions options{};
  options.metrics = {cls::MetricKind::RocAuc, cls::MetricKind::Accuracy};
  const cls::StatusOr<pluss::Series> values = cls::classification_metrics(*made.value(), batch, options);
  std::cout << "ok = " << std::boolalpha << values.ok() << "\n";
  std::cout << "[RocAuc, Accuracy] = " << values.value() << "\n";

  return 0;
}
```

Output

```text
ok = true
[RocAuc, Accuracy] = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

## Changelog

- **0.1** — two overloads (label-vector, classifier-attached).

## See Also

- [`classification_summary`](classification_summary.md) — the full report
  this narrows.
- [`EstimatorBase::assess`](../estimators/EstimatorBase/assess.md) — the
  path that accepts score-based metrics too.
- [`regression_metrics`](regression_metrics.md) — the regression
  counterpart.
