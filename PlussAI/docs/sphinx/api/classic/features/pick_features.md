# `pluss::ai::classic::pick_features`

Ranks features exactly like [`rank_features`](rank_features.md), then
keeps the top-`k` (or top-fraction) and returns the reduced
[`FeatureBatch`](../data/FeatureBatch.md) directly — the labeled,
score-based counterpart of [`variance_prune`](variance_prune.md). **The
reduced batch's column order follows the score ranking, not the original
column order** — the single most important difference from
`variance_prune`'s own `reduced`, which stays in ascending original index
order. See [Example 5](#example-5) for the two compared directly.

## Signature

```cpp
classic::StatusOr<classic::FeaturePickResult> classic::pick_features(
    const classic::LabeledBatch& batch,
    const classic::FeaturePickOptions& options = {});
```

## What It Does

Scores and ranks `batch`'s features exactly the way
[`rank_features`](rank_features.md) does, then reduces `batch` itself down
to just the top-`k` (or top-fraction) columns and hands back the resulting
[`FeatureBatch`](../data/FeatureBatch.md) -- the one call to reach for when
the goal is a smaller, ready-to-use batch rather than the ranking
indices/scores alone.

## Parameters

`batch`
: Identical contract to [`rank_features`'s `batch`](rank_features.md#parameters).

`options` (`FeaturePickOptions`, default `{}`)
: ```cpp
  struct classic::FeaturePickOptions {
    classic::FeatureScoreMode mode = classic::FeatureScoreMode::ClassF;
    std::int64_t top_k = 10;
    double fraction = 0.0;
    bool absolute_score = true;
  };
  ```

  `mode` / `absolute_score` — identical contract to
  [`rank_features`'s `options`](rank_features.md#parameters), including
  `absolute_score` having no observable effect on either score formula.

  `fraction` (default `0.0`, meaning "unset") — when `> 0.0`, **overrides
  `top_k` entirely** (checked first): a value in `(0, 1]` is a literal
  fraction of `feature_count()`; a value `> 1` (up to `100`) is a percent
  — so `fraction = 0.25` and `fraction = 25` both keep 25% of features.
  The kept count is `ceil(fraction_resolved * feature_count())`, at least
  `1`. `fraction = 1.0` keeps 100% (the literal-fraction branch, not the
  percent one, since the check is `<= 1.0`). See
  [Example 2](#example-2) and [Example 3](#example-3).

  `top_k` (default `10`) — used only when `fraction <= 0.0`; the count is
  `min(top_k, feature_count())`, so a `top_k` larger than the actual
  feature count is silently capped, never an error. See
  [Example 1](#example-1).

## Returns

A `StatusOr<FeaturePickResult>`:

```cpp
struct classic::FeaturePickResult {
  classic::FeatureBatch reduced;              // columns in SCORE-DESCENDING order
  pluss::Series selected_feature_indices;     // Int64, same score-descending order
  pluss::Series scores;                       // Float64, one per feature, ORIGINAL column order
  pluss::Series ranked_feature_indices;       // Int64, every feature, score-descending
};
```

`scores`/`ranked_feature_indices` are exactly
[`rank_features`'s own output](rank_features.md#returns) for the same
`batch`/`mode`/`absolute_score` — full-length, not truncated to the kept
count. `selected_feature_indices` is the **first** `count` entries of
`ranked_feature_indices` (`count` from `top_k`/`fraction` above);
`reduced` is [`take_features`](take_features.md) applied with exactly
those indices, in that order — so `reduced`'s first column is always the
single highest-scoring feature, not whichever original column happened to
be first.

## Errors

- `batch` fails [`validate_labeled`](../data/LabeledBatch.md), `batch.y`
  contains a `NaN`, or a length mismatch —
  `Status::invalid_argument` / `Status::shape_mismatch`.
- `mode = ClassF`: fewer than two distinct values in `batch.y` —
  `Status::invalid_argument`.
- `mode = RegressionF`: fewer than 3 samples, or a constant `batch.y` —
  `Status::invalid_argument`.
- `options.fraction` not finite — `Status::invalid_argument`.
- `options.fraction` resolves (after the `<= 1.0` literal-vs-percent
  check) to `<= 0.0` or `> 1.0` — `Status::invalid_argument`.
- `options.fraction <= 0.0` (unset) and `options.top_k <= 0` —
  `Status::invalid_argument`.

## Practical Examples

`top_k` keeping the two strongest of four features, `fraction` in its
literal `(0, 1]` form, `fraction` in its percent `(1, 100]` form,
`fraction` overriding a simultaneously-set `top_k`, the reduced batch's
score-order contrasted directly against `variance_prune`'s original-order,
`RegressionF` mode, and an error roundup for the fraction/top_k logic.

### Example 1

Four features, `top_k = 2` — the two most class-separating features,
whichever original columns they were.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // features 1 and 3 separate the classes; 0 and 2 are noise.
  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 9.0, 0.0,
       1.0, 0.1, 3.0, 0.2,
       9.0, 0.2, 1.0, 0.1,
       2.0, 10.0, 5.0, 10.0,
       8.0, 10.1, 2.0, 10.2,
       3.0, 10.2, 8.0, 10.1},
      {24}, f64).reshape({6, 4});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions options{};
  options.top_k = 2;
  const cls::FeaturePickResult result = cls::pick_features(batch, options).value();
  std::cout << "selected_feature_indices = " << result.selected_feature_indices << "\n";
  std::cout << "reduced.feature_count() = " << result.reduced.feature_count() << "\n";

  return 0;
}
```

Output

```text
selected_feature_indices = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 3]
reduced.feature_count() = 2
```

### Example 2

`fraction = 0.5` keeps 50% of 4 features — the literal-fraction form.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 9.0, 0.0, 1.0, 0.1, 3.0, 0.2, 9.0, 0.2, 1.0, 0.1,
       2.0, 10.0, 5.0, 10.0, 8.0, 10.1, 2.0, 10.2, 3.0, 10.2, 8.0, 10.1},
      {24}, f64).reshape({6, 4});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions options{};
  options.fraction = 0.5;
  const cls::FeaturePickResult result = cls::pick_features(batch, options).value();
  std::cout << "reduced.feature_count() (ceil(0.5*4) = 2) = " << result.reduced.feature_count() << "\n";

  return 0;
}
```

Output

```text
reduced.feature_count() (ceil(0.5*4) = 2) = 2
```

### Example 3

`fraction = 25` (interpreted as `25%`, the percent form) picks the same
count as `fraction = 0.25` would.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 9.0, 0.0, 1.0, 0.1, 3.0, 0.2, 9.0, 0.2, 1.0, 0.1,
       2.0, 10.0, 5.0, 10.0, 8.0, 10.1, 2.0, 10.2, 3.0, 10.2, 8.0, 10.1},
      {24}, f64).reshape({6, 4});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions percent_form{};
  percent_form.fraction = 25.0;
  cls::FeaturePickOptions literal_form{};
  literal_form.fraction = 0.25;

  const cls::FeaturePickResult via_percent = cls::pick_features(batch, percent_form).value();
  const cls::FeaturePickResult via_literal = cls::pick_features(batch, literal_form).value();
  std::cout << "percent form feature_count() = " << via_percent.reduced.feature_count() << "\n";
  std::cout << "literal form feature_count() = " << via_literal.reduced.feature_count() << "\n";

  return 0;
}
```

Output

```text
percent form feature_count() = 1
literal form feature_count() = 1
```

### Example 4

`fraction` set alongside `top_k` — `fraction` wins, `top_k` is ignored
entirely.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 9.0, 0.0, 1.0, 0.1, 3.0, 0.2, 9.0, 0.2, 1.0, 0.1,
       2.0, 10.0, 5.0, 10.0, 8.0, 10.1, 2.0, 10.2, 3.0, 10.2, 8.0, 10.1},
      {24}, f64).reshape({6, 4});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions options{};
  options.top_k = 4;         // would keep all 4 if it were read
  options.fraction = 0.25;   // takes priority: keeps 1
  const cls::FeaturePickResult result = cls::pick_features(batch, options).value();
  std::cout << "feature_count() (fraction wins, top_k ignored) = " << result.reduced.feature_count() << "\n";

  return 0;
}
```

