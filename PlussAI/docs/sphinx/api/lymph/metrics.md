# Lymph `metrics`

`metrics` is the lymph family that turns a `Series` of truth values and a
`Series` of predictions into structured evidence — scalar and per-fold metric
values, per-class breakdowns, confusion tables, and operating-point curves.

It is the **compute layer**. Everything here takes data you already hold; it
does not run a model. When you have a trained `models::Model` and want the same
evidence *and* the inference done for you, use [`reports`](reports.md).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`classification`](metrics/classification.md) | the `metrics::classification` group — `confusion_table`, `summary`, `report`, `score_report`, `threshold` |
| [`regression`](metrics/regression.md) | `metrics::regression::report(truth, pred, {kinds})` → a `MetricReport` |
| [`Threshold`](metrics/Threshold.md) | `metrics::classification::threshold(scores, labels)` → `curve()` and `best(objective)` |
| [`MetricReport`](metrics/MetricReport.md) | the per-metric value bag (`accuracy`, `f1`, `rmse`, …) `report(...)` returns; `.values` is the raw `Series` |
| [`CrossValidationReport`](metrics/CrossValidationReport.md) | per-fold reports + metric mean / stdev + `board()` — what a validation `Plan::run` returns |
| [`ClassReport`](metrics/ClassReport.md) | per-class precision / recall / f1 / support; `ConfusionTable` is documented here too |
| [`SeriesCompareReport`](metrics/SeriesCompareReport.md) | element-wise difference from `compare_series` / `max_abs_diff` |
| [`confusion_table`](metrics/confusion_table.md) / [`classification_summary`](metrics/classification_summary.md) | the two classification-table functions, in full |
| [`calibrate`](metrics/calibrate.md) | `calibrate_with_platt` / `_isotonic` / `_temperature` → a `TrainOptions` with score calibration set |

## How To Call It

Every entry point is a free function under `metrics::classification::` or
`metrics::regression::`, taking the truth `Series` and the prediction (or score)
`Series` directly:

Build a confusion table, a classification summary, and a two-metric report from the same truth and prediction vectors. The program prints the accuracy from the summary and how many metrics the report carries.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace lymph = pluss::ai::lymph;
using pluss::table::DataFrame;
using pluss::table::Slate;

lymph::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series truth = pluss::make_series(std::vector<double>{0, 0, 1, 1, 1, 0}, {6}, cfg);
  const pluss::Series pred = pluss::make_series(std::vector<double>{0, 1, 1, 1, 0, 0}, {6}, cfg);
  const lymph::ConfusionTable table = lymph::metrics::classification::confusion_table(truth, pred);
  (void)table;
  const lymph::ClassReport summary = lymph::metrics::classification::summary(truth, pred);
  std::cout << "accuracy = " << summary.accuracy.item<double>() << "\n";
  const lymph::MetricReport report = lymph::metrics::classification::report(
      truth, pred, {lymph::MetricKind::F1, lymph::MetricKind::Mcc});
  std::cout << "requested metrics = " << report.values.numel() << "\n";
  return 0;
}
```

Output

```text
accuracy = 0.666667
requested metrics = 2
```

`report` scores hard labels; `score_report` takes a probability `Series` (or a
`[n, k]` probability matrix) and is the only data-level path to `RocAuc`,
`Gini`, `AveragePrecision`, `LogLoss` and `Brier`.

`report(...)` returns a [`MetricReport`](metrics/MetricReport.md); read
`.values` for the raw `Series` or a named slot (`.f1`, `.rmse`) with
`.item<double>()`. There is one spelling per function — the old bare
`lymph::confusion_table` / `lymph::classification_summary` names and the
`Classification` / `Regression` accessor classes have been removed.

## Relationship To Other Families

- [`selection::validate`](selection/validate.md) consumes the validation
  contract and runs it; the `CrossValidationReport` it hands back is defined
  here (`reports::CrossVal`).
- [`reports`](reports.md) is the model-backed twin — the same helpers, but they
  take a `models::Model` or `workflow::Workflow` and do the inference for you.
- The umbrella aliases (`reports::Metric`, `reports::Class`,
  `reports::CrossVal`, `reports::SeriesCompare`) live in
  [`reports`](reports.md); the types themselves are `metrics` types.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

metrics/Introduction
metrics/MetricReport
metrics/CrossValidationReport
metrics/ClassReport
metrics/SeriesCompareReport
metrics/Threshold
metrics/classification
metrics/regression
metrics/confusion_table
metrics/classification_summary
metrics/calibrate
```
<!-- apiref:toctree:end -->
