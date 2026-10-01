# `pluss::loader::split_loaders`

[`split_fields`](split_fields.md) plus a [`DataLoader`](DataLoader.md)
per part, each with its own [`DataLoaderOptions`](DataLoaderOptions.md).

## Signature

```cpp
pluss::loader::DataLoaderSplit pluss::loader::split_loaders(const std::vector<pluss::Series>& fields, pluss::loader::DataSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions val_options = {}, pluss::loader::DataLoaderOptions test_options = {});
pluss::loader::DataLoaderSplit pluss::loader::split_loaders(std::initializer_list<pluss::Series> fields, pluss::loader::DataSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions val_options = {}, pluss::loader::DataLoaderOptions test_options = {});
pluss::loader::DataLoaderSplit pluss::loader::split_loaders(const pluss::loader::SeriesDataset& dataset, pluss::loader::DataSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions val_options = {}, pluss::loader::DataLoaderOptions test_options = {});
pluss::loader::DataLoaderSplit pluss::loader::split_loaders(const pluss::loader::Dataset& dataset, pluss::loader::DataSplitOptions split_options = {}, pluss::loader::DataLoaderOptions train_options = {}, pluss::loader::DataLoaderOptions val_options = {}, pluss::loader::DataLoaderOptions test_options = {});
```

## What It Does

Calls [`split_fields`](split_fields.md) with `split_options`, then
constructs one `DataLoader` per resulting part -- `train`/`val`/`test`
-- each from that part's own fields and its own `DataLoaderOptions`
(`train_options`/`val_options`/`test_options` respectively). Every
loader's `options.batch_dim` is overwritten with `split_options.batch_dim`
(itself the dataset's own `batch_dim()`, for the two dataset overloads),
so batching always operates along the same axis the split itself used.

A common shape for this is a shuffling `train_options` (for
training-time epoch shuffling) alongside unshuffled `val_options`/
`test_options` (for reproducible evaluation) -- see Example 2.

## Parameters

`const std::vector<Series>& fields` / `std::initializer_list<Series> fields`
: The aligned fields to split, as in [`split_fields`](split_fields.md).

`const SeriesDataset& dataset` / `const Dataset& dataset`
: A dataset to split by its own fields, as in
  [`split_fields`](split_fields.md).

`DataSplitOptions split_options = {}`
: The ratios, shuffle flag, seed, and batch dimension the split itself
  is computed with -- see [`DataSplitOptions`](DataSplitOptions.md).

`DataLoaderOptions train_options = {}` / `val_options = {}` / `test_options = {}`
: The options each corresponding `DataLoader` is constructed with --
  independent of each other and of `split_options`, aside from
  `batch_dim` being overwritten as described above.

## Returns

[`DataLoaderSplit`](DataLoaderSplit.md) -- the three-`DataLoader` trio.

## Errors

Same preconditions as [`split_fields`](split_fields.md) -- thrown before
any `DataLoader` is constructed.

## Practical Examples

Four examples, one per overload: a plain `std::vector<Series>` split
with a custom training batch size, a brace-literal split with distinct
per-part batch sizes, a `SeriesDataset` overload batching two aligned
fields together, and a `Dataset` overload over a subclass that only
implements `get()`.

### Example 1

The `std::vector<Series>` overload, with a small training batch size
and every other option left at its default.

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

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.shuffle = false;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  loader::DataLoaderSplit loaders = loader::split_loaders(fields, split_options, train_options);

  std::cout << "train.sample_count() = " << loaders.train.sample_count() << "\n";
  std::cout << "train.batch_count() = " << loaders.train.batch_count() << "\n";
  std::cout << "val.sample_count() = " << loaders.val.sample_count() << "\n";
  std::cout << "test.sample_count() = " << loaders.test.sample_count() << "\n";

  return 0;
}
```

Output

```text
train.sample_count() = 6
train.batch_count() = 3
val.sample_count() = 2
test.sample_count() = 2
```

### Example 2

The `std::initializer_list<Series>` overload, with a distinct
`batch_size` per part -- `test_options` left at its default (`32`).

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.shuffle = false;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;
  loader::DataLoaderOptions val_options{};
  val_options.batch_size = 1;

  loader::DataLoaderSplit loaders = loader::split_loaders({x}, split_options, train_options, val_options);

  std::cout << "train.batch_size() = " << loaders.train.batch_size() << "\n";
  std::cout << "val.batch_size() = " << loaders.val.batch_size() << "\n";
  std::cout << "test.batch_size() = " << loaders.test.batch_size() << " (default)\n";

  return 0;
}
```

Output

```text
train.batch_size() = 2
val.batch_size() = 1
test.batch_size() = 32 (default)
```

### Example 3

The `SeriesDataset` overload, over two aligned fields (`x`/`y`) -- each
returned loader batches both fields together.

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

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.shuffle = false;

  loader::DataLoaderSplit loaders = loader::split_loaders(dataset, split_options);

  const auto batch = loaders.train.batch_at(0);
  std::cout << "train field_count() = " << loaders.train.field_count() << "\n";
  std::cout << "train first batch x:\n" << batch.x() << "\n";
  std::cout << "train first batch y:\n" << batch.y() << "\n";

  return 0;
}
```

Output

```text
train field_count() = 2
train first batch x:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [1],
  [2],
  [3],
  [4],
  [5]]
train first batch y:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [10],
  [20],
  [30],
  [40],
  [50]]
```

### Example 4

The `Dataset` overload, over a subclass that only implements `get()` --
the same [`Dataset::materialize_fields`](Dataset/materialize_fields.md)
get()-fallback `split_fields` itself relies on applies here too, so a
minimal `Dataset` subclass works with `split_loaders` out of the box.

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

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.shuffle = false;

  loader::DataLoaderSplit loaders = loader::split_loaders(dataset, split_options);

  std::cout << "train.sample_count() = " << loaders.train.sample_count() << "\n";
  std::cout << "val.sample_count() = " << loaders.val.sample_count() << "\n";
  std::cout << "test.sample_count() = " << loaders.test.sample_count() << "\n";

  return 0;
}
```

Output

```text
train.sample_count() = 6
val.sample_count() = 2
test.sample_count() = 2
```

## See Also

- [`DataLoaderSplit`](DataLoaderSplit.md) -- the result returned.
- [`split_fields`](split_fields.md) -- the field-level split this wraps.
- [`temporal_split_loaders`](temporal_split_loaders.md) -- the
  forward-chaining alternative for time-series data.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