Output

```text
feature_count() (fraction wins, top_k ignored) = 1
```

### Example 5

`pick_features`'s `reduced` is ordered by score descending — genuinely
different from [`variance_prune`'s](variance_prune.md) ascending-original-
index order, shown side by side on the same underlying columns.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // column 0: weakly separating; column 1: strongly separating.
  const cls::Series x = pluss::make_series(
      {4.0, 0.0, 4.5, 0.1, 5.0, 0.2, 6.0, 10.0, 6.5, 10.1, 7.0, 10.2},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions pick_options{};
  pick_options.top_k = 2;
  const cls::FeaturePickResult picked = cls::pick_features(batch, pick_options).value();
  const cls::VariancePruneResult pruned = cls::variance_prune(batch.features).value();

  std::cout << "pick_features selected_feature_indices  (score order:    [1, 0]) = " << picked.selected_feature_indices << "\n";
  std::cout << "variance_prune selected_feature_indices (original order: [0, 1]) = " << pruned.selected_feature_indices << "\n";

  return 0;
}
```

Output

```text
pick_features selected_feature_indices  (score order:    [1, 0]) = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
variance_prune selected_feature_indices (original order: [0, 1]) = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 6

`RegressionF` mode, `top_k = 1` — the single feature most correlated with
a continuous target.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 2.0, 1.0, 3.0, 9.0, 4.0, 2.0, 5.0, 8.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions options{};
  options.mode = cls::FeatureScoreMode::RegressionF;
  options.top_k = 1;
  const cls::FeaturePickResult result = cls::pick_features(batch, options).value();
  std::cout << "selected_feature_indices (column 0, the correlated one) = " << result.selected_feature_indices << "\n";

  return 0;
}
```

Output

```text
selected_feature_indices (column 0, the correlated one) = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[0]
```

### Example 7

An error roundup: `fraction` out of `(0, 100]` range, a non-finite
`fraction`, and `top_k <= 0` with `fraction` left unset.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 10.0, 2.0, 10.1, 8.0, 10.2, 3.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions too_big{};
  too_big.fraction = 150.0;
  std::cout << "fraction=150 ok = " << std::boolalpha << cls::pick_features(batch, too_big).ok() << "\n";

  cls::FeaturePickOptions non_finite{};
  non_finite.fraction = std::numeric_limits<double>::infinity();
  std::cout << "non-finite fraction ok = " << cls::pick_features(batch, non_finite).ok() << "\n";

  cls::FeaturePickOptions zero_top_k{};
  zero_top_k.top_k = 0;
  std::cout << "top_k=0, fraction unset ok = " << cls::pick_features(batch, zero_top_k).ok() << "\n";

  return 0;
}
```

Output

```text
fraction=150 ok = false
non-finite fraction ok = false
top_k=0, fraction unset ok = false
```

## Changelog

- **0.1** — `pick_features`, `FeaturePickOptions`, `FeaturePickResult`.

## See Also

- [`rank_features`](rank_features.md) — the same scoring, without the
  reduction to a `FeatureBatch`.
- [`variance_prune`](variance_prune.md) — the unsupervised counterpart,
  whose `reduced` stays in original column order.
- [`take_features`](take_features.md) — builds `reduced` internally.
- [`f_score_rank`](f_score_rank.md) / [`f_regression_rank`](f_regression_rank.md)
  — narrower `top_k`-only wrappers returning just the index list.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
