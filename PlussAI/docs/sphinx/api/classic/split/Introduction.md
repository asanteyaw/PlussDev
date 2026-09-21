# `classic::split` Introduction

## What This Family Owns

`split` computes **indices**, never feature data. Every function takes a
sample count, a label column, or a group column, and returns which row
indices belong to train versus test — an [`IndexSplit`](IndexSplit.md) for
one partition, a [`SplitPlan`](SplitPlan.md) (several `IndexSplit`s) for a
cross-validation scheme. [`slice_feature_batch` / `slice_labeled_batch`](slice_batch.md)
are the only functions here that touch real data, and they do only one
thing: apply indices you already have to a batch.

## One Options Struct, Many Callers

[`SplitOptions`](SplitOptions.md) is large because it is shared by every
function in this family **and** by every higher-level caller that runs a
split internally — [`metrics::cross_assess`](../metrics.md) picks among
`holdout_split`/`crossval_plan`/`stratified_crossval_plan`/
`group_crossval_plan`/`ts_crossval_plan` based on its `plan` field, and
[`tune`](../tune.md) threads a `SplitOptions` through to the same place for
every trial it evaluates. Calling a `split` function directly (rather than
going through `cross_assess`/`tune`) means `plan` itself is never
consulted — you already picked the function that matches it by name — but
every other field the function you called actually reads still applies.
See each function's own `## Parameters` for exactly which fields of
`SplitOptions` it reads.

## Determinism

Every function that shuffles takes its randomness from `SplitOptions.random`
(a [`RandomState`](../core/RandomState.md)) through the same shuffle
routine, so the same `RandomState` value and inputs always produce the same
partition. `ts_crossval_plan` never shuffles at all
— it rejects `shuffle = true` outright, since a time-ordered split's whole
point is preserving chronological order.

## Reading Order

1. [`holdout`](holdout.md) — the simplest case, one train/test split.
2. [`crossval_plan`](crossval_plan.md) — k-fold and its stratified/grouped
   variants.
3. [`ts_crossval_plan`](ts_crossval_plan.md) — reached for specifically when
   data is time-ordered.
4. [`slice_batch`](slice_batch.md) — applying any of the above to real data.
5. [`IndexSplit`](IndexSplit.md) / [`SplitPlan`](SplitPlan.md) /
   [`SplitOptions`](SplitOptions.md) — the three value types, read as
   needed.

## Related Pages

- [`metrics`](../metrics.md) — `cross_assess`, the primary consumer of a
  full `SplitOptions`.
- [`tune`](../tune.md) — hyperparameter search over the same kind of split.
- [`data`](../data.md) — `FeatureBatch` / `LabeledBatch`, sliced by
  [`slice_batch`](slice_batch.md).
