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

Hold out a quarter of the rows, then slice the batch into train and test parts. The program prints each part's row count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const cls::LabeledBatch batch = fixture_batch(cfg);
  cls::SplitOptions options{};
  options.holdout_fraction = 0.25;
  const cls::StatusOr<cls::IndexSplit> split = cls::holdout_split(batch.features.sample_count(), options);
  const cls::FeatureBatch train = cls::slice_feature_batch(batch.features, split.value().train_indices).value();
  const cls::FeatureBatch test = cls::slice_feature_batch(batch.features, split.value().test_indices).value();
  std::cout << "train rows = " << train.sample_count() << "\n";
  std::cout << "test rows = " << test.sample_count() << "\n";
  return 0;
}
```

Output

```text
train rows = 6
test rows = 2
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
