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

```cpp
lymph::metrics::classification::confusion_table(truth, pred);
lymph::metrics::classification::summary(truth, pred);
lymph::metrics::classification::report(truth, pred, {MetricKind::F1, MetricKind::Mcc});
lymph::metrics::classification::score_report(scores, labels, {MetricKind::RocAuc, MetricKind::Brier});
lymph::metrics::classification::threshold(scores, labels).best();
lymph::metrics::regression::report(truth, pred, {MetricKind::Rmse, MetricKind::R2});
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
