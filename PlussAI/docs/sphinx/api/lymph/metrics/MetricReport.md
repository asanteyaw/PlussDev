<!-- pluss-hand-authored -->

# `lymph::metrics::MetricReport`

[Back to `metrics`](../metrics.md)

`MetricReport` is the **per-metric value bag** that
`metrics::classification::report(...)` and `metrics::regression::report(...)`
return. It is a plain value: a `values` `Series` in the order you asked for,
plus a named 1-element `Series` for each metric that was computed.

## Scope And Alias

The type is `lymph::metrics::MetricReport`, re-exported at `lymph::` scope and
carrying the umbrella alias `lymph::reports::Metric`. All three name the same
type; use `reports::Metric` in cross-family code.

## Fields

`std::vector<MetricKind> requested`
: The metric kinds, in the order they were requested. Aligned index-for-index
  with `values`.

`Series values`
: One value per requested metric, same order as `requested`. Length =
  `requested.size()`.

`Series accuracy` · `balanced_accuracy` · `precision` · `recall` ·
`sensitivity` · `f1` · `specificity` · `geometric_mean` · `balance_quality` ·
`roc_auc` · `logloss` · `mcc` · `cohen_kappa` · `gini` · `average_precision` ·
`brier`
: Classification slots. Each is a **1-element `Series`** slice of `values` when
  that metric was requested, and `pluss::series::undefined()` otherwise.

`Series mse` · `rmse` · `mae` · `r2` · `quantile_loss`
: Regression slots, same rule.

A slot you did not request is left undefined — check `field.defined()` before
reading, or just read `values` by index.

`mcc` (Matthews correlation) and `cohen_kappa` are computed from hard labels, so
they come through any path. `roc_auc`, `logloss`, `gini` (`2·roc_auc − 1`),
`average_precision` (area under the precision-recall curve), and `brier` (a
loss) need scores / probabilities: get them from
[`metrics::classification::score_report(scores, labels, {…})`](classification.md)
when you hold a probability `Series`, or from a model-backed
[`reports`](../reports.md) call that runs inference first. They are **not**
filled by the label-only `metrics::classification::report`.

## Reading a `MetricReport`

### By name

Request the metrics you want, then read the matching slot with `.item<double>()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 1, 1, 0, 1}, {10}, f64);

  const std::vector<lymph::MetricKind> want = {
      lymph::MetricKind::Accuracy, lymph::MetricKind::Precision,
      lymph::MetricKind::Recall, lymph::MetricKind::F1};

  const lymph::metrics::MetricReport report =
      lymph::metrics::classification::report(truth, predicted, want);

  const std::vector<double> values = report.values.to_vec<double>();
  std::cout << "requested x values:\n";
  for (std::size_t i = 0; i < report.requested.size(); ++i) {
    std::cout << "  metric[" << i << "] = " << values[i] << "\n";
  }
  std::cout << "report.accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "report.f1       = " << report.f1.item<double>() << "\n";

  return 0;
}
```

Output

```text
requested x values:
  metric[0] = 0.8
  metric[1] = 0.791667
  metric[2] = 0.791667
  metric[3] = 0.791667
report.accuracy = 0.8
report.f1       = 0.791667
```

### From `metrics::regression::report`

`metrics::regression::report(truth, pred, ...)` fills the regression slots
instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({10., 12., 14., 16., 18., 20., 22., 24.}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({10.5, 11.5, 14.2, 15.6, 18.4, 19.7, 22.3, 23.6}, {8}, f64);

  const std::vector<lymph::MetricKind> want = {
      lymph::MetricKind::R2, lymph::MetricKind::Mse,
      lymph::MetricKind::Rmse, lymph::MetricKind::Mae};

  const lymph::metrics::MetricReport report =
      lymph::metrics::regression::report(truth, predicted, want);

  std::cout << "r2   = " << report.r2.item<double>() << "\n";
  std::cout << "mse  = " << report.mse.item<double>() << "\n";
  std::cout << "rmse = " << report.rmse.item<double>() << "\n";
  std::cout << "mae  = " << report.mae.item<double>() << "\n";

  return 0;
}
```

Output

```text
r2   = 0.992857
mse  = 0.15
rmse = 0.387298
mae  = 0.375
```

### The default metric set

Call `report(truth, pred)` with no metric list and it picks the task's default
metrics. `requested` tells you which ones you got.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 1, 1, 0, 1}, {10}, f64);

  const lymph::metrics::MetricReport report =
      lymph::metrics::classification::report(truth, predicted);

  const std::vector<double> values = report.values.to_vec<double>();
  std::cout << "default classification metrics (" << report.requested.size() << "):\n";
  for (std::size_t i = 0; i < report.requested.size(); ++i) {
    std::cout << "  kind " << static_cast<int>(report.requested[i])
              << " = " << values[i] << "\n";
  }
  std::cout << "balanced_accuracy defined? " << std::boolalpha
            << report.balanced_accuracy.defined() << "\n";
  std::cout << "roc_auc defined? " << report.roc_auc.defined()
            << " (not in the default set)\n";

  return 0;
}
```

Output

```text
default classification metrics (5):
  kind 0 = 0.8
  kind 1 = 0.791667
  kind 2 = 0.791667
  kind 3 = 0.791667
  kind 4 = 0.791667
balanced_accuracy defined? true
roc_auc defined? false (not in the default set)
```

### MCC and Cohen's kappa from labels

`mcc` (Matthews correlation) and `cohen_kappa` are confusion-matrix metrics, so
they come straight off a truth / prediction label pair — no scores needed. (For
a 2×2 with TP = 3, FN = 1, TN = 2, FP = 2: MCC = 4 / √240 ≈ 0.2582,
κ = 0.125 / 0.5 = 0.25.)

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({1, 1, 1, 1, 0, 0, 0, 0}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({1, 1, 1, 0, 0, 0, 1, 1}, {8}, f64);

  const lymph::metrics::MetricReport report =
      lymph::metrics::classification::report(
          truth, predicted, {lymph::MetricKind::Mcc, lymph::MetricKind::CohenKappa});

  std::cout << "mcc         = " << report.mcc.item<double>() << "\n";
  std::cout << "cohen_kappa = " << report.cohen_kappa.item<double>() << "\n";
  std::cout << "values      = " << report.values << "\n";

  return 0;
}
```

Output

```text
mcc         = 0.258199
cohen_kappa = 0.25
values      = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.25819889, 0.25]
```

`gini`, `average_precision`, and `brier` are not reachable through the label-only
`report` — they need scores. From a probability `Series` you already hold, call
[`metrics::classification::score_report(scores, labels, {…})`](classification.md);
from a model, `model.reports().metric_series(batch, {MetricKind::Gini, …})` runs
inference and then does the same.

## Methods

None. `MetricReport` is a plain value type — the `values` / `requested` pair and
the named metric slots above are its whole surface. The
[`CrossValidationReport`](CrossValidationReport.md) `mean` / `stdev` fields are
`MetricReport`s too.

## Changelog

- **0.1** — `requested` / `values` plus the classification and regression metric
  slots; umbrella alias `reports::Metric`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
