# `pluss::loader::temporal_split_loaders`

[`temporal_split_fields`](temporal_split_fields.md) plus a
[`DataLoader`](DataLoader.md) per fold's train/test part.

## Signature

```cpp
pluss::loader::TemporalLoaderSplit pluss::loader::temporal_split_loaders(const std::vector<pluss::Series>& fields, pluss::loader::TemporalSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions test_options = {});
pluss::loader::TemporalLoaderSplit pluss::loader::temporal_split_loaders(std::initializer_list<pluss::Series> fields, pluss::loader::TemporalSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions test_options = {});
pluss::loader::TemporalLoaderSplit pluss::loader::temporal_split_loaders(const pluss::loader::SeriesDataset& dataset, pluss::loader::TemporalSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions test_options = {});
pluss::loader::TemporalLoaderSplit pluss::loader::temporal_split_loaders(const pluss::loader::Dataset& dataset, pluss::loader::TemporalSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions test_options = {});
```

## What It Does

Calls [`temporal_split_fields`](temporal_split_fields.md) with
`split_options`, then constructs one `train`/`test` `DataLoader` pair
per resulting fold -- each from that fold's own fields and
`train_options`/`test_options` respectively. Every loader's
`options.batch_dim` is overwritten with `split_options.batch_dim`
(itself the dataset's own `batch_dim()`, for the two dataset overloads),
so batching always operates along the same axis the split itself used.

Unlike [`split_loaders`](split_loaders.md) (which produces one
train/val/test trio), this produces one train/test pair *per fold* --
`TemporalLoaderSplit::folds` has `split_options.folds` entries.

## Parameters

`const std::vector<Series>& fields` / `std::initializer_list<Series> fields`
: The aligned fields to split, as in
  [`temporal_split_fields`](temporal_split_fields.md).

`const SeriesDataset& dataset` / `const Dataset& dataset`
: A dataset to split by its own fields, as in
  [`temporal_split_fields`](temporal_split_fields.md).

`TemporalSplitOptions split_options = {}`
: Fold count, test size, max train size, gap, and batch dimension the
  split itself is computed with -- see
  [`TemporalSplitOptions`](TemporalSplitOptions.md).

`DataLoaderOptions train_options = {}` / `test_options = {}`
: The options each fold's `train`/`test` `DataLoader` is constructed
  with -- independent of each other and of `split_options`, aside from
  `batch_dim` being overwritten as described above.

## Returns

[`TemporalLoaderSplit`](TemporalLoaderSplit.md) -- one `train`/`test`
`DataLoader` pair per fold.

## Errors

Same preconditions as
[`temporal_split_fields`](temporal_split_fields.md) -- thrown before
any `DataLoader` is constructed.

## Practical Examples

Four examples, one per overload: a plain `std::vector<Series>` split
with a custom training batch size, a brace-literal split with distinct
per-part batch sizes read from the last fold, a `SeriesDataset` overload
batching two aligned fields together, and a `Dataset` overload over a
subclass that only implements `get()`.

### Example 1

The `std::vector<Series>` overload, with a small training batch size.

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

  loader::TemporalSplitOptions split_options{};
  split_options.folds = 3;
  split_options.test_size = 2;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  loader::TemporalLoaderSplit loaders = loader::temporal_split_loaders(fields, split_options, train_options);

  std::cout << "fold_count = " << loaders.folds.size() << "\n";
  std::cout << "fold 0 train.sample_count() = " << loaders.folds[0].train.sample_count() << "\n";
  std::cout << "fold 0 train.batch_count() = " << loaders.folds[0].train.batch_count() << "\n";
  std::cout << "fold 0 test.sample_count() = " << loaders.folds[0].test.sample_count() << "\n";

  return 0;
}
```

Output

```text
fold_count = 3
fold 0 train.sample_count() = 4
fold 0 train.batch_count() = 2
fold 0 test.sample_count() = 2
```

### Example 2

The `std::initializer_list<Series>` overload, with a distinct
`batch_size` per part, reading the last fold.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});

  loader::TemporalSplitOptions split_options{};
  split_options.folds = 3;
  split_options.test_size = 2;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;
  loader::DataLoaderOptions test_options{};
  test_options.batch_size = 1;

  loader::TemporalLoaderSplit loaders = loader::temporal_split_loaders({x}, split_options, train_options, test_options);

  std::cout << "fold 2 train.batch_size() = " << loaders.folds[2].train.batch_size() << "\n";
  std::cout << "fold 2 test.batch_size() = " << loaders.folds[2].test.batch_size() << "\n";
  std::cout << "fold 2 test.batch_count() = " << loaders.folds[2].test.batch_count() << "\n";

  return 0;
}
```

Output

```text
fold 2 train.batch_size() = 2
fold 2 test.batch_size() = 1
fold 2 test.batch_count() = 2
```

### Example 3

The `SeriesDataset` overload, over two aligned fields (`x`/`y`) -- each
fold's loaders batch both fields together.

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

  loader::TemporalSplitOptions split_options{};
  split_options.folds = 3;
  split_options.test_size = 2;

  loader::TemporalLoaderSplit loaders = loader::temporal_split_loaders(dataset, split_options);

  const auto batch = loaders.folds[0].train.batch_at(0);
  std::cout << "fold 0 train field_count() = " << loaders.folds[0].train.field_count() << "\n";
  std::cout << "fold 0 train first batch x:\n" << batch.x() << "\n";
  std::cout << "fold 0 train first batch y:\n" << batch.y() << "\n";

  return 0;
}
```

Output

```text
fold 0 train field_count() = 2
fold 0 train first batch x:
pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0],
  [1],
  [2],
  [3]]
fold 0 train first batch y:
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

  loader::TemporalSplitOptions split_options{};
  split_options.folds = 3;
  split_options.test_size = 2;

  loader::TemporalLoaderSplit loaders = loader::temporal_split_loaders(dataset, split_options);

  std::cout << "fold_count = " << loaders.folds.size() << "\n";
  std::cout << "fold 0 train.sample_count() = " << loaders.folds[0].train.sample_count() << "\n";
  std::cout << "fold 0 test.sample_count() = " << loaders.folds[0].test.sample_count() << "\n";

  return 0;
}
```

Output

```text
fold_count = 3
fold 0 train.sample_count() = 4
fold 0 test.sample_count() = 2
```

## See Also

- [`TemporalLoaderSplit`](TemporalLoaderSplit.md) -- the result
  returned.
- [`temporal_split_fields`](temporal_split_fields.md) -- the
  field-level split this wraps.
- [`split_loaders`](split_loaders.md) -- the plain random-split
  alternative for i.i.d. data.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
