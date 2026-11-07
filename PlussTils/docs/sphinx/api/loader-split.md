# `pluss::loader::split_fields and split_loaders`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

## Signature

```cpp
#include <pluss/tils/kit/data_loader.h>

DataSplit split_fields(const std::vector<Series>& fields, DataSplitOptions options = {});
DataSplit split_fields(const SeriesDataset& dataset, DataSplitOptions options = {});
DataSplit split_fields(const Dataset& dataset, DataSplitOptions options = {});

DataLoaderSplit split_loaders(const std::vector<Series>& fields, DataSplitOptions split_options = {},
                               DataLoaderOptions train_options = {}, DataLoaderOptions val_options = {},
                               DataLoaderOptions test_options = {});
```

## What It Does

`split_fields` partitions a dataset into three parts, train, validation, and test, by ratio: `train_ratio` and `val_ratio` set the first two shares, and the rest becomes test. With `shuffle` left at its default, the rows are shuffled once, with `seed`, before the cut, so every row still appears in exactly one of the three parts. The same input, options, and fields all accept a plain field list, an initializer list, a `SeriesDataset`, or any `Dataset`; the dataset overloads read `batch_dim` from the dataset itself rather than from the options.

`split_loaders` does the same partition and wraps each part in its own `DataLoader`, one call instead of three field lists to wrap yourself. Each of the three loaders takes its own `DataLoaderOptions`, so the train loader can shuffle its batches while the test loader does not, for example.

## Parameters

| Parameter | Meaning |
|---|---|
| `fields` / `dataset` | The data to split. |
| `options` / `split_options` | The ratios, shuffle, and seed for the partition. |
| `train_options`, `val_options`, `test_options` | The `DataLoaderOptions` for each of the three resulting loaders, for `split_loaders`. |

## `DataSplitOptions` Fields

| Field | Meaning | Default |
|---|---|---|
| `train_ratio` | Fraction of rows assigned to the train split. | `0.8` |
| `val_ratio` | Fraction of rows assigned to the validation split; the remainder becomes test. | `0.1` |
| `shuffle` | Whether rows are shuffled once, by `seed`, before the cut. | `true` |
| `seed` | The shuffle seed. | `0` |
| `batch_dim` | Which axis holds the samples, for the plain-field-list overload. Dataset overloads read this from the dataset instead. | `0` |

## Returns

`split_fields` returns a `DataSplit`: `train_fields`, `val_fields`, and `test_fields`, each a `std::vector<Series>`. `split_loaders` returns a `DataLoaderSplit`: `train`, `val`, and `test`, each a ready-to-use `DataLoader`.

## Errors

None beyond what `DataLoader`'s own construction refuses, when `split_loaders` builds the three loaders.

## Practical Examples

### Example 1

Example 1 splits a field list by ratio and checks the partition.

```cpp
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);
  std::vector<double> tens(values);
  for (auto& v : tens) v *= 10.0;
  const pluss::Series y = pluss::make_series(tens, {10}, cfg);

  // Sixty percent train, twenty percent validation. The remaining twenty percent is test.
  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.seed = 7;
  const loader::DataSplit split = loader::split_fields({x, y}, options);

  std::cout << "train rows: " << split.train_fields[0].sizes()[0] << '\n';
  std::cout << "val rows: " << split.val_fields[0].sizes()[0] << '\n';
  std::cout << "test rows: " << split.test_fields[0].sizes()[0] << '\n';

  // The three splits partition the input: every row appears exactly once.
  std::vector<double> all;
  for (const auto* part : {&split.train_fields[0], &split.val_fields[0], &split.test_fields[0]}) {
    const auto v = part->reshape({part->numel()}).host<double>();
    all.insert(all.end(), v.begin(), v.end());
  }
  std::sort(all.begin(), all.end());
  std::cout << "partition covers all rows once: " << (all == values) << '\n';

  // Fields stay aligned: y was built as ten times x, so every train row keeps that relation.
  const auto train_x = split.train_fields[0].reshape({split.train_fields[0].numel()}).pull<double>();
  const auto train_y = split.train_fields[1].reshape({split.train_fields[1].numel()}).pull<double>();
  bool aligned = train_x.size() == train_y.size();
  for (std::size_t i = 0; aligned && i < train_x.size(); ++i) aligned = train_y[i] == 10.0 * train_x[i];
  std::cout << "fields stay aligned: " << aligned << '\n';

  // The same seed gives the same split.
  const loader::DataSplit again = loader::split_fields({x, y}, options);
  std::cout << "same seed, same split: "
            << (again.train_fields[0].reshape({again.train_fields[0].numel()}).pull<double>() == train_x) << '\n';
}
```

Output

```text
train rows: 6
val rows: 2
test rows: 2
partition covers all rows once: true
fields stay aligned: true
same seed, same split: true
```

The three parts total the input row count exactly once each, the fields stay aligned with each other, and the same seed gives the same split.

### Example 2

Example 2 splits straight into three ready-to-use loaders.

```cpp
// split_loaders does the same partition as split_fields and wraps each part in its own DataLoader,
// so you get three ready-to-use loaders in one call instead of three field lists to wrap yourself.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.seed = 3;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  const loader::DataLoaderSplit loaders = loader::split_loaders({x}, split_options, train_options);
  std::cout << "train samples: " << loaders.train.sample_count() << '\n';
  std::cout << "val samples: " << loaders.val.sample_count() << '\n';
  std::cout << "test samples: " << loaders.test.sample_count() << '\n';
  std::cout << "train batches of two: " << loaders.train.batch_count() << '\n';
  return 0;
}
```

Output

```text
train samples: 6
val samples: 2
test samples: 2
train batches of two: 3
```

Each loader reports the sample count its own share of the split holds, and the train loader's batch count follows its own batch size.

### Example 3

Example 3 splits a SeriesDataset directly, instead of unpacking its fields first.

```cpp
// split_fields also accepts a Dataset or a SeriesDataset directly, reading its batch_dim from the
// dataset instead of the options, so a dataset you already built can be split without unpacking it.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);
  const loader::SeriesDataset dataset({x});

  loader::DataSplitOptions options{};
  options.train_ratio = 0.7;
  options.val_ratio = 0.1;
  const loader::DataSplit split = loader::split_fields(dataset, options);
  std::cout << "train rows: " << split.train_fields[0].numel() << '\n';
  std::cout << "val rows: " << split.val_fields[0].numel() << '\n';
  std::cout << "test rows: " << split.test_fields[0].numel() << '\n';
  return 0;
}
```

Output

```text
train rows: 7
val rows: 1
test rows: 2
```

The split reads the dataset's own batch_dim, and the three parts total its row count.

## See Also

- [Temporal splitting](loader-temporal-split.md) -- the time-ordered counterpart, with no shuffling.
- [`DataLoader`](loader-data-loader.md)
