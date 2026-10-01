# `pluss::loader::temporal_split_fields`

[`temporal_split_indices`](temporal_split_indices.md) applied directly
to aligned field `Series` (or a
[`Dataset`](Dataset.md)/[`SeriesDataset`](SeriesDataset.md)), returning
per-fold train/test `Series`.

## Signature

```cpp
pluss::loader::TemporalFieldSplit pluss::loader::temporal_split_fields(const std::vector<pluss::Series>& fields, pluss::loader::TemporalSplitOptions options = {});
pluss::loader::TemporalFieldSplit pluss::loader::temporal_split_fields(std::initializer_list<pluss::Series> fields, pluss::loader::TemporalSplitOptions options = {});
pluss::loader::TemporalFieldSplit pluss::loader::temporal_split_fields(const pluss::loader::SeriesDataset& dataset, pluss::loader::TemporalSplitOptions options = {});
pluss::loader::TemporalFieldSplit pluss::loader::temporal_split_fields(const pluss::loader::Dataset& dataset, pluss::loader::TemporalSplitOptions options = {});
```

## What It Does

Computes `options.folds` forward-chaining folds via
[`temporal_split_indices`](temporal_split_indices.md) over the shared
sample axis, then applies each fold's `train_indices`/`test_indices` to
every field via `index_select`, so every field's train/test parts stay
aligned to each other within each fold.

The two `Dataset`/`SeriesDataset` overloads first materialize the
dataset's fields (via `materialize_fields()` when overridden, falling
back to one `get(i)` call per sample otherwise -- see
[`Dataset::materialize_fields`](Dataset/materialize_fields.md)) and
overwrite `options.batch_dim` with the dataset's own `batch_dim()`
before splitting, exactly as [`split_fields`](split_fields.md) does.

## Parameters

`const std::vector<Series>& fields` / `std::initializer_list<Series> fields`
: The aligned fields to split -- every field must be defined, rank
  `>= 1`, and share the same size along `options.batch_dim`.

`const SeriesDataset& dataset` / `const Dataset& dataset`
: A dataset to split by its own fields, instead of raw `Series` passed
  directly.

`TemporalSplitOptions options = {}`
: Fold count, test size, max train size, gap, and batch dimension -- see
  [`TemporalSplitOptions`](TemporalSplitOptions.md).

## Returns

[`TemporalFieldSplit`](TemporalFieldSplit.md) -- the per-fold
`train_fields`/`test_fields`.

## Errors

Same preconditions as
[`temporal_split_indices`](temporal_split_indices.md), plus
[`split_fields`](split_fields.md)'s own field-alignment checks (empty
`fields`, an undefined or rank-`0` field, or fields disagreeing on their
size along `options.batch_dim`).

## Practical Examples

Four examples, one per overload: a plain `std::vector<Series>` split, a
brace-literal split read from a later fold, a `SeriesDataset` overload
splitting two aligned fields together, and a `Dataset` overload over a
subclass that only implements `get()`.

### Example 1

The `std::vector<Series>` overload, reading the first fold.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});
  const std::vector<pluss::Series> fields{x};

  loader::TemporalSplitOptions options{};
  options.folds = 3;
  options.test_size = 2;

  loader::TemporalFieldSplit split = loader::temporal_split_fields(fields, options);

  std::cout << "fold_count = " << split.folds.size() << "\n";
  std::cout << "fold 0 train_fields[0]:\n" << split.folds[0].train_fields[0] << "\n";
  std::cout << "fold 0 test_fields[0]:\n" << split.folds[0].test_fields[0] << "\n";

  return 0;
}
```

Output

```text
fold_count = 3
fold 0 train_fields[0]:
pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0],
  [1],
  [2],
  [3]]
fold 0 test_fields[0]:
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[4],
  [5]]
```

### Example 2

The `std::initializer_list<Series>` overload (a brace literal directly
in the call), reading the second fold -- its training window has grown
to include the first fold's test window too.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});

  loader::TemporalSplitOptions options{};
  options.folds = 3;
  options.test_size = 2;

  loader::TemporalFieldSplit split = loader::temporal_split_fields({x}, options);
  std::cout << "fold 1 train_fields[0]:\n" << split.folds[1].train_fields[0] << "\n";
  std::cout << "fold 1 test_fields[0]:\n" << split.folds[1].test_fields[0] << "\n";

  return 0;
}
```

Output

```text
fold 1 train_fields[0]:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [1],
  [2],
  [3],
  [4],
  [5]]
fold 1 test_fields[0]:
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[6],
  [7]]
```

### Example 3

The `SeriesDataset` overload, splitting two aligned fields (`x`/`y`)
together -- both fields' parts stay aligned within each fold.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0}, {10, 1});
  loader::SeriesDataset dataset({x, y});

  loader::TemporalSplitOptions options{};
  options.folds = 3;
  options.test_size = 2;

  loader::TemporalFieldSplit split = loader::temporal_split_fields(dataset, options);

  std::cout << "fold 0 train_fields[0] (x):\n" << split.folds[0].train_fields[0] << "\n";
  std::cout << "fold 0 train_fields[1] (y):\n" << split.folds[0].train_fields[1] << "\n";

  return 0;
}
```

Output

```text
fold 0 train_fields[0] (x):
pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0],
  [1],
  [2],
  [3]]
fold 0 train_fields[1] (y):
pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0],
  [10],
  [20],
  [30]]
```

### Example 4

The `Dataset` overload, over a subclass that only implements `get()` --
the same get()-fallback [`split_fields`](split_fields.md) relies on
applies here too.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

namespace loader = pluss::loader;

class ScalarDataset final : public loader::Dataset {
 public:
  explicit ScalarDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }
  loader::DataBatch get(std::int64_t index) const override {
    loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }

 private:
  std::vector<double> values_;
};

int main() {
  ScalarDataset dataset({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});

  loader::TemporalSplitOptions options{};
  options.folds = 3;
  options.test_size = 2;

  loader::TemporalFieldSplit split = loader::temporal_split_fields(dataset, options);

  std::cout << "fold_count = " << split.folds.size() << "\n";
  std::cout << "fold 0 train_fields[0]:\n" << split.folds[0].train_fields[0] << "\n";
  std::cout << "fold 0 test_fields[0]:\n" << split.folds[0].test_fields[0] << "\n";

  return 0;
}
```

Output

```text
fold_count = 3
fold 0 train_fields[0]:
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 3]
fold 0 test_fields[0]:
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 5]
```

## See Also

- [`TemporalFieldSplit`](TemporalFieldSplit.md) -- the result returned.
- [`temporal_split_indices`](temporal_split_indices.md) -- the
  underlying index computation.
- [`temporal_split_loaders`](temporal_split_loaders.md) -- this plus a
  [`DataLoader`](DataLoader.md) per fold's train/test part.
- [`split_fields`](split_fields.md) -- the plain random-split
  alternative for i.i.d. data.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
