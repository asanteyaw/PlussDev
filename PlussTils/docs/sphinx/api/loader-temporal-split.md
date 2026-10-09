# `pluss::loader::temporal_split_indices, temporal_split_fields, and temporal_split_loaders`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

```cpp
#include <pluss/tils/aux/data_loader.h>

struct TemporalSplitOptions {
  std::int64_t folds{5};
  std::optional<std::int64_t> test_size{};
  std::optional<std::int64_t> max_train_size{};
  std::int64_t gap{0};
  std::int64_t batch_dim{0};
};

struct TemporalIndexFold { std::vector<std::int64_t> train_indices, test_indices; };
struct TemporalIndexSplit { std::vector<TemporalIndexFold> folds; };
TemporalIndexSplit temporal_split_indices(std::int64_t sample_count, TemporalSplitOptions options = {});

struct TemporalFieldFold { std::vector<Series> train_fields, test_fields; };
struct TemporalFieldSplit { std::vector<TemporalFieldFold> folds; };
TemporalFieldSplit temporal_split_fields(const std::vector<Series>& fields, TemporalSplitOptions options = {});

struct TemporalLoaderFold { DataLoader train, test; };
struct TemporalLoaderSplit { std::vector<TemporalLoaderFold> folds; };
TemporalLoaderSplit temporal_split_loaders(const std::vector<Series>& fields, TemporalSplitOptions split_options = {},
                                           DataLoaderOptions train_options = {}, DataLoaderOptions test_options = {});
```

These three functions produce rolling-origin folds for time-ordered data, the same idea as scikit-learn's `TimeSeriesSplit`: no shuffling anywhere, and each fold's test window comes later in the sequence than its train window. `temporal_split_indices` computes the plan as index lists; `temporal_split_fields` applies the same plan to real `Series` fields; `temporal_split_loaders` goes one step further and wraps each fold's train and test fields in their own `DataLoader`.

`folds` sets how many train/test pairs to produce. `test_size` sets how many samples each test window holds; left unset, it is `sample_count / (folds + 1)`. The test windows do not overlap, and together they cover the last `folds * test_size` samples of the sequence, earliest fold first. A fold's train window is everything before its test window, from the very start, unless `max_train_size` caps it, in which case the window slides forward instead of growing. `gap` leaves that many samples between the end of a fold's train window and the start of its test window, untouched by either.

`temporal_split_fields` and `temporal_split_loaders` also accept an initializer list, a `SeriesDataset`, or a `Dataset`, the same way `split_fields` does, reading `batch_dim` from the dataset for those overloads.

## Parameters

| Parameter | Meaning |
|---|---|
| `sample_count` | The total number of samples, for `temporal_split_indices`. |
| `fields` | The data to fold, for the other two functions. |
| `options` / `split_options` | The fold count, test size, gap, and cap on the train window. |
| `train_options`, `test_options` | The `DataLoaderOptions` for each fold's two loaders, for `temporal_split_loaders`. |

## Returns

A `TemporalIndexSplit`, `TemporalFieldSplit`, or `TemporalLoaderSplit`, each holding one fold per element of `folds`.

## Errors

`sample_count` must be at least 2, `folds` at least 2, and the data must hold enough samples to fit every fold's test window and the gap between them; an impossible combination is refused.

## Practical Examples

### Example 1

Example 1 computes four rolling folds over twenty samples.

```cpp
// temporal_split_indices produces several train/test folds that each move forward in time: the
// train indices for a later fold include everything an earlier fold's train and test together
// covered, and the test window always comes immediately after the training window.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  loader::TemporalSplitOptions options{};
  options.folds = 4;
  options.test_size = 3;

  const loader::TemporalIndexSplit split = loader::temporal_split_indices(20, options);
  std::cout << "fold count: " << split.folds.size() << '\n';
  for (std::size_t i = 0; i < split.folds.size(); ++i) {
    const auto& fold = split.folds[i];
    std::cout << "fold " << i << ": train 0.." << fold.train_indices.size()
              << " test " << fold.test_indices.front() << ".." << fold.test_indices.back() << '\n';
  }
  return 0;
}
```

Output

```text
fold count: 4
fold 0: train 0..8 test 8..10
fold 1: train 0..11 test 11..13
fold 2: train 0..14 test 14..16
fold 3: train 0..17 test 17..19
```

Each fold's test window starts exactly where the previous fold's test window ended, and every fold's train window starts at zero and grows.

### Example 2

Example 2 adds a gap between training and testing.

