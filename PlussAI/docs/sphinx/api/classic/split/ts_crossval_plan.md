# `pluss::ai::classic::ts_crossval_plan`

Split a sample count into a [`SplitPlan`](SplitPlan.md) of chronologically
ordered folds — each fold's test window comes strictly after its train
window, and later folds' test windows come later still. This is the
correct cross-validation shape for time-ordered data, where
[`crossval_plan`](crossval_plan.md)'s ordinary shuffled or unshuffled folds
would let a model train on the future and test on the past — a fold here
never does that.

## Signature

```cpp
classic::StatusOr<classic::SplitPlan> classic::ts_crossval_plan(
    std::int64_t sample_count, const classic::SplitOptions& options = {});
```

## What It Does

Lays out `folds` train/test windows across `sample_count` rows assumed to
already be in chronological order, advancing each fold's windows forward
in time -- never assigning a later row to train and an earlier row to
test, the way an ordinary shuffled or unshuffled `crossval_plan` fold
could for time-ordered data. `options.time_gap`/`time_max_train_size`
control the buffer between train and test and whether the train window
expands or stays a fixed recent size, respectively.

## Parameters

`sample_count`
: The total number of rows, assumed already in chronological order (row
  `0` earliest). Must be `>= 2`.

`options` (`SplitOptions`, default `{}`)
: `folds` — the number of folds; must be `>= 2` and small enough that
  `folds` test windows plus `time_gap` still fit inside `sample_count`.
  `time_test_size` — each fold's test window size; `<= 0` (the default)
  infers `sample_count / (folds + 1)`. `time_gap` — samples skipped between
  a fold's train window and its test window; `0` by default.
  `time_max_train_size` — caps each fold's train window to its most recent
  samples; `<= 0` (the default) leaves it unbounded (expanding). `shuffle`
  **must** be `false` (the default) — see `## Errors`. Every other field is
  ignored.

## Returns

A `SplitPlan` with `folds` entries, ordered chronologically: fold `0`'s
test window is the earliest, fold `folds - 1`'s the latest (closest to the
end of the data). Every fold's train window is `[0, test_start - time_gap)`
by default (expanding — it grows with each later fold) or
`[test_start - time_gap - time_max_train_size, test_start - time_gap)` when
`time_max_train_size` is set (a fixed-size rolling window instead). Test
windows are contiguous, non-overlapping, and all the same size
(`time_test_size`, or the inferred default).

## Errors

- `sample_count < 2` — `Status::invalid_argument`.
- `options.shuffle == true` — `Status::invalid_argument`,
  `"ts_crossval_plan requires SplitOptions.shuffle=false."` — shuffling
  would destroy the chronological ordering the whole function exists to
  preserve.
- `folds < 2`, a non-positive inferred or explicit `time_test_size`, or a
  combination of `sample_count`/`folds`/`time_test_size`/`time_gap` that
  cannot fit the requested number of non-overlapping test windows —
  `Status::invalid_argument`.

## Practical Examples

The default expanding-window behavior, `time_gap` opening a buffer between
train and test, and `time_max_train_size` turning the window into a fixed-
size rolling one instead of expanding.

### Example 1

Twelve rows, 3 folds: each fold's train window grows, each test window is
the next chronological block.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  options.shuffle = false;  // required by ts_crossval_plan
  const cls::SplitPlan plan = cls::ts_crossval_plan(12, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << ": train=" << plan.folds[i].train_indices
              << " test=" << plan.folds[i].test_indices << "\n";
  }

  return 0;
}
```

Output

```text
fold 0: train=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2] test=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 4, 5]
fold 1: train=pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[0, 1, 2, 3, 4, 5] test=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[6, 7, 8]
fold 2: train=pluss::Series(shape=[9], strides=[1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[0, 1, 2, 3, 4, 5, 6, 7, 8] test=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[9, 10, 11]
```

### Example 2

`time_gap = 2` opens a 2-sample buffer between each fold's train window and
its test window — the most recent 2 samples before a test window are
excluded from training.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 2;
  options.time_test_size = 3;
  options.time_gap = 2;
  options.shuffle = false;  // required by ts_crossval_plan
  const cls::SplitPlan plan = cls::ts_crossval_plan(12, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << ": train=" << plan.folds[i].train_indices
              << " test=" << plan.folds[i].test_indices << "\n";
  }

  return 0;
}
```

Output

```text
fold 0: train=pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 3] test=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[6, 7, 8]
fold 1: train=pluss::Series(shape=[7], strides=[1], dtype=int64, platform=cpu, numel=7, contiguous=true)
data=[0, 1, 2, 3, 4, 5, 6] test=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[9, 10, 11]
```

### Example 3

`time_max_train_size` caps every fold's train window to its most recent `N`
samples — a fixed-size rolling window instead of an ever-growing one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  options.time_max_train_size = 4;
  options.shuffle = false;  // required by ts_crossval_plan
  const cls::SplitPlan plan = cls::ts_crossval_plan(12, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << ": train size=" << plan.folds[i].train_indices.numel()
              << " test size=" << plan.folds[i].test_indices.numel() << "\n";
  }

  return 0;
}
```

Output

```text
fold 0: train size=3 test size=3
fold 1: train size=4 test size=3
fold 2: train size=4 test size=3
```

### Example 4

`shuffle = true` is rejected outright.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.shuffle = true;
  const cls::StatusOr<cls::SplitPlan> plan = cls::ts_crossval_plan(10, options);
  std::cout << "ok = " << std::boolalpha << plan.ok() << "\n";
  std::cout << "message = \"" << plan.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "ts_crossval_plan requires SplitOptions.shuffle=false."
```

## Changelog

- **0.1** — `ts_crossval_plan`.

## See Also

- [`SplitPlan`](SplitPlan.md) / [`IndexSplit`](IndexSplit.md) — the
  returned types.
- [`SplitOptions`](SplitOptions.md) — the options struct.
- [`crossval_plan`](crossval_plan.md) — the shuffle-eligible counterpart,
  for data with no chronological structure.
- [`slice_batch`](slice_batch.md) — applying a fold's indices to real data.
