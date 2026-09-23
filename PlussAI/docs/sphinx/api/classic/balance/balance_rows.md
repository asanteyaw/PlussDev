# `pluss::ai::classic::balance_rows`

Applies a [`BalanceTarget`](BalanceTarget.md) and returns the rebalanced
[`LabeledBatch`](../data/LabeledBatch.md) directly — runs
[`plan_balance`](plan_balance.md) internally, then materializes the
result: real duplicated/retained rows via
[`slice_labeled_batch`](../split/slice_batch.md), and (when the plan used
`BlendNeighbors`) real interpolated feature rows computed from the
plan's own `synthetic_base_indices`/`synthetic_neighbor_indices`/
`synthetic_steps`. This is the function `TrainOptions::balance` uses to
rebalance a classifier's training rows before fitting.

## Signature

```cpp
classic::StatusOr<classic::LabeledBatch> classic::balance_rows(
    const classic::LabeledBatch& batch,
    const classic::BalanceTarget& target,
    classic::RandomState random = {});
```

## Parameters

`batch` / `target` / `random`
: Identical contract to
  [`plan_balance`'s `LabeledBatch` overload](plan_balance.md#parameters)
  — every field, every error condition, carries over unchanged, since
  this function's first step is exactly that call.

## Returns

A `StatusOr<LabeledBatch>`. When the resolved plan never synthesizes
(`BalancePlan::uses_synthesis == false` — the ordinary case for `growth
= ReuseRows`, or `kind = Keep`), the result is exactly
[`slice_labeled_batch(batch, plan.indices)`](../split/slice_batch.md) —
nothing computed beyond row selection. When synthesis did happen, every
synthetic feature row is the real linear interpolation `base + step *
(neighbor - base)`, appended after the retained/duplicated rows, in the
**same** [`ObservationLayout`](../data/FeatureBatch.md#state) as
`batch.features` (concatenated along the sample dimension either way —
rows or columns). A synthetic row's own label is copied from its base
class (cast back to `batch.y`'s own dtype, even if it started as an
integer type); a synthetic row's own `sample_weights` entry (only when
`batch.sample_weights` is defined) is the **average** of its base and
neighbor rows' own weights.

## Errors

- `batch` fails [`validate_labeled`](../data/LabeledBatch.md) —
  whatever that validation returns.
- Everything [`plan_balance`](plan_balance.md#errors) (the
  `LabeledBatch` overload) can already fail with.
- The resulting batch itself fails
  [`validate_labeled`](../data/LabeledBatch.md) (an internal consistency
  check after assembly) — whatever that validation returns.

## Practical Examples

A basic `Equalize` lifecycle, a synthetic row's value verified by hand
against the plan's own `synthetic_steps`, `sample_weights` averaged for
a synthetic row, the `Columns`-layout path, integer-typed labels
preserved through synthesis, the no-synthesis path proven identical to
a direct slice, and an error roundup.

### Example 1

`Equalize` with `ReuseRows` — the result's own class counts match the
plan's `target_counts`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 100.0, 200.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  cls::RandomState random{};
  random.seed = 13;
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();
  std::cout << "sample_count() = " << balanced.features.sample_count() << "\n";
  std::cout << "class_mix.counts = " << cls::class_mix(balanced.y).value().counts << "\n";

  return 0;
}
```

Output

```text
sample_count() = 12
class_mix.counts = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 6]
```

### Example 2

A synthetic row's real feature value, verified by hand against the
plan's own `synthetic_base_indices`/`synthetic_neighbor_indices`/
`synthetic_steps` for that same call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // both classes have >= 2 rows, so every extra row here is genuinely
  // synthesized -- no class falls back to plain duplication, which keeps
  // plan.indices.numel() equal to the original row count (7), making the
  // very first synthetic row land at output index 7.
  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0, 510.0}, {7}, f64).reshape({7, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {7}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({7.0, 3.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 17;

  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();

  const std::int64_t base_idx = plan.synthetic_base_indices.slice(0, 0, 1).item<std::int64_t>();
  const std::int64_t neighbor_idx = plan.synthetic_neighbor_indices.slice(0, 0, 1).item<std::int64_t>();
  const double step = plan.synthetic_steps.slice(0, 0, 1).item<double>();
  const double base_val = x.slice(0, base_idx, base_idx + 1).item<double>();
  const double neighbor_val = x.slice(0, neighbor_idx, neighbor_idx + 1).item<double>();
  const double expected = base_val + step * (neighbor_val - base_val);

  // plan.indices.numel() (not the original row count!) is where the
  // retained/duplicated rows end and the first synthetic row begins --
  // the two only coincide when nothing fell back to plain duplication.
  const std::int64_t synthesized_row = plan.indices.numel();
  const double actual = balanced.features.x.slice(0, synthesized_row, synthesized_row + 1).item<double>();

  std::cout << "expected (base + step*(neighbor-base)) = " << expected << "\n";
  std::cout << "actual (from balance_rows output)      = " << actual << "\n";

  return 0;
}
```

Output

