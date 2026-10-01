# `pluss::ai::classic::holdout_split` / `strat_holdout_split`

Split a sample count (or a label column) into one train/test
[`IndexSplit`](IndexSplit.md). `holdout_split` partitions rows with no
regard for class balance; `strat_holdout_split` keeps each class's
train/test proportion close to its overall proportion — reach for the
stratified form whenever the target is categorical and its classes are
imbalanced enough that a plain random split could leave a rare class
under- or unrepresented in one side.

## Signature

```cpp
classic::StatusOr<classic::IndexSplit> classic::holdout_split(
    std::int64_t sample_count, const classic::SplitOptions& options = {});

classic::StatusOr<classic::IndexSplit> classic::strat_holdout_split(
    const pluss::Series& labels, const classic::SplitOptions& options = {});
```

## What It Does

Divides row indices into one train partition and one test partition, sized
by `options.holdout_fraction`, and returns them as an
[`IndexSplit`](IndexSplit.md) -- a single train/test split, not the
multi-fold form [`crossval_plan`](crossval_plan.md) produces.
`holdout_split` places rows without regard to class; `strat_holdout_split`
keeps each class's train/test proportion close to its overall proportion
in the full label column.

## Parameters

`sample_count` (`holdout_split` only)
: The total number of rows to partition. Must be `>= 2`.

`labels` (`strat_holdout_split` only)
: A rank-any `Series` of class labels, one per row (reshaped internally to
  rank-1) — its length is the sample count. Must be defined, contain at
  least 2 observations, and contain no `NaN`.

`options` (`SplitOptions`, default `{}`)
: Only three fields matter here: `holdout_fraction` (the target test
  fraction — see `## Returns` for exactly how it becomes a count),
  `shuffle` (whether rows are randomized before partitioning, and — for
  `strat_holdout_split` — whether each class's own rows and the final
  train/test index order are shuffled too), and `random` (the seed behind
  that shuffle). Every other field is ignored by these two functions.

## Returns

An `IndexSplit` whose `test_indices` count is
`clamp(ceil(sample_count * holdout_fraction), 1, sample_count - 1)` — at
least one test row, always at least one train row, rounded up from the
raw fraction. `strat_holdout_split` allocates that same target train/test
split *within* each class first (proportionally, via a largest-remainder
allocation that keeps the totals exact), then concatenates every class's
share; a train/test row's absolute position carries no meaning either way
— only set membership does.

## Errors

- `sample_count < 2` — `Status::invalid_argument`.
- `holdout_fraction` not finite or outside `(0, 1)` —
  `Status::invalid_argument`.
- `strat_holdout_split`: `labels` undefined, fewer than 2 observations, or
  containing `NaN` — `Status::invalid_argument`.
- `strat_holdout_split`: any class has fewer than 2 observations, or the
  computed train/test size is smaller than the number of distinct classes
  — `Status::invalid_argument` (there is no way to give every class at
  least one representative on both sides otherwise).

## Practical Examples

The default holdout fraction's exact train/test counts, the deterministic
`random` seed reproducing an identical split, and stratification keeping a
rare class represented on both sides where a plain split easily would not.

### Example 1

Ten rows, default `holdout_fraction = 0.2`: two test rows, eight train.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::IndexSplit split = cls::holdout_split(10).value();
  std::cout << "train count = " << split.train_indices.numel() << "\n";
  std::cout << "test count  = " << split.test_indices.numel() << "\n";
  std::cout << "test_indices = " << split.test_indices << "\n";

  return 0;
}
```

Output

```text
train count = 8
test count  = 2
test_indices = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 8]
```

### Example 2

The same `RandomState` reproduces the identical split; a different seed
produces a different one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions same_seed{};
  same_seed.random.seed = 42;
  const cls::IndexSplit first = cls::holdout_split(10, same_seed).value();
  const cls::IndexSplit second = cls::holdout_split(10, same_seed).value();
  std::cout << "first  = " << first.test_indices << "\n";
  std::cout << "second = " << second.test_indices << "\n";

  cls::SplitOptions other_seed{};
  other_seed.random.seed = 7;
  const cls::IndexSplit third = cls::holdout_split(10, other_seed).value();
  std::cout << "third  = " << third.test_indices << "\n";

  return 0;
}
```

Output

```text
first  = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[8, 1]
second = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[8, 1]
third  = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[8, 5]
```

### Example 3

A rare class (2 of 12 rows) still lands on both sides of a stratified
split, sized so its 2 rows split `1`/`1` rather than risking both landing
in the same partition.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 10 rows of class 0, 2 rows of class 1.
  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {12}, f64);

  cls::SplitOptions options{};
  options.holdout_fraction = 0.5;
  const cls::IndexSplit split = cls::strat_holdout_split(labels, options).value();
  std::cout << "train count = " << split.train_indices.numel() << "\n";
  std::cout << "test count  = " << split.test_indices.numel() << "\n";
  std::cout << "test_indices = " << split.test_indices << "\n";

  return 0;
}
```

Output

```text
train count = 6
test count  = 6
test_indices = pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[10, 3, 0, 5, 7, 6]
```

## Changelog

- **0.1** — `holdout_split`, `strat_holdout_split`.

## See Also

- [`IndexSplit`](IndexSplit.md) — the returned type.
- [`SplitOptions`](SplitOptions.md) — the options struct.
- [`crossval_plan`](crossval_plan.md) — the multi-fold counterpart.
- [`slice_batch`](slice_batch.md) — applying the returned indices to real
  data.
