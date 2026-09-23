# `pluss::ai::classic::variance_prune`

Drops near-constant feature columns — the unsupervised member of this
family: no labels involved, purely a property of `batch.x` itself. Every
kept column's index is preserved in **ascending original order**, unlike
[`pick_features`](pick_features.md#returns), whose reduced batch is
ordered by score instead.

## Signature

```cpp
classic::StatusOr<classic::VariancePruneResult> classic::variance_prune(
    const classic::FeatureBatch& batch,
    double variance_floor = 1e-12);
```

## Parameters

`batch`
: Must pass [`validate_features`](../data/FeatureBatch.md) and have
  rank-2 `x`. Either [`ObservationLayout`](../data/FeatureBatch.md#state)
  is supported.

`variance_floor` (`double`, default `1e-12`)
: The minimum **population** variance (`sum((x - mean)^2) / n`, divided by
  the sample count, not `n - 1`) a column must exceed to be kept — a
  column whose variance equals `variance_floor` **exactly** is dropped,
  since the comparison is strict `>`, not `>=`. See
  [Example 3](#example-3). Must be finite and `>= 0`.

## Returns

A `StatusOr<VariancePruneResult>`:

```cpp
struct classic::VariancePruneResult {
  classic::FeatureBatch reduced;
  pluss::Series selected_feature_indices;  // Int64, ascending original order
  pluss::Series feature_variances;         // Float64, one per ORIGINAL column
};
```

`reduced` — the [`take_features`](take_features.md)-selected batch
(so, like that function, always `Float64` regardless of `batch.x`'s own
dtype); `selected_feature_indices` — which original columns survived, in
ascending order (never reordered by variance); `feature_variances` — every
**original** column's population variance, `feature_count()`-long
regardless of how many survived, so a dropped column's own variance is
still readable. **`variance_prune` never returns an empty result:** if
every column's variance is at or below the floor, the single
highest-variance column is kept anyway, so `reduced` always has at least
one feature. See [Example 4](#example-4).

## Errors

- `variance_floor` not finite, or `< 0` — `Status::invalid_argument`.
- `batch` fails [`validate_features`](../data/FeatureBatch.md), or
  `batch.x` is not rank-2 — whatever `validate_features` returns, or
  `Status::shape_mismatch`.

## Practical Examples

The default floor dropping a genuinely constant column, a custom floor
filtering low-variance noise, the exact-floor strict-inequality edge
case, the never-returns-empty fallback, and an error roundup.

### Example 1

Three columns: one varies a lot, one varies a little, one is exactly
constant. The default `variance_floor = 1e-12` drops only the constant
one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // column 0: varies a lot; column 1: varies a little; column 2: constant.
  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 5.0,
       10.0, 1.01, 5.0,
       20.0, 0.99, 5.0,
       30.0, 1.02, 5.0},
      {12}, f64).reshape({4, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult result = cls::variance_prune(batch).value();
  std::cout << "feature_variances = " << result.feature_variances << "\n";
  std::cout << "selected_feature_indices = " << result.selected_feature_indices << "\n";
  std::cout << "reduced.feature_count() = " << result.reduced.feature_count() << "\n";

  return 0;
}
```

Output

```text
feature_variances = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[125, 0.000125, 0]
selected_feature_indices = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
reduced.feature_count() = 2
```

### Example 2

A custom `variance_floor` also filters out the "varies a little" column.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 5.0,
       10.0, 1.01, 5.0,
       20.0, 0.99, 5.0,
       30.0, 1.02, 5.0},
      {12}, f64).reshape({4, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult result = cls::variance_prune(batch, 0.01).value();
  std::cout << "selected_feature_indices (only column 0 survives) = " << result.selected_feature_indices << "\n";

  return 0;
}
```

Output

```text
selected_feature_indices (only column 0 survives) = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[0]
```

### Example 3

A column whose variance equals `variance_floor` **exactly** is dropped —
the comparison is strict `>`, not `>=`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Column 0's population variance ({-1, 1}) is exactly 1.0: mean=0,
  // sum((x-0)^2)/2 = 1.0. Column 1 always has a clearly higher variance,
  // so it survives either floor and the never-empty fallback never has a
  // chance to mask column 0's own drop/keep outcome.
  const cls::Series x = pluss::make_series(
      {-1.0, -10.0, 1.0, 10.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult at_floor = cls::variance_prune(batch, 1.0).value();
  const cls::VariancePruneResult below_floor = cls::variance_prune(batch, 0.999999).value();
  std::cout << "floor == column 0's variance exactly -> selected_feature_indices = " << at_floor.selected_feature_indices << "\n";
  std::cout << "floor slightly below                 -> selected_feature_indices = " << below_floor.selected_feature_indices << "\n";

  return 0;
}
```

Output

```text
floor == column 0's variance exactly -> selected_feature_indices = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[1]
floor slightly below                 -> selected_feature_indices = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 4

Every column at or below the floor — `variance_prune` still keeps the
single highest-variance one rather than returning an empty batch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Both columns are constant; column 1 (value 5.0) has zero variance,
  // column 0 (value 1.0) also has zero variance -- but the fallback still
  // keeps exactly one.
  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 1.0, 5.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult result = cls::variance_prune(batch, 0.5).value();
  std::cout << "feature_variances = " << result.feature_variances << "\n";
  std::cout << "selected_feature_indices (never empty) = " << result.selected_feature_indices << "\n";
  std::cout << "reduced.feature_count() = " << result.reduced.feature_count() << "\n";

  return 0;
}
```

Output

```text
feature_variances = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
selected_feature_indices (never empty) = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[0]
reduced.feature_count() = 1
```

### Example 5

An error roundup: a negative and a non-finite `variance_floor`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  cls::FeatureBatch batch{};
  batch.x = x;

  std::cout << "negative floor ok  = " << std::boolalpha << cls::variance_prune(batch, -1.0).ok() << "\n";
  std::cout << "non-finite floor ok = " << cls::variance_prune(batch, std::numeric_limits<double>::infinity()).ok() << "\n";

  return 0;
}
```

Output

```text
negative floor ok  = false
non-finite floor ok = false
```

## Changelog

- **0.1** — `variance_prune`, `VariancePruneResult`.

## See Also

- [`take_features`](take_features.md) — builds `reduced` internally.
- [`pick_features`](pick_features.md) — the labeled, score-based
  counterpart, whose reduced batch is ordered by score rather than
  original index.
- [`FeatureBatch`](../data/FeatureBatch.md) — the input and (nested)
  output type.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