```text
expected (base + step*(neighbor-base)) = 8.09607
actual (from balance_rows output)      = 8.09607
```

### Example 3

A synthetic row's `sample_weights` entry is the average of its base and
neighbor rows' own weights.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0, 510.0}, {7}, f64).reshape({7, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {7}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  batch.sample_weights = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0}, {7}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({7.0, 3.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 17;

  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();

  const std::int64_t base_idx = plan.synthetic_base_indices.slice(0, 0, 1).item<std::int64_t>();
  const std::int64_t neighbor_idx = plan.synthetic_neighbor_indices.slice(0, 0, 1).item<std::int64_t>();
  const double w_base = batch.sample_weights.slice(0, base_idx, base_idx + 1).item<double>();
  const double w_neighbor = batch.sample_weights.slice(0, neighbor_idx, neighbor_idx + 1).item<double>();
  const std::int64_t synthesized_row = plan.indices.numel();
  const double actual_w = balanced.sample_weights.slice(0, synthesized_row, synthesized_row + 1).item<double>();

  std::cout << "expected ((w_base + w_neighbor) / 2) = " << (w_base + w_neighbor) / 2.0 << "\n";
  std::cout << "actual                                = " << actual_w << "\n";

  return 0;
}
```

Output

```text
expected ((w_base + w_neighbor) / 2) = 1.5
actual                                = 1.5
```

### Example 4

`ObservationLayout::Columns` — synthetic rows are concatenated along the
correct dimension, and the result keeps `batch.features`'s own layout.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 6 samples, 1 feature, Rows form, then transposed to Columns form.
  const pluss::Series row_x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = row_x.transpose(0, 1).contiguous();
  batch.features.layout = cls::ObservationLayout::Columns;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({6.0, 2.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 23;
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();

  std::cout << "layout is Columns = " << std::boolalpha << (balanced.features.layout == cls::ObservationLayout::Columns) << "\n";
  std::cout << "sample_count() = " << balanced.features.sample_count() << "\n";

  return 0;
}
```

Output

```text
layout is Columns = true
sample_count() = 8
```

### Example 5

Integer-typed labels are preserved through synthesis — a synthetic
row's label is cast back to `batch.y`'s own dtype, not left as the
internal `Float64` working type.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Config i64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1}, {6}, i64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({6.0, 2.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target).value();

  std::cout << "batch.y dtype is Int64     = " << std::boolalpha << (batch.y.scalar_type() == pluss::ScalarType::Int64) << "\n";
  std::cout << "balanced.y dtype is Int64  = " << (balanced.y.scalar_type() == pluss::ScalarType::Int64) << "\n";

  return 0;
}
```

Output

```text
batch.y dtype is Int64     = true
balanced.y dtype is Int64  = true
```

### Example 6

When the plan never synthesizes, the result is exactly a direct
[`slice_labeled_batch`](../split/slice_batch.md) call on the plan's own
`indices` — proven identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 100.0, 200.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  cls::RandomState random{};
  random.seed = 29;

  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  const cls::LabeledBatch via_balance_rows = cls::balance_rows(batch, target, random).value();
  const cls::LabeledBatch via_direct_slice = cls::slice_labeled_batch(batch, plan.indices).value();

  const double diff = (via_balance_rows.features.x - via_direct_slice.features.x)
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "uses_synthesis = " << std::boolalpha << plan.uses_synthesis << "\n";
  std::cout << "max abs diff (no synthesis -> identical to a direct slice) = " << diff << "\n";

  return 0;
}
```

Output

```text
uses_synthesis = false
max abs diff (no synthesis -> identical to a direct slice) = 0
```

### Example 7

An error roundup: an invalid `batch`, and a `plan_balance` failure
(`Ratio` on three classes) propagating through unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch undefined_batch{};
  std::cout << "undefined batch ok = " << std::boolalpha << cls::balance_rows(undefined_batch, cls::BalanceTarget{}).ok() << "\n";

  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64).reshape({3, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64);
  cls::LabeledBatch three_classes{};
  three_classes.features.x = x;
  three_classes.y = y;
  cls::BalanceTarget ratio_target{};
  ratio_target.kind = cls::BalanceTargetKind::Ratio;
  const cls::StatusOr<cls::LabeledBatch> rejected = cls::balance_rows(three_classes, ratio_target);
  std::cout << "Ratio with 3 classes ok = " << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
undefined batch ok = false
Ratio with 3 classes ok = false
message = "plan_balance BalanceTargetKind::Ratio currently requires exactly two classes."
```

## Changelog

- **0.1** — `balance_rows`.

## See Also

- [`plan_balance`](plan_balance.md) — computes the plan this function
  executes.
- [`BalanceTarget`](BalanceTarget.md) — every field this function reads.
- [`TrainOptions`](../core/TrainOptions.md#balance) — `.balance`, where
  this exact logic reaches every classifier automatically before it
  fits.
- [`slice_labeled_batch`](../split/slice_batch.md) — the no-synthesis
  code path this function reduces to.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
