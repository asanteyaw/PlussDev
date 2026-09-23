# `classic::inspection` Introduction

## What This Family Owns

`inspection` owns three free functions for understanding an **already-
fitted** model's behavior, each building its own fresh model from a
[`ModelSpec`](../families/ModelSpec.md) internally — there is no overload
anywhere in this family that accepts an already-fitted model instead.
[`permutation_impact`](permutation_impact.md) answers "does this feature
matter" (a metric-based, held-out-data importance score);
[`dependence_curve`](dependence_curve.md)/[`dependence_surface`](dependence_surface.md)
answer "how does the prediction change" as one or two features sweep
across their own observed range, holding everything else fixed — the
classic partial-dependence method. Like
[`features`](../features.md), nothing here is a persistent object.

## Importance Vs. Dependence

These are answering genuinely different questions, and mixing them up is
the easiest mistake to make with this family:

- [`permutation_impact`](permutation_impact.md) needs a **metric** and a
  **held-out `eval` batch with labels** — it measures how much scrambling
  a feature hurts a scored metric, so it answers "how much does this
  feature matter to the fitted model's actual performance."
- [`dependence_curve`](dependence_curve.md)/[`dependence_surface`](dependence_surface.md)
  need **no metric and no labels on `eval` at all** (`eval` is a plain
  [`FeatureBatch`](../data/FeatureBatch.md), not a
  [`LabeledBatch`](../data/LabeledBatch.md)) — they measure how the raw
  prediction itself moves as a feature changes, answering "what shape is
  the model's response," independent of whether that response is
  accurate.

A feature can have a strong, clear dependence curve and still score a
near-zero permutation importance (if the model's other features already
capture the same signal), and vice versa — the two are not redundant
checks of the same thing.

## Classification Support Is Deliberately Narrower Than Elsewhere

Every family that builds a classifier from a
[`ModelSpec`](../families/ModelSpec.md) elsewhere in this reference
supports both binary and multiclass targets. `dependence_curve` and
`dependence_surface` do not: both require a classifier whose
`infer_scores` produces exactly **one** score per row — a binary
classifier — because averaging a multi-class score matrix into a single
mean prediction has no one well-defined meaning (which class's
probability would it be?). A multiclass `spec` is rejected with a clear
`Status::invalid_argument` rather than either crashing or silently
returning a number no one asked for — see
[`dependence_curve`'s Example 5](dependence_curve.md#example-5) and
[`dependence_surface`'s Example 4](dependence_surface.md#example-4).
[`permutation_impact`](permutation_impact.md) has no such restriction —
it only ever compares one scalar metric value before and after a shuffle,
which is already well-defined for any number of classes.

## Reading Order

1. [`permutation_impact`](permutation_impact.md) — held-out,
   metric-based feature importance.
2. [`dependence_curve`](dependence_curve.md) — one-feature partial
   dependence.
3. [`dependence_surface`](dependence_surface.md) — the two-feature
   generalization.

## Related Pages

- [`families`](../families.md) — [`ModelSpec`](../families/ModelSpec.md),
  which every function in this family builds and fits internally.
- [`metrics`](../metrics.md) — every `MetricKind`
  [`permutation_impact`](permutation_impact.md) can score against, and
  which direction each one prefers.
- [`features`](../features.md) — a pre-training, label-correlation-only
  feature score, contrasted with this family's post-training,
  fitted-model-based one.
