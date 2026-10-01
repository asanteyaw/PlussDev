# `pluss::loader::TemporalIndexSplit`

A plain result struct: the forward-chaining train/test index folds
produced by one [`temporal_split_indices`](temporal_split_indices.md)
call. Never constructed directly by calling code -- every instance
comes back from `temporal_split_indices`.

## Signature

```cpp
pluss::loader::TemporalIndexSplit::TemporalIndexSplit() = default;
```

A plain aggregate -- `folds` defaults to an empty vector, but a
default-constructed instance is never meaningful on its own; every real
instance comes from
[`temporal_split_indices`](temporal_split_indices.md).

## State

### `folds`

`std::vector<TemporalIndexFold> folds`
: One entry per fold, in ascending time order (earliest test window
  first). Each `TemporalIndexFold` is itself a plain struct with two
  fields:
  - `train_indices` (`std::vector<std::int64_t>`) -- the sample indices
    this fold trains on, strictly before its own test window (plus
    `TemporalSplitOptions::gap`, if set).
  - `test_indices` (`std::vector<std::int64_t>`) -- the sample indices
    this fold evaluates on.

## Methods

None -- `TemporalIndexSplit` is a plain data result; every field is
public and read directly, see `## State` above.

## Practical Examples

One example: three folds over ten samples, with an explicit test size.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::TemporalSplitOptions options{};
  options.folds = 3;
  options.test_size = 2;

  loader::TemporalIndexSplit split = loader::temporal_split_indices(10, options);

  for (std::size_t i = 0; i < split.folds.size(); ++i) {
    const auto& fold = split.folds[i];
    std::cout << "fold " << i << " train_indices = [";
    for (auto v : fold.train_indices) std::cout << v << " ";
    std::cout << "] test_indices = [";
    for (auto v : fold.test_indices) std::cout << v << " ";
    std::cout << "]\n";
  }

  return 0;
}
```

Output

```text
fold 0 train_indices = [0 1 2 3 ] test_indices = [4 5 ]
fold 1 train_indices = [0 1 2 3 4 5 ] test_indices = [6 7 ]
fold 2 train_indices = [0 1 2 3 4 5 6 7 ] test_indices = [8 9 ]
```

## Changelog

- **0.1** — the per-fold `train_indices`/`test_indices` result of
  `temporal_split_indices`.

## See Also

- [`temporal_split_indices`](temporal_split_indices.md) -- produces this
  result.
- [`TemporalSplitOptions`](TemporalSplitOptions.md) -- controls fold
  count, test size, max train size, gap, and batch dimension.
- [`TemporalFieldSplit`](TemporalFieldSplit.md) -- the same folds,
  applied directly to field `Series`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
