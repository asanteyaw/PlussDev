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
| [`classification`](metrics/classification.md) | `classification(truth, pred)` → a `Classification` accessor: `confusion_table()`, `summary()`, `values()`, `report()` |
| [`regression`](metrics/regression.md) | `regression(truth, pred)` → a `Regression` accessor: `values()`, `report()` |
| [`Threshold`](metrics/Threshold.md) | `threshold(scores, labels)` → `curve()` and `best(objective)` for choosing an operating point |
| [`MetricReport`](metrics/MetricReport.md) | the per-metric value bag (`accuracy`, `f1`, `rmse`, …) returned by every `report()` |
| [`CrossValidationReport`](metrics/CrossValidationReport.md) | per-fold reports + metric mean / stdev + `board()` — what a validation `Plan::run` returns |
| [`ClassReport`](metrics/ClassReport.md) | per-class precision / recall / f1 / support; `ConfusionTable` is documented here too |
| [`SeriesCompareReport`](metrics/SeriesCompareReport.md) | element-wise difference from `compare_series` / `max_abs_diff` |
| [`confusion_table`](metrics/confusion_table.md) / [`classification_summary`](metrics/classification_summary.md) | the free-function forms of the two classification tables |
| [`calibrate`](metrics/calibrate.md) | `calibrate_with_platt` / `_isotonic` / `_temperature` → a `TrainOptions` with score calibration set |

## Two Ways In

- **Fluent accessor** — `metrics::classification(truth, pred).summary()`,
  `metrics::regression(truth, pred).report()`,
  `metrics::threshold(scores, labels).best()`. One object, several views, no
  repeated argument threading.
- **Free function** — `metrics::confusion_table(truth, pred)`,
  `metrics::classification_summary(truth, pred)` when you want exactly one table
  and nothing else.

Both compute the same numbers; the accessor just caches the inputs.

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
