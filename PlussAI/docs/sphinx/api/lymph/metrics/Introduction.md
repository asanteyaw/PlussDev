# `metrics` Introduction

## What This Family Owns

`metrics` owns the step *after* prediction and *before* a decision: given a
`Series` of ground truth and a `Series` of model output, produce a structured,
reusable evaluation artifact instead of a one-off number.

Concretely:

- **Metric values** — `accuracy`, `f1`, `roc_auc`, `mcc`, `cohen_kappa`,
  `gini`, `average_precision`, `brier`, `rmse`, `r2`, and the rest, as a
  `Series` (one metric) or a [`MetricReport`](MetricReport.md) (a named bag).
  Label-only metrics (accuracy, f1, MCC, kappa, …) work from a truth /
  prediction pair via `classification::report`; score metrics (roc_auc, gini,
  average_precision, brier, logloss) take a probability `Series` via
  `classification::score_report`, or run through a model-backed
  [`reports`](../reports.md) call that infers the probabilities first.
- **Classification tables** — a [`ConfusionTable`](ClassReport.md) and a
  per-class [`ClassReport`](ClassReport.md) (precision / recall / f1 / support).
- **Cross-validation** — a [`CrossValidationReport`](CrossValidationReport.md):
  every fold's report, plus the metric mean and stdev across folds, plus a
  `board()` table.
- **Operating points** — a [`Threshold`](Threshold.md) accessor over a score /
  label pair: the full precision–recall–threshold `curve()`, or the single
  `best()` threshold for an objective.
- **Series diffs** — [`compare_series`](SeriesCompareReport.md) for "how far
  apart are these two vectors", used in tests and reproducibility checks.
- **Calibration presets** — [`calibrate`](calibrate.md) helpers that return a
  `TrainOptions` with Platt / isotonic / temperature scaling switched on.

## Compute Layer vs. Facade

Everything in `metrics` takes **data you already have** — the truth `Series` and
the prediction (or score) `Series`. It never trains or runs a model.

[`reports`](../reports.md) is the facade: the same helpers, but each takes a
`models::Model` (or `workflow::Workflow`) plus a dataset and runs the inference
for you. `model.reports().classification_summary(frame)` is
`metrics::classification::summary(y_true, model.infer(x))` with the middle step
folded in.

## Reading Order

1. [`MetricReport`](MetricReport.md) — the value bag `report(...)` returns
2. [`classification`](classification.md) / [`regression`](regression.md) — the
   `metrics::classification::` / `metrics::regression::` function groups
3. [`CrossValidationReport`](CrossValidationReport.md) — fold-level evidence
4. [`Threshold`](Threshold.md) — when the operating point matters

## Namespace

The compute helpers live in `lymph::metrics::classification` and
`lymph::metrics::regression` — one spelling each
(`lymph::metrics::classification::confusion_table`, and so on). The report
*types* carry umbrella aliases in [`reports`](../reports.md): `reports::Metric`,
`reports::Class`, `reports::CrossVal`, `reports::SeriesCompare`.

## Related Pages

- [`reports`](../reports.md) — model-backed evaluation, plus the alias index.
- [`selection::validate`](../selection/validate.md) — earns the evidence a
  `CrossValidationReport` holds.
- [`models::Model`](../models/Model.md) — produces the predictions scored here.
