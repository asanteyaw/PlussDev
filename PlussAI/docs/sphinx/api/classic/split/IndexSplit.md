# `pluss::ai::classic::IndexSplit`

A pair of row-index `Series` — which rows belong to train, which to test —
for one partition of the data. Every function in
[`holdout`](holdout.md) returns one directly; every function in
[`crossval_plan`](crossval_plan.md) and [`ts_crossval_plan`](ts_crossval_plan.md)
returns several of them bundled into a [`SplitPlan`](SplitPlan.md). Pass
either field to [`slice_feature_batch` / `slice_labeled_batch`](slice_batch.md)
to actually cut a batch along it.

## Signature

```cpp
pluss::Series classic::IndexSplit::train_indices;
pluss::Series classic::IndexSplit::test_indices;
```

## Construction

Produced only by [`holdout_split`](holdout.md) /
[`strat_holdout_split`](holdout.md), or read out of a
[`SplitPlan::folds`](SplitPlan.md#state) entry. A plain value type — copy
or store it freely.

## State

`train_indices` — `pluss::Series`
: `Int64`, rank-1. The row indices (into whatever `sample_count` or label
  column the producing call was given) selected for training.

`test_indices` — `pluss::Series`
: `Int64`, rank-1. The row indices selected for testing. Disjoint from
  `train_indices` — every function in this family partitions rows, it never
  duplicates one into both sides.

## Methods

None — `IndexSplit` is a plain data holder, read directly.

## Practical Example

A holdout split's two index sets, and slicing a batch by each.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0}, {5}, f64).reshape({5, 1});

  cls::SplitOptions options{};
  options.holdout_fraction = 0.4;
  const cls::IndexSplit split = cls::holdout_split(batch.sample_count(), options).value();

  std::cout << "train_indices = " << split.train_indices << "\n";
  std::cout << "test_indices  = " << split.test_indices << "\n";

  const pluss::Series train_x = cls::slice_feature_batch(batch, split.train_indices).value().x;
  std::cout << "train x = " << train_x << "\n";

  return 0;
}
```

Output

```text
train_indices = pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 3, 4]
test_indices  = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 0]
train x = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [3],
  [4]]
```

## Changelog

- **0.1** — `train_indices`, `test_indices`.

## See Also

- [`holdout`](holdout.md) — the functions that produce one directly.
- [`SplitPlan`](SplitPlan.md) — several `IndexSplit`s, one per fold.
- [`slice_batch`](slice_batch.md) — applying these indices to real data.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
