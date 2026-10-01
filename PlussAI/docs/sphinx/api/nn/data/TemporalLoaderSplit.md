# `pluss::loader::TemporalLoaderSplit`

A plain result struct: the forward-chaining train/test
[`DataLoader`](DataLoader.md) folds produced by one
[`temporal_split_loaders`](temporal_split_loaders.md) call --
[`temporal_split_fields`](temporal_split_fields.md)'s own per-fold
parts, each already wrapped in a `DataLoader`. Never constructed
directly by calling code -- every instance comes back from
`temporal_split_loaders`.

## Signature

```cpp
pluss::loader::TemporalLoaderSplit::TemporalLoaderSplit() = default;
```

A plain aggregate -- `folds` defaults to an empty vector, but a
default-constructed instance is never meaningful on its own; every real
instance comes from
[`temporal_split_loaders`](temporal_split_loaders.md). Each
`TemporalLoaderFold` holds two `DataLoader`s, so (like `DataLoader`
itself) `TemporalLoaderSplit` is move-only.

## State

### `folds`

`std::vector<TemporalLoaderFold> folds`
: One entry per fold, in ascending time order (earliest test window
  first). Each `TemporalLoaderFold` is itself a plain struct with two
  fields:
  - `train` (`DataLoader`) -- a `DataLoader` over that fold's
    `TemporalFieldFold::train_fields`, constructed with whichever
    `train_options` was passed to `temporal_split_loaders`.
  - `test` (`DataLoader`) -- a `DataLoader` over that fold's
    `TemporalFieldFold::test_fields`, constructed with whichever
    `test_options` was passed to `temporal_split_loaders`.

## Methods

None -- `TemporalLoaderSplit` is a plain data result; every field is
public and read directly, see `## State` above.

## Practical Examples

One example: three folds with a small training batch size, reading the
first fold's loaders.

### Example 1

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

## Changelog

- **0.1** — the per-fold `train`/`test` `DataLoader` result of
  `temporal_split_loaders`.

## See Also

- [`temporal_split_loaders`](temporal_split_loaders.md) -- produces this
  result.
- [`TemporalFieldSplit`](TemporalFieldSplit.md) -- the underlying
  field-level folds, before each part is wrapped in a `DataLoader`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
