# `pluss::ai::classic::BalancePlan`

The full index / synthesis / cleaning recipe
[`plan_balance`](plan_balance.md) computes and
[`balance_rows`](balance_rows.md) executes — every field needed to
reproduce the rebalanced batch without re-running any of the random
sampling, plus before/after class-distribution snapshots for inspection.

## Signature

```cpp
pluss::Series classic::BalancePlan::indices;                    // Int64, the retained/duplicated row indices to keep
pluss::Series classic::BalancePlan::target_counts;               // Float64, [class_count], the resolved target count for each class
pluss::Series classic::BalancePlan::synthetic_base_indices;      // Int64, one per synthetic row (undefined if none)
pluss::Series classic::BalancePlan::synthetic_neighbor_indices;  // Int64, one per synthetic row (undefined if none)
pluss::Series classic::BalancePlan::synthetic_steps;             // Float64, one per synthetic row, in [0, 1) (undefined if none)
pluss::Series classic::BalancePlan::synthetic_labels;            // Float64, one per synthetic row (undefined if none)
pluss::Series classic::BalancePlan::removed_indices;             // Int64, rows cleaning removed (undefined unless clean != None)
classic::ClassMix classic::BalancePlan::before;                  // the ORIGINAL, pre-cleaning distribution
classic::ClassMix classic::BalancePlan::after;                   // the distribution once this plan is fully applied
bool classic::BalancePlan::uses_replacement;
bool classic::BalancePlan::uses_synthesis;
bool classic::BalancePlan::uses_cleaning;
```

## Construction

Produced only by [`plan_balance`](plan_balance.md) (and, internally, by
[`balance_rows`](balance_rows.md), which calls `plan_balance` itself). A
plain value type — copy or store it freely.

## State

`indices` — `pluss::Series`
: The row indices (into the **original** label/feature data) that make
  up the retained/duplicated portion of the rebalanced result — every
  row `balance_rows` keeps via [`slice_labeled_batch`](../split/slice_batch.md),
  in this exact order. A downsampled class's own rows are a strict
  subset; an oversampled class under `growth = ReuseRows` repeats some
  indices.

`target_counts` — `pluss::Series`
: The resolved per-class target count `target.kind` produced — see
  [`plan_balance`'s `## Returns`](plan_balance.md#returns) for exactly
  which distribution (original vs. post-cleaning) this is computed
  against.

`synthetic_base_indices` / `synthetic_neighbor_indices` / `synthetic_steps` / `synthetic_labels`
: Set only when `uses_synthesis` is `true` (`growth = BlendNeighbors`
  actually produced at least one synthetic row) — all four
  `Series::undefined()` otherwise. Together they describe every
  synthetic row without yet computing it:
  `base + step * (neighbor - base)`, where `base`/`neighbor` index into
  the **original** feature rows and `step` is the interpolation
  fraction. [`balance_rows`](balance_rows.md) performs that arithmetic;
  `plan_balance` alone never does.

`removed_indices` — `pluss::Series`
: Set only when `uses_cleaning` is `true` (`target.clean != None`) —
  `Series::undefined()` otherwise, even if cleaning ran and happened to
  remove nothing. The original row indices cleaning discarded, before
  any resampling or synthesis ran.

`before` — `ClassMix`
: The class distribution of the **original** labels, before cleaning,
  resampling, or synthesis.

`after` — `ClassMix`
: The class distribution once every retained, duplicated, and synthetic
  row is accounted for — what the batch would look like after
  [`balance_rows`](balance_rows.md) actually applies this plan.

`uses_replacement` / `uses_synthesis` / `uses_cleaning` — `bool`
: Quick flags summarizing which mechanisms this particular plan actually
  used — `uses_cleaning` is `true` whenever `target.clean != None`,
  regardless of whether cleaning ended up removing any rows.

## Methods

None — `BalancePlan` is a plain data holder, read directly.

## Practical Example

An `Equalize` target with both duplication and cleaning, every field
read back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 20.0, 20.1, 40.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  target.clean = cls::BalanceCleanKind::BoundaryPairs;
  target.protect_small_classes = false;
  cls::RandomState random{};
  random.seed = 4;
  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();

  std::cout << "before.counts = " << plan.before.counts << "\n";
  std::cout << "after.counts  = " << plan.after.counts << "\n";
  std::cout << "uses_cleaning = " << std::boolalpha << plan.uses_cleaning << "\n";
  std::cout << "uses_replacement = " << plan.uses_replacement << "\n";
  std::cout << "uses_synthesis = " << plan.uses_synthesis << "\n";
  std::cout << "removed_indices.numel() = " << plan.removed_indices.numel() << "\n";

  return 0;
}
```

Output

```text
before.counts = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 2]
after.counts  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 5]
uses_cleaning = true
uses_replacement = true
uses_synthesis = false
removed_indices.numel() = 2
```

## Changelog

- **0.1** — `indices`, `target_counts`, `synthetic_base_indices`,
  `synthetic_neighbor_indices`, `synthetic_steps`, `synthetic_labels`,
  `removed_indices`, `before`, `after`, `uses_replacement`,
  `uses_synthesis`, `uses_cleaning`.

## See Also

- [`plan_balance`](plan_balance.md) — the producing function.
- [`balance_rows`](balance_rows.md) — executes this plan into a new
  `LabeledBatch`.
- [`ClassMix`](ClassMix.md) — `before`/`after`'s own type.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
