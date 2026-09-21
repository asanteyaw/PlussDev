# `pluss::ai::classic::split`

`split` owns turning a sample count (or a label/group column) into
**indices** — which rows train, which rows test — without touching any
actual feature data itself. Every function here returns an
[`IndexSplit`](split/IndexSplit.md) (one train/test pair) or a
[`SplitPlan`](split/SplitPlan.md) (several folds' worth); apply the result
to real data with [`slice_feature_batch`/`slice_labeled_batch`](split/slice_batch.md).

This separation — compute indices, then slice — is what lets the same
[`SplitOptions`](split/SplitOptions.md) drive a one-off holdout split you
apply yourself, or a whole cross-validation loop inside
[`metrics::cross_assess`](metrics.md) or a [`tune`](tune.md) search, without
`split` itself needing to know which.

## The Surface

| Function | Returns | Reach for it when |
| --- | --- | --- |
| [`holdout_split` / `strat_holdout_split`](split/holdout.md) | one [`IndexSplit`](split/IndexSplit.md) | a single train/test partition is enough |
| [`crossval_plan` / `stratified_crossval_plan` / `group_crossval_plan`](split/crossval_plan.md) | a [`SplitPlan`](split/SplitPlan.md) of `folds` partitions | k-fold cross-validation, optionally class-stratified or group-aware |
| [`ts_crossval_plan`](split/ts_crossval_plan.md) | a [`SplitPlan`](split/SplitPlan.md) of chronological, non-shuffled partitions | time-ordered data, where a fold's test window must come strictly after its train window |
| [`slice_feature_batch` / `slice_labeled_batch`](split/slice_batch.md) | a [`FeatureBatch`](data/FeatureBatch.md) / [`LabeledBatch`](data/LabeledBatch.md) | applying an `IndexSplit`'s indices to your actual data |

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

cls::SplitOptions options{};
options.holdout_fraction = 0.2;
const cls::StatusOr<cls::IndexSplit> split = cls::holdout_split(batch.sample_count(), options);

const cls::FeatureBatch train = cls::slice_feature_batch(batch, split.value().train_indices).value();
const cls::FeatureBatch test = cls::slice_feature_batch(batch, split.value().test_indices).value();
```

## Related Pages

- [`metrics`](metrics.md) — `cross_assess`, which drives one of these split
  functions internally from the same `SplitOptions`, based on its `plan`
  field.
- [`tune`](tune.md) — hyperparameter search, evaluated across the same kind
  of split plan.
- [`data`](data.md) — `FeatureBatch` / `LabeledBatch`, what
  `slice_feature_batch` / `slice_labeled_batch` actually slice.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

split/Introduction
split/holdout
split/crossval_plan
split/ts_crossval_plan
split/slice_batch
split/IndexSplit
split/SplitPlan
split/SplitOptions
```
<!-- apiref:toctree:end -->
