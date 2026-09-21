# `classic::metrics` Introduction

## What This Family Owns

`metrics` turns labels and predictions into numbers. Every function falls
into one of three shapes:

- **From label vectors you already hold** —
  [`confusion_table`](confusion_table.md), [`classification_summary`](classification_summary.md),
  [`classification_metrics`](classification_metrics.md),
  [`regression_metrics`](regression_metrics.md),
  [`threshold_curve`](threshold_curve.md) (and its siblings
  `best_threshold` / `average_precision`), [`score_metrics`](score_metrics.md) —
  no model involved at all.
- **From a model and a batch** — the same functions above each have a
  `(const ClassifierBase&, const LabeledBatch&, ...)` overload that runs
  `infer` internally first, for the common case where you have not already
  computed predictions by hand.
- **Across cross-validation folds** — [`cross_assess`](cross_assess.md) (and
  its convenience siblings `cross_scores` / `cross_score`) is the one
  function that owns both fitting *and* scoring, repeated once per
  [`split`](../split.md) fold.

## Two Reports, One Shared Core

[`confusion_table`](confusion_table.md) computes the raw `[k, k]` count
matrix; [`classification_summary`](classification_summary.md) is built
directly on top of it — every number in a [`ClassReport`](ClassReport.md)
(precision, recall, specificity, MCC, Cohen's kappa, every micro/macro/
weighted average) is derived from that same confusion matrix. Reach for
`confusion_table` alone when the matrix itself is what you want to inspect
or display; reach for `classification_summary` for the derived statistics.
[`classification_metrics`](classification_metrics.md) is a further
convenience over `classification_summary` — the same numbers, narrowed to
a requested subset and returned as a flat `Series` instead of a full
`ClassReport`.

## Beyond Sensitivity/Specificity: Balance-Aware Metrics

Alongside the standard precision/recall/F1 family, every classification
report here also computes **specificity**, **geometric mean** (of recall and
specificity), and **balance quality** (a geometric-mean-based score that
additionally rewards a small gap between recall and specificity) — per
class and as micro/macro/weighted aggregates, the same shape as
precision/recall/F1. These exist for the same reason `RobustScaler` exists
in [`scaling`](../prep/scaling.md): recall alone (or precision alone) can
look excellent on a badly imbalanced problem while specificity quietly
suffers; reach for these when class imbalance is a real concern, not just
when accuracy looks too good to trust.

## Hard Labels Versus Scores

Most functions here work from **hard** predicted labels (already-thresholded
class assignments). [`threshold_curve`](threshold_curve.md) and
[`score_metrics`](score_metrics.md) are the two that work from **scores**
(continuous probabilities or decision values) instead — the former to sweep
every possible decision threshold, the latter to compute a mix of
score-based metrics (`RocAuc`, `Gini`, `AveragePrecision`, `Brier`,
`LogLoss`) and hard-label metrics (thresholded at `0.5`, or `argmax` for
multiclass) from the same score vector in one call.

## Reading Order

1. [`confusion_table`](confusion_table.md) / [`ConfusionTable`](ConfusionTable.md) —
   the shared foundation.
2. [`classification_summary`](classification_summary.md) / [`ClassReport`](ClassReport.md) —
   the full report built on it.
3. [`classification_metrics`](classification_metrics.md) /
   [`regression_metrics`](regression_metrics.md) — narrower, flat-`Series`
   convenience forms.
4. [`threshold_curve`](threshold_curve.md) / [`score_metrics`](score_metrics.md) —
   score-based metrics, reached for by name.
5. [`cross_assess`](cross_assess.md) / [`CrossAssessReport`](CrossAssessReport.md) —
   the cross-validation loop tying everything above to [`split`](../split.md).

## Related Pages

- [`estimators`](../estimators.md) — `EstimatorBase::assess`, built on this
  family.
- [`split`](../split.md) — `SplitOptions` / `SplitPlan`, what `cross_assess`
  drives internally.
- [`tune`](../tune.md) — hyperparameter search, scored with these same
  metrics.
