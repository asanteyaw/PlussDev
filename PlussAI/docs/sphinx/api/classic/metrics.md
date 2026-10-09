# `pluss::ai::classic::metrics`

`metrics` owns turning predictions into numbers: confusion matrices,
per-class and aggregate classification reports, regression error metrics,
threshold-swept precision/recall curves, and cross-validated scoring. Every
[`EstimatorBase::assess`](estimators/EstimatorBase/assess.md) call is built
on functions from this family; `metrics` is where you reach when you want
the same numbers computed from predictions you already have, or scored
across several cross-validation folds rather than one train/test split.

## The Surface

| Function / type | What it computes | Reach for it when |
| --- | --- | --- |
| [`cross_assess` / `cross_scores` / `cross_score`](metrics/cross_assess.md), [`CrossAssessReport`](metrics/CrossAssessReport.md) | fit-and-score a `ModelSpec` across every fold of a [`split`](split.md) plan | evaluating a model choice's reliability across several folds rather than one holdout |
| [`confusion_table`](metrics/confusion_table.md), [`ConfusionTable`](metrics/ConfusionTable.md), [`ConfusionOptions`](metrics/ConfusionOptions.md) | a `[k, k]` count matrix from truth/predicted labels | the raw agreement matrix, optionally normalized |
| [`classification_summary`](metrics/classification_summary.md), [`ClassReport`](metrics/ClassReport.md), [`ClassReportOptions`](metrics/ClassReportOptions.md) | every per-class and aggregate classification statistic in one call | a full report — precision/recall/F1, specificity/geometric-mean/balance-quality, MCC, Cohen's kappa, micro/macro/weighted averages |
| [`classification_metrics`](metrics/classification_metrics.md) | a chosen subset of `classification_summary`'s numbers, as a flat `Series` | only specific metric values, in a fixed requested order |
| [`regression_metrics`](metrics/regression_metrics.md) | Mse/Rmse/Mae/R2 from truth/prediction vectors | a regression counterpart to `classification_metrics`, from vectors you already hold |
| [`threshold_curve`](metrics/threshold_curve.md), [`ThresholdCurve`](metrics/ThresholdCurve.md) | precision/recall/F1 at every achievable binary decision threshold, plus `best_threshold` / `average_precision` | choosing (or evaluating) a decision threshold rather than assuming `0.5` |
| [`score_metrics`](metrics/score_metrics.md) | classification metrics straight from a score/probability `Series`, no model | predictions came from somewhere other than a `ClassifierBase::infer` call |

## How To Call It

Summarize predictions against truth labels. The macro average is laid out as precision, recall, and F1, so the program reads those three values from the report.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series truth = pluss::make_series(std::vector<double>{0, 0, 1, 1, 1, 0}, {6}, cfg);
  const pluss::Series predicted = pluss::make_series(std::vector<double>{0, 1, 1, 1, 0, 0}, {6}, cfg);
  const cls::StatusOr<cls::ClassReport> report = cls::classification_summary(truth, predicted);
  const std::vector<double> macro = report.value().macro_average.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({3}).to_vec<double>();
  std::cout << "macro precision = " << macro[0] << "\n";
  std::cout << "macro recall = " << macro[1] << "\n";
  std::cout << "macro F1 = " << macro[2] << "\n";
  return 0;
}
```

Output

```text
macro precision = 0.666667
macro recall = 0.666667
macro F1 = 0.666667
```

Every function in this family that takes `truth`/`predicted` label vectors
also has a `(const ClassifierBase&, const LabeledBatch&, ...)` overload that
runs inference for you first — see each function's own `## Parameters` for
which.

## Related Pages

- [`estimators`](estimators.md) — `EstimatorBase::assess`, the model-attached
  entry point this family's free functions sit underneath.
- [`split`](split.md) — `SplitOptions` / `SplitPlan`, what
  [`cross_assess`](metrics/cross_assess.md) runs internally.
- [`tune`](tune.md) — hyperparameter search, scored with the same metrics.
- [`core`](core.md) — `AssessOptions`, including the full `MetricKind` list
  `assess` accepts.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

metrics/Introduction
metrics/cross_assess
metrics/confusion_table
metrics/classification_summary
metrics/classification_metrics
metrics/regression_metrics
metrics/score_metrics
metrics/threshold_curve
metrics/CrossAssessReport
metrics/ClassReport
metrics/ConfusionTable
metrics/ThresholdCurve
metrics/ConfusionOptions
metrics/ClassReportOptions
```
<!-- apiref:toctree:end -->
