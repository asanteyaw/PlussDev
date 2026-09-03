# `reports` Introduction

## What This Family Owns

`reports` owns two things that do not fit inside any single other family:

1. **The model-backed evaluation facade.** [`metrics`](../metrics.md) scores a
   truth `Series` against a prediction `Series`. `reports` does the same, but
   its entry points take a trained [`models::Model`](../models/Model.md) or a
   [`workflow::Workflow`](../workflow.md) plus a dataset, run the inference, and
   score the result — so the caller never threads `model.infer(x)` through by
   hand.

2. **The report-type index.** Report objects are defined across `metrics` and
   `selection`, but they all carry a `reports::` alias with a single short
   spelling. The [landing page](../reports.md) tabulates every alias, its
   underlying type, and its home family.

## Facade, Not Reimplementation

Nothing in `reports` computes a metric. `model.reports().classification_summary(
frame)` is exactly:

```cpp
const pluss::Series y_pred = model.infer(frame_features);
lymph::metrics::classification_summary(y_true, y_pred);
```

with the first line folded in. The value is convenience and correctness — the
inference and the scoring stay in step, and the same call works for a `Model`
or a whole `Workflow`.

## Reading Order

1. [`reports::Model`](Model.md) — `model.reports()` and everything it bundles
2. [`classification_summary` / `confusion_table`](classification_summary.md) —
   the model / workflow overloads
3. [`threshold_curve` / `best_threshold`](threshold_curve.md) — operating points

## Related Pages

- [`metrics`](../metrics.md) — the compute layer these calls delegate to.
- [`models::Model`](../models/Model.md) — `model.reports()` is a method on it.
- [`selection`](../selection.md) — home of `reports::Compare` and
  `reports::Tune`.
