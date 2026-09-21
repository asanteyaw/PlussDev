# `pluss::ai::classic::SplitPlan`

An ordered list of [`IndexSplit`](IndexSplit.md) folds — the result of any
cross-validation-style split in this family:
[`crossval_plan`](crossval_plan.md), [`stratified_crossval_plan`](crossval_plan.md),
[`group_crossval_plan`](crossval_plan.md), and
[`ts_crossval_plan`](ts_crossval_plan.md) all return one. Loop over `folds`
to run the same fit/evaluate cycle once per fold.

## Signature

```cpp
std::vector<classic::IndexSplit> classic::SplitPlan::folds;
```

## Construction

Produced only by the cross-validation-plan functions listed above. A plain
value type — copy or store it freely.

## State

`folds` — `std::vector<IndexSplit>`
: One entry per fold, in the order the producing function generated them.
  For [`crossval_plan`](crossval_plan.md) /
  [`stratified_crossval_plan`](crossval_plan.md), that is fold index order
  (`0` through `folds - 1`) — `stratified_crossval_plan` with
  `repeats > 1` concatenates each repeat's folds back to back, so
  `folds.size() == options.folds * options.repeats`. For
  [`ts_crossval_plan`](ts_crossval_plan.md), fold order is chronological —
  each fold's test window starts later than the previous fold's.

## Methods

None — `SplitPlan` is a plain data holder; iterate `folds` directly.

## Practical Example

A 3-fold plan, iterated to print each fold's train/test sizes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  const cls::SplitPlan plan = cls::crossval_plan(10, options).value();

  std::cout << "fold count = " << plan.folds.size() << "\n";
  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    const cls::IndexSplit& fold = plan.folds[i];
    std::cout << "fold " << i << ": train=" << fold.train_indices.numel()
              << " test=" << fold.test_indices.numel() << "\n";
  }

  return 0;
}
```

Output

```text
fold count = 3
fold 0: train=6 test=4
fold 1: train=7 test=3
fold 2: train=7 test=3
```

## Changelog

- **0.1** — `folds`.

## See Also

- [`IndexSplit`](IndexSplit.md) — one fold's train/test indices.
- [`crossval_plan`](crossval_plan.md) / [`ts_crossval_plan`](ts_crossval_plan.md) —
  every function that produces one.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
