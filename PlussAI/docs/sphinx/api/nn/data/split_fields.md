# `pluss::loader::split_fields`

A one-shot random train/val/test split of aligned field `Series` (or a
[`Dataset`](Dataset.md)/[`SeriesDataset`](SeriesDataset.md));
[`split_loaders`](split_loaders.md) wraps each part in a
[`DataLoader`](DataLoader.md).

## Signature

```cpp
pluss::loader::DataSplit pluss::loader::split_fields(const std::vector<pluss::Series>& fields, pluss::loader::DataSplitOptions options = {});
pluss::loader::DataSplit pluss::loader::split_fields(std::initializer_list<pluss::Series> fields, pluss::loader::DataSplitOptions options = {});
pluss::loader::DataSplit pluss::loader::split_fields(const pluss::loader::SeriesDataset& dataset, pluss::loader::DataSplitOptions options = {});
pluss::loader::DataSplit pluss::loader::split_fields(const pluss::loader::Dataset& dataset, pluss::loader::DataSplitOptions options = {});
```

## What It Does

Partitions a shared sample axis into three disjoint index sets --
`train`/`val`/`test`, sized by `options.train_ratio`/`val_ratio` (test is
the remainder) -- then applies that same partition to every field via
`index_select`, so every field's parts stay aligned to each other. With
`options.shuffle = true` (the default), the partition is a random
permutation seeded from `options.seed`; with `shuffle = false`, samples
are assigned in their original order (first `train_count` to `train`,
next `val_count` to `val`, the rest to `test`).

The two `Dataset`/`SeriesDataset` overloads first materialize the
dataset's fields (via `materialize_fields()` when overridden, falling
back to one `get(i)` call per sample otherwise -- see
[`Dataset::materialize_fields`](Dataset/materialize_fields.md)) and
overwrite `options.batch_dim` with the dataset's own `batch_dim()`
before splitting, so a dataset's declared sample axis is always honored
regardless of what `options.batch_dim` was set to.

## Parameters

`const std::vector<Series>& fields` / `std::initializer_list<Series> fields`
: The aligned fields to split -- every field must be defined, rank
  `>= 1`, and share the same size along `options.batch_dim`.

`const SeriesDataset& dataset` / `const Dataset& dataset`
: A dataset to split by its own fields, instead of raw `Series` passed
  directly.

`DataSplitOptions options = {}`
: The ratios, shuffle flag, seed, and batch dimension -- see
  [`DataSplitOptions`](DataSplitOptions.md).

## Returns

[`DataSplit`](DataSplit.md) -- the three parts' fields, in the same
order and count as the input fields.

## Errors

- `fields` empty, or any field undefined / rank `0` -- throws.
- Fields disagree on their size along `options.batch_dim` -- throws.
- `options.train_ratio`/`val_ratio` outside `[0, 1]`, or their sum
  exceeds `1.0` -- throws.

## Practical Examples

Four examples, one per overload: a plain `std::vector<Series>` split, a
brace-literal shuffled split with a fixed seed, a `SeriesDataset`
overload splitting two aligned fields together, and a `Dataset`
overload over a subclass that only implements `get()`.

### Example 1

The `std::vector<Series>` overload, unshuffled, with explicit ratios so
every part's size is visible directly in the split itself.

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

  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.shuffle = false;
  loader::DataSplit split = loader::split_fields(fields, options);

  std::cout << "train_fields[0]:\n" << split.train_fields[0] << "\n";
  std::cout << "val_fields[0]:\n" << split.val_fields[0] << "\n";
  std::cout << "test_fields[0]:\n" << split.test_fields[0] << "\n";

  return 0;
}
```

Output

```text
train_fields[0]:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [1],
  [2],
  [3],
  [4],
  [5]]
val_fields[0]:
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[6],
  [7]]
test_fields[0]:
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[8],
  [9]]
```

### Example 2

The `std::initializer_list<Series>` overload (a brace literal directly
in the call), shuffled with a fixed seed -- the same seed on the same
input always reproduces the identical split.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});

  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.shuffle = true;
  options.seed = 42;
  loader::DataSplit split = loader::split_fields({x}, options);
  std::cout << "train_fields[0]:\n" << split.train_fields[0] << "\n";

  loader::DataSplit split_again = loader::split_fields({x}, options);
  std::cout << "second call (same seed), train_fields[0]:\n" << split_again.train_fields[0] << "\n";

  return 0;
}
```

Output

```text
train_fields[0]:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[6],
  [9],
  [4],
  [1],
  [3],
  [2]]
second call (same seed), train_fields[0]:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[6],
  [9],
  [4],
  [1],
  [3],
  [2]]
```

### Example 3

The `SeriesDataset` overload, splitting two aligned fields (`x`/`y`)
together -- both fields' parts stay aligned to the same sample indices.

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

  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.shuffle = false;
  loader::DataSplit split = loader::split_fields(dataset, options);

  std::cout << "train_fields[0] (x):\n" << split.train_fields[0] << "\n";
  std::cout << "train_fields[1] (y):\n" << split.train_fields[1] << "\n";

  return 0;
}
```

Output

```text
train_fields[0] (x):
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [1],
  [2],
  [3],
  [4],
  [5]]
train_fields[1] (y):
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [10],
  [20],
  [30],
  [40],
  [50]]
```

### Example 4

The `Dataset` overload, over a subclass that only implements `get()`
(no `materialize_fields()` override) -- the fallback materializes one
field per sample via `get(i)` and unites them, so `split_fields` works
on it directly. Note the resulting shape: `get()` here returns one bare
scalar per sample, promoted to rank 1 by the fallback (see
[`Dataset::materialize_fields`](Dataset/materialize_fields.md)), so each
part comes out 1-D rather than the `[n, 1]` shape the earlier examples'
hand-built fields had.

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

  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.shuffle = false;
  loader::DataSplit split = loader::split_fields(dataset, options);

  std::cout << "train_fields[0]:\n" << split.train_fields[0] << "\n";
  std::cout << "val_fields[0]:\n" << split.val_fields[0] << "\n";
  std::cout << "test_fields[0]:\n" << split.test_fields[0] << "\n";

  return 0;
}
```

Output

```text
train_fields[0]:
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 1, 2, 3, 4, 5]
val_fields[0]:
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 7]
test_fields[0]:
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[8, 9]
```

## See Also

- [`DataSplit`](DataSplit.md) -- the result returned.
- [`DataSplitOptions`](DataSplitOptions.md) -- the ratios/shuffle/seed
  controlling the split.
- [`split_loaders`](split_loaders.md) -- this plus a
  [`DataLoader`](DataLoader.md) per part.
- [`temporal_split_fields`](temporal_split_fields.md) -- the
  forward-chaining alternative for time-series data.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
