# `pluss::ai::classic::slice_feature_batch` / `slice_labeled_batch`

Apply an index `Series` — typically one side of an
[`IndexSplit`](IndexSplit.md), or any other row-index selection — to an
actual [`FeatureBatch`](../data/FeatureBatch.md) or
[`LabeledBatch`](../data/LabeledBatch.md), returning the corresponding
subset. Every other function in this family only computes indices;
these two are the step that turns an index split into real train/test
data.

## Signature

```cpp
classic::StatusOr<classic::FeatureBatch> classic::slice_feature_batch(
    const classic::FeatureBatch& in, const pluss::Series& indices);

classic::StatusOr<classic::LabeledBatch> classic::slice_labeled_batch(
    const classic::LabeledBatch& in, const pluss::Series& indices);
```

## Parameters

`in`
: The batch to slice. Must already pass
  [`validate_features`](../data/FeatureBatch.md) (or, for
  `slice_labeled_batch`, the equivalent labeled-batch check).

`indices`
: A defined `Series` of row indices to select, in the order given —
  `indices` need not be sorted, and repeating an index repeats that row in
  the output. Reshaped to rank-1 internally, so any rank carrying the right
  element count works.

## Returns

`slice_feature_batch` returns a `FeatureBatch` with the same `layout` as
`in`, its `x` selected along the sample axis (row axis under `Rows`
layout, column axis under `Columns`) by `indices`.
`slice_labeled_batch` does the same to `in.features` and additionally
selects `in.y` (and `in.sample_weights`, if defined) by the same
`indices` — every field stays aligned to the same rows.

## Errors

- `in` fails feature (or labeled-batch) validation — that `Status`,
  propagated directly.
- `indices` undefined — `Status::invalid_argument`.
- The selected result fails feature validation (an index selection that
  somehow produces an invalid shape) — that `Status`, propagated.

## Practical Examples

Slicing a `FeatureBatch` and a `LabeledBatch` (including its
`sample_weights`) by a holdout split's two index sets, and an out-of-range
index failing cleanly.

### Example 1

A `FeatureBatch` cut into its train and test rows by a holdout split.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64).reshape({5, 1});

  cls::SplitOptions options{};
  options.holdout_fraction = 0.4;
  const cls::IndexSplit split = cls::holdout_split(5, options).value();

  const cls::FeatureBatch train = cls::slice_feature_batch(batch, split.train_indices).value();
  const cls::FeatureBatch test = cls::slice_feature_batch(batch, split.test_indices).value();
  std::cout << "train.x = " << train.x << "\n";
  std::cout << "test.x  = " << test.x << "\n";

  return 0;
}
```

Output

```text
train.x = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[20],
  [40],
  [50]]
test.x  = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[30],
  [10]]
```

### Example 2

A `LabeledBatch` with `sample_weights` — slicing keeps `features`, `y`, and
`sample_weights` aligned to the same rows.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64);
  batch.sample_weights = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64);

  const pluss::Config i64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);
  const std::vector<std::int64_t> index_values{0, 2};
  const pluss::Series indices = pluss::make_series(index_values, {2}, i64);
  const cls::LabeledBatch sliced = cls::slice_labeled_batch(batch, indices).value();
  std::cout << "features.x       = " << sliced.features.x << "\n";
  std::cout << "y                = " << sliced.y << "\n";
  std::cout << "sample_weights   = " << sliced.sample_weights << "\n";

  return 0;
}
```

Output

```text
features.x       = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [3]]
y                = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10, 30]
sample_weights   = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
```

### Example 3

An undefined `indices` Series fails cleanly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});

  const cls::StatusOr<cls::FeatureBatch> sliced = cls::slice_feature_batch(batch, pluss::Series::undefined());
  std::cout << "ok = " << std::boolalpha << sliced.ok() << "\n";
  std::cout << "message = \"" << sliced.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "slice_feature_batch requires defined indices."
```

## Changelog

- **0.1** — `slice_feature_batch`, `slice_labeled_batch`.

## See Also

- [`IndexSplit`](IndexSplit.md) — the usual source of `indices`.
- [`FeatureBatch`](../data/FeatureBatch.md) / [`LabeledBatch`](../data/LabeledBatch.md) —
  the sliced types.
