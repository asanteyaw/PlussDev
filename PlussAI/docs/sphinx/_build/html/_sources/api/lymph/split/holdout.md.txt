# `lymph::validate::holdout`

## Summary

`validate::holdout(...)` creates a holdout-style validation plan.

Use it when you want one explicit train/test carve-out rather than many folds.

## What It Means

`validate::holdout(...)` is the fastest validation builder in this family.

It is the right surface when you want:

- quick iteration
- one inspectable train/test split
- a final carve-out after broader selection work has already happened

The tradeoff is equally important: you get speed and simplicity, but not fold-level
variance information.

## High-Value Toggle

The defining field is `holdout_fraction`.

Effects:

- larger fractions give you a larger test set and less training data
- smaller fractions preserve more training data but make the test estimate noisier

There is no universally correct value. The right value depends on dataset size,
variance, and how expensive it would be to miss a bad model.

## Signature

```cpp
lymph::HoldoutPlan lymph::validate::holdout(...);
lymph::HoldoutPlan& lymph::HoldoutPlan::test_size(double value);
lymph::HoldoutPlan& lymph::HoldoutPlan::shuffle(bool value = true);
lymph::HoldoutPlan& lymph::HoldoutPlan::seed(std::uint64_t value);
lymph::prep::HoldoutSplit lymph::HoldoutPlan::split(
    const pluss::table::DataFrame& frame) const;
```

## Practical Examples

These examples show the most important thing to learn first: holdout is a split
artifact builder, and changing `test_size(...)` materially changes how much data is
left for training.

### Example 1

This example creates a 25% holdout split on a tiny frame and prints the resulting
train/test row counts.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}))},
      {"late", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
      {"target", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
  });

  const lymph::HoldoutPlan plan =
      lymph::validate::holdout().test_size(0.25).shuffle(true).seed(20260825ULL);
  const lymph::prep::HoldoutSplit split = plan.split(frame);

  std::cout << "== holdout_split ==\n";
  std::cout << "train_rows=" << split.train_set.ncases()
            << " test_rows=" << split.test_set.ncases() << "\n";
}
```

Output

```text
== holdout_split ==
train_rows=6 test_rows=2
```

This is the right mental model for `holdout()`: one explicit carve-out you can
inspect, train on, and evaluate against.

### Example 2

This follow-up example reuses the same `frame` but changes the holdout fraction to
50% and disables shuffling so you can see how strongly the train/test balance can
move from one policy change.

```cpp
const lymph::HoldoutPlan half_plan =
    lymph::validate::holdout().test_size(0.5).shuffle(false).seed(20260825ULL);
const lymph::prep::HoldoutSplit half_split = half_plan.split(frame);

std::cout << "== holdout_split_half ==\n";
std::cout << "train_rows=" << half_split.train_set.ncases()
          << " test_rows=" << half_split.test_set.ncases() << "\n";
```

Output

```text
== holdout_split_half ==
train_rows=4 test_rows=4
```

That single change doubled the test allocation. On real projects that choice
changes both:

- how much evidence you get from the test slice
- how much signal the model is allowed to learn from during training

## When Holdout Is Not Enough

Do not confuse fast with reliable.

One holdout split is often appropriate for:

- rough iteration
- early filtering of weak ideas
- very large datasets where full cross-validation would be wasteful

It is often insufficient when:

- data is small
- class imbalance is severe
- time structure matters
- you need a variance estimate across folds

In those cases, use a richer plan such as [cross](cross.md) or [time](time.md).

## Related Pages

- [ValidationOptions](../metrics/ValidationOptions.md)
- [Plan](Plan.md)
