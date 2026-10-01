# `pluss::loader::temporal_split_indices`

Forward-chaining (walk-forward) train/test index folds over a sample
count -- never trains on future data.

## Signature

```cpp
pluss::loader::TemporalIndexSplit pluss::loader::temporal_split_indices(std::int64_t sample_count, pluss::loader::TemporalSplitOptions options = {});
```

## What It Does

Produces `options.folds` folds of `(train_indices, test_indices)` pairs
over `[0, sample_count)`, each fold's test window strictly later in time
than that fold's own training indices (plus `options.gap`, if set), and
each successive fold's test window strictly later than the previous
fold's. This is what makes it safe for time series: unlike
[`split_fields`](split_fields.md)'s plain random split, no fold ever
trains on a sample that comes after what it is evaluated on.

Every fold's `train_indices` starts at `0` (or at the rolling-window
start, when `options.max_train_size` is set) and runs up to that fold's
own gap-adjusted boundary -- an expanding window by default, since later
folds simply see more history. `test_indices` is always exactly
`options.test_size` (explicit or inferred) samples long, and folds are
laid out back-to-back from the end of the sample range: with `folds`
folds of `test_size` each, the first fold's test window starts at
`sample_count - folds * test_size`.

## Parameters

`std::int64_t sample_count`
: The total number of samples to split, over `[0, sample_count)`. Must
  be `>= 2`.

`TemporalSplitOptions options = {}`
: Fold count, test size, max train size, gap, and batch dimension (the
  last unused by this particular overload, since it produces indices
  rather than fields) -- see
  [`TemporalSplitOptions`](TemporalSplitOptions.md).

## Returns

[`TemporalIndexSplit`](TemporalIndexSplit.md) -- the per-fold
`train_indices`/`test_indices`.

## Errors

- `sample_count < 2`, or `options.folds < 2` -- throws.
- `options.gap < 0`, or `options.test_size`/`max_train_size` provided
  but `<= 0` -- throws.
- `options.folds + 1` exceeds `sample_count`, or the folds/test size/gap
  combination does not leave room for at least one training sample --
  throws.

## Practical Examples

Three examples: an explicit test size, the inferred default test size,
and a rolling window with a gap.

### Example 1

An explicit `test_size` -- each fold's test window is exactly `2`
samples, and the training window expands with every fold.

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

### Example 2

Leaving `test_size` unset infers `sample_count / (folds + 1)` -- here
`12 / 4 = 3`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::TemporalSplitOptions options{};
  options.folds = 3;

  loader::TemporalIndexSplit split = loader::temporal_split_indices(12, options);

  std::cout << "fold 0 test_indices = [";
  for (auto v : split.folds[0].test_indices) std::cout << v << " ";
  std::cout << "]\n";

  return 0;
}
```

Output

```text
fold 0 test_indices = [3 4 5 ]
```

### Example 3

`gap` leaves a buffer between training and test, and `max_train_size`
caps the training window to a fixed-size rolling window instead of
letting it expand indefinitely.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::TemporalSplitOptions options{};
  options.folds = 3;
  options.test_size = 2;
  options.gap = 1;
  options.max_train_size = 3;

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
fold 0 train_indices = [0 1 2 ] test_indices = [4 5 ]
fold 1 train_indices = [2 3 4 ] test_indices = [6 7 ]
fold 2 train_indices = [4 5 6 ] test_indices = [8 9 ]
```

Every fold's training window is exactly `3` samples (capped by
`max_train_size`) and ends one sample before its test window starts
(the `gap = 1` buffer) -- fold 0's `train_indices` ends at `2`, one
before `test_indices`' first index `4`.

## See Also

- [`TemporalIndexSplit`](TemporalIndexSplit.md) -- the result returned.
- [`TemporalSplitOptions`](TemporalSplitOptions.md) -- fold count, test
  size, max train size, gap, and batch dimension.
- [`temporal_split_fields`](temporal_split_fields.md) -- this applied
  directly to field `Series`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