```cpp
// gap leaves a buffer of untouched indices between the end of the training window and the start of
// the test window, so a value right at the boundary never leaks between the two.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  loader::TemporalSplitOptions no_gap{};
  no_gap.folds = 2;
  no_gap.test_size = 3;
  const auto without = loader::temporal_split_indices(20, no_gap);

  loader::TemporalSplitOptions with_gap = no_gap;
  with_gap.gap = 2;
  const auto with = loader::temporal_split_indices(20, with_gap);

  std::cout << "without gap, fold 0 train size: " << without.folds[0].train_indices.size() << '\n';
  std::cout << "with gap, fold 0 train size: " << with.folds[0].train_indices.size() << '\n';
  std::cout << "both folds' test windows start the same: "
            << (without.folds[0].test_indices.front() == with.folds[0].test_indices.front()) << '\n';
  return 0;
}
```

Output

```text
without gap, fold 0 train size: 14
with gap, fold 0 train size: 12
both folds' test windows start the same: 1
```

The gap shortens the training window by the gap size, and the test window's start is unaffected.

### Example 3

Example 3 caps the training window with max_train_size.

```cpp
// Without max_train_size, each later fold's training window grows to include everything before it.
// With it set, the window is capped to a fixed size, so it slides forward instead of growing.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  loader::TemporalSplitOptions growing{};
  growing.folds = 3;
  growing.test_size = 3;
  const auto expanding = loader::temporal_split_indices(20, growing);

  loader::TemporalSplitOptions capped = growing;
  capped.max_train_size = 5;
  const auto fixed = loader::temporal_split_indices(20, capped);

  std::cout << "expanding window sizes: ";
  for (const auto& fold : expanding.folds) std::cout << fold.train_indices.size() << ' ';
  std::cout << '\n';
  std::cout << "capped window sizes: ";
  for (const auto& fold : fixed.folds) std::cout << fold.train_indices.size() << ' ';
  std::cout << '\n';
  std::cout << "capped windows never exceed 5: " << (fixed.folds.back().train_indices.size() <= 5) << '\n';
  return 0;
}
```

Output

```text
expanding window sizes: 11 14 17 
capped window sizes: 5 5 5 
capped windows never exceed 5: true
```

Without the cap, each fold's window is larger than the last; with it, every fold's window is the same fixed size.

### Example 4

Example 4 applies the same fold plan to real Series fields.

```cpp
// temporal_split_fields applies the same fold plan to real Series fields, so each fold's train and
// test are ready-to-use data, in time order, with no shuffling anywhere in the process.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(12);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {12}, cfg);

  loader::TemporalSplitOptions options{};
  options.folds = 2;
  options.test_size = 3;
  const loader::TemporalFieldSplit split = loader::temporal_split_fields({x}, options);

  std::cout << "fold count: " << split.folds.size() << '\n';
  const auto& last_fold = split.folds.back();
  std::cout << "last fold train size: " << last_fold.train_fields[0].numel() << '\n';
  std::cout << "last fold test size: " << last_fold.test_fields[0].numel() << '\n';
  std::cout << "last fold test is the final three values: "
            << (last_fold.test_fields[0].to_vec<double>() == std::vector<double>{9, 10, 11}) << '\n';
  return 0;
}
```

Output

```text
fold count: 2
last fold train size: 9
last fold test size: 3
last fold test is the final three values: true
```

The last fold's test values are exactly the final three values of the series, in their original order.

### Example 5

Example 5 wraps each fold's fields in train and test loaders.

```cpp
// temporal_split_loaders wraps each fold's train and test fields in their own DataLoader, ready to
// iterate, the temporal counterpart to split_loaders.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(12);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {12}, cfg);

  loader::TemporalSplitOptions split_options{};
  split_options.folds = 2;
  split_options.test_size = 3;
  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  const loader::TemporalLoaderSplit split = loader::temporal_split_loaders({x}, split_options, train_options);
  std::cout << "fold count: " << split.folds.size() << '\n';
  std::cout << "fold 0 train batches of two: " << split.folds[0].train.batch_count() << '\n';
  std::cout << "fold 0 test samples: " << split.folds[0].test.sample_count() << '\n';
  return 0;
}
```

Output

```text
fold count: 2
fold 0 train batches of two: 3
fold 0 test samples: 3
```

Each fold's train loader reports batches by its own batch size, and its test loader reports the fold's test sample count.

## See Also

- [Ordinary splitting](loader-split.md) -- the ratio-based, shuffled counterpart.
