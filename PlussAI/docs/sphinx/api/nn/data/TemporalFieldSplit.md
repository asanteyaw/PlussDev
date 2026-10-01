# `pluss::loader::TemporalFieldSplit`

A plain result struct: the forward-chaining train/test field `Series`
produced by one [`temporal_split_fields`](temporal_split_fields.md)
call -- [`temporal_split_indices`](temporal_split_indices.md)'s own
index folds, applied directly to field `Series`. Never constructed
directly by calling code -- every instance comes back from
`temporal_split_fields`.

## Signature

```cpp
pluss::loader::TemporalFieldSplit::TemporalFieldSplit() = default;
```

A plain aggregate -- `folds` defaults to an empty vector, but a
default-constructed instance is never meaningful on its own; every real
instance comes from
[`temporal_split_fields`](temporal_split_fields.md).

## State

### `folds`

`std::vector<TemporalFieldFold> folds`
: One entry per fold, in ascending time order (earliest test window
  first). Each `TemporalFieldFold` is itself a plain struct with two
  fields:
  - `train_fields` (`std::vector<Series>`) -- this fold's training-part
    field `Series`, one per input field, in the same order as the
    fields passed to `temporal_split_fields`.
  - `test_fields` (`std::vector<Series>`) -- this fold's test-part
    field `Series`, same order and count as `train_fields`.

## Methods

None -- `TemporalFieldSplit` is a plain data result; every field is
public and read directly, see `## State` above.

## Practical Examples

One example: two folds over a single field, with an explicit test size.

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

## Changelog

- **0.1** — the per-fold `train_fields`/`test_fields` result of
  `temporal_split_fields`.

## See Also

- [`temporal_split_fields`](temporal_split_fields.md) -- produces this
  result.
- [`TemporalIndexSplit`](TemporalIndexSplit.md) -- the underlying index
  folds, before being applied to field `Series`.
- [`TemporalLoaderSplit`](TemporalLoaderSplit.md) -- these folds, each
  wrapped in a `DataLoader`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
