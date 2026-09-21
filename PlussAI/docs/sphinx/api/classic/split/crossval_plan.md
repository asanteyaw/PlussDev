# `pluss::ai::classic::crossval_plan` / `stratified_crossval_plan` / `group_crossval_plan`

Split a sample count, a label column, or a group column into a
[`SplitPlan`](SplitPlan.md) of `folds` train/test partitions — ordinary
k-fold, class-stratified k-fold, and group-aware k-fold respectively.
`crossval_plan` assumes rows are independent and interchangeable;
`stratified_crossval_plan` keeps each fold's class proportions close to the
overall proportions (the multi-fold sibling of
[`strat_holdout_split`](holdout.md)); `group_crossval_plan` keeps every row
sharing a group value on the same side of every fold, so a group never
appears in both a fold's train and test partitions (repeated-measurement or
multi-row-per-entity data, where a fold must not leak the same entity
across train and test).

## Signature

```cpp
classic::StatusOr<classic::SplitPlan> classic::crossval_plan(
    std::int64_t sample_count, const classic::SplitOptions& options = {});

classic::StatusOr<classic::SplitPlan> classic::stratified_crossval_plan(
    const pluss::Series& labels, const classic::SplitOptions& options = {});

classic::StatusOr<classic::SplitPlan> classic::group_crossval_plan(
    const pluss::Series& groups, const classic::SplitOptions& options = {});
classic::StatusOr<classic::SplitPlan> classic::group_crossval_plan(
    /* a range of std::int64_t */, const classic::SplitOptions& options = {});
```

The second `group_crossval_plan` overload accepts anything convertible to a
range of `std::int64_t` — including a plain `{...}` brace-initializer list
of group ids — and wraps it into a `Series` internally before calling the
first overload.

## Parameters

`sample_count` (`crossval_plan` only)
: The total number of rows to partition. Must be `>= 2`.

`labels` (`stratified_crossval_plan` only)
: A rank-any `Series` of class labels, one per row — its length is the
  sample count. Same validity requirements as
  [`strat_holdout_split`](holdout.md)'s `labels`.

`groups` (`group_crossval_plan` only)
: A rank-any `Series`, or a plain range/brace-list, of group ids, one per
  row. Rows sharing the same value are one group. Must have at least 2
  observations and contain no `NaN`.

`options` (`SplitOptions`, default `{}`)
: `folds` (must be `>= 2` and `<=` the sample count — for
  `group_crossval_plan`, also `<=` the number of distinct groups),
  `shuffle`, and `random` matter to all three. `repeats` matters only to
  `stratified_crossval_plan`. Every other field is ignored.

## Returns

A `SplitPlan` with exactly `folds` entries, `folds.size() == options.folds`
(or `options.folds * options.repeats` for `stratified_crossval_plan` — see
below). Every fold's `test_indices` across the whole plan partition every
row exactly once for `crossval_plan` / `stratified_crossval_plan`; for
`group_crossval_plan`, they partition every *group* exactly once (a
group's rows always move together). Fold sizes are as equal as an exact
partition allows — `crossval_plan` distributes any remainder rows across
the first folds one each; `group_crossval_plan` distributes remainder
*groups* (unshuffled: greedily, largest group first, to whichever fold
currently has the fewest samples; shuffled: by a random group-to-fold
assignment instead, so fold sizes can differ more).

`stratified_crossval_plan` with `repeats > 1` runs the whole stratified
k-fold split `repeats` times (each with an independently-derived seed) and
concatenates every repeat's folds into one plan — call this when you want
several independent stratified partitions rather than one, without calling
the function in a loop yourself.

## Errors

- `sample_count < 2` (or, for the label/group forms, fewer than 2 rows) —
  `Status::invalid_argument`.
- `folds < 2`, or `folds >` the sample count — `Status::invalid_argument`.
- `group_crossval_plan`: `folds >` the number of distinct groups —
  `Status::invalid_argument`.
- `stratified_crossval_plan`: `repeats < 1` — `Status::invalid_argument`.
- The label/group `Series` is undefined, too short, or contains `NaN` —
  `Status::invalid_argument`.

## Practical Examples

The remainder-distribution rule on an uneven fold count, class proportions
preserved across folds versus a plain k-fold that can happen to separate
them, a group never split across train and test, and `repeats` producing
independent stratified partitions.

### Example 1

Ten rows into 3 folds: sizes `4, 3, 3` — the one remainder row goes to the
first fold.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  const cls::SplitPlan plan = cls::crossval_plan(10, options).value();
  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << ": test size = " << plan.folds[i].test_indices.numel() << "\n";
  }

  return 0;
}
```

Output

```text
fold 0: test size = 4
fold 1: test size = 3
fold 2: test size = 3
```

### Example 2

Six rows of class 0 and two of class 1, split into 2 folds: the
stratified plan gives each fold one class-1 row; a plain `crossval_plan`
over the same 8 rows has no such guarantee.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::SplitOptions options{};
  options.folds = 2;
  const cls::SplitPlan plan = cls::stratified_crossval_plan(labels, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << plan.folds[i].test_indices << "\n";
  }

  return 0;
}
```

Output

```text
fold 0 test_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 5, 6]
fold 1 test_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 3, 4, 7]
```

### Example 3

Four groups of two rows each (repeated measurements), 2 folds: each fold's
test set is whole groups, never half of one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 2;
  options.shuffle = false;
  const cls::SplitPlan plan = cls::group_crossval_plan({0, 0, 1, 1, 2, 2, 3, 3}, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << plan.folds[i].test_indices << "\n";
  }

  return 0;
}
```

Output

```text
fold 0 test_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 3, 6, 7]
fold 1 test_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 4, 5]
```

### Example 4

`repeats = 3` on the same stratified split produces three independent
partitions concatenated into one plan — `folds.size() == folds * repeats`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);

  cls::SplitOptions options{};
  options.folds = 2;
  options.repeats = 3;
  const cls::SplitPlan plan = cls::stratified_crossval_plan(labels, options).value();
  std::cout << "folds.size() = " << plan.folds.size() << " (expected 2 * 3 = 6)\n";

  return 0;
}
```

Output

```text
folds.size() = 6 (expected 2 * 3 = 6)
```

## Changelog

- **0.1** — `crossval_plan`, `stratified_crossval_plan`,
  `group_crossval_plan` (`Series` and range/brace-list overloads).

## See Also

- [`SplitPlan`](SplitPlan.md) / [`IndexSplit`](IndexSplit.md) — the
  returned types.
- [`SplitOptions`](SplitOptions.md) — the options struct.
- [`holdout`](holdout.md) — the single-partition counterpart.
- [`ts_crossval_plan`](ts_crossval_plan.md) — the chronological-order
  variant, for time-ordered data.
- [`slice_batch`](slice_batch.md) — applying a fold's indices to real data.
