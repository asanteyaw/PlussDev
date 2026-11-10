# `pluss::ai::classic::rank_features`

Scores every feature column by a univariate F-statistic against a target,
then orders them by score — the base ranking primitive
[`pick_features`](pick_features.md) and [`f_score_rank`](f_score_rank.md)/
[`f_regression_rank`](f_regression_rank.md) all build on. Unlike those
three, `rank_features` returns **every** feature's score, ranked or not —
reach for it directly when you want the full ranking rather than only a
reduced batch or a top-k index list.

## Signature

```cpp
classic::StatusOr<classic::FeatureRankReport> classic::rank_features(
    const classic::LabeledBatch& batch,
    const classic::FeatureRankOptions& options = {});
```

## What It Does

Computes a univariate F-statistic for every feature column in `batch`
against `batch.y` -- ANOVA F for a classification target, correlation-based
F for a regression target -- and returns the full result: every feature's
score, in ranked order, with nothing dropped. This is the base primitive
[`pick_features`](pick_features.md),
[`f_score_rank`](f_score_rank.md), and
[`f_regression_rank`](f_regression_rank.md) all narrow down to a smaller
result; call `rank_features` directly when the complete picture, not just
a reduced batch or a top-k index list, is what's needed.

## Parameters

`batch`
: Must pass [`validate_labeled`](../data/LabeledBatch.md); `batch.y` must
  be finite (no `NaN`) — a `NaN` label fails outright rather than being
  skipped.

`options` (`FeatureRankOptions`, default `{}`)
: Selects the scoring rule.

| Field | Default | Meaning |
|---|---|---|
| `mode` | `ClassF` | `ClassF` uses ANOVA F for classification; `RegressionF` uses correlation-based F for regression. |
| `absolute_score` | `true` | Has no observable effect on either F score. |

`ClassF` computes, per feature, the one-way ANOVA F-ratio
`MS_between / MS_within` across `batch.y`'s distinct values. `RegressionF`
computes `r^2 / (1 - r^2) * (n - 2)` from Pearson correlation with `batch.y`
as a continuous target. Both formulas are non-negative, so `absolute_score`
does not change either result. See [Example 4](#example-4).

## Returns

A `StatusOr<FeatureRankReport>`:

### `classic::FeatureRankReport` fields

| Field | Type | Default |
|---|---|---|
| `scores` | `pluss::Series` | — |
| `ranked_feature_indices` | `pluss::Series` | — |

`scores` is `feature_count()`-long, in the same column order as
`batch.features.x` — never reordered. `ranked_feature_indices` is also
`feature_count()`-long (every feature, not a top-k subset), sorted by
score descending; a tie is broken by **ascending original index** (a
stable sort under a strict `>` comparator), so two features with
identical scores always resolve in original column order. See
[Example 5](#example-5) for a forced tie.

## Errors

- `batch` fails [`validate_labeled`](../data/LabeledBatch.md), `batch.y`
  contains a `NaN`, or `batch.y`'s length does not match the sample count
  — `Status::invalid_argument` / `Status::shape_mismatch`.
- `mode = ClassF`: fewer than two distinct values in `batch.y` —
  `Status::invalid_argument`.
- `mode = RegressionF`: fewer than 3 samples, or `batch.y` is constant
  (zero variance) — `Status::invalid_argument`.

## Practical Examples

The default `ClassF` mode on a clean two-feature separation, `RegressionF`
mode on a correlated target, `absolute_score` proven to change nothing,
`ranked_feature_indices`'s tie-break rule forced directly, and an error
roundup covering both modes.

### Example 1

Two features: one cleanly separates two classes, one is unrelated noise —
`ClassF`'s default ranking puts the separating feature first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 separates the two classes cleanly; feature 1 is unrelated noise.
  const cls::Series x = pluss::make_series(
      {0.0, 5.0,
       0.1, 1.0,
       0.2, 9.0,
       10.0, 2.0,
       10.1, 8.0,
       10.2, 3.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::FeatureRankReport report = cls::rank_features(batch).value();
  std::cout << "scores = " << report.scores << "\n";
  std::cout << "ranked_feature_indices = " << report.ranked_feature_indices << "\n";

  return 0;
}
```

Output

```text
scores = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[15000, 0.050632911]
ranked_feature_indices = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 2

`ClassF` with **three** classes — the ANOVA F-statistic generalizes
directly, no special two-class case.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 5.1, 5.0, 5.2, 5.1, 4.9, 10.0, 5.0, 10.1, 5.1},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::FeatureRankReport report = cls::rank_features(batch).value();
  std::cout << "scores (feature 0 separates 3 classes, feature 1 does not) = " << report.scores << "\n";

  return 0;
}
```

Output

```text
scores (feature 0 separates 3 classes, feature 1 does not) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10000, 8.6057553e-29]
```

### Example 3

`RegressionF` mode: one feature strongly correlated with a continuous
target, one uncorrelated.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 = y exactly (perfect correlation); feature 1 is unrelated.
  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 2.0, 1.0, 3.0, 9.0, 4.0, 2.0, 5.0, 8.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeatureRankOptions options{};
  options.mode = cls::FeatureScoreMode::RegressionF;
  const cls::FeatureRankReport report = cls::rank_features(batch, options).value();
  std::cout << "scores = " << report.scores << "\n";
  std::cout << "ranked_feature_indices (feature 0 first) = " << report.ranked_feature_indices << "\n";

  return 0;
}
```

Output

```text
scores = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.077434e+15, 0.32594235]
ranked_feature_indices (feature 0 first) = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 4

`absolute_score = false` produces **byte-identical** scores to the
default `true` — both F-statistic formulas are already non-negative by
construction.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

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

  cls::FeatureRankOptions absolute{};
  cls::FeatureRankOptions signed_scores{};
  signed_scores.absolute_score = false;

  const cls::Series abs_scores = cls::rank_features(batch, absolute).value().scores;
  const cls::Series raw_scores = cls::rank_features(batch, signed_scores).value().scores;
  const double diff = (abs_scores - raw_scores).abs().amax(0, false).item<double>();
  std::cout << "max abs diff (absolute_score has no effect) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (absolute_score has no effect) = 0
```

### Example 5

A forced tie: two features engineered to have **identical** scores —
`ranked_feature_indices` resolves the tie by ascending original index.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // features 0 and 1 are identical copies of each other -- same F-score.
  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 10.0, 10.0, 11.0, 11.0, 12.0, 12.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::FeatureRankReport report = cls::rank_features(batch).value();
  std::cout << "scores (tied) = " << report.scores << "\n";
  std::cout << "ranked_feature_indices (tie -> ascending index: [0, 1]) = " << report.ranked_feature_indices << "\n";

  return 0;
}
```

Output

```text
scores (tied) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[150, 150]
ranked_feature_indices (tie -> ascending index: [0, 1]) = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 6

An error roundup: a `NaN` label, a single-class `ClassF` target, too few
samples for `RegressionF`, and a constant `RegressionF` target.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({2, 2});

  cls::LabeledBatch nan_labels{};
  nan_labels.features.x = x;
  nan_labels.y = pluss::make_series({0.0, nan}, {2}, f64);
  std::cout << "NaN label ok           = " << std::boolalpha << cls::rank_features(nan_labels).ok() << "\n";

  cls::LabeledBatch single_class{};
  single_class.features.x = x;
  single_class.y = pluss::make_series({1.0, 1.0}, {2}, f64);
  std::cout << "single-class ClassF ok = " << cls::rank_features(single_class).ok() << "\n";

  const cls::Series small_x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  cls::LabeledBatch too_few{};
  too_few.features.x = small_x;
  too_few.y = pluss::make_series({5.0}, {1}, f64);
  cls::FeatureRankOptions regression{};
  regression.mode = cls::FeatureScoreMode::RegressionF;
  std::cout << "1 sample RegressionF ok = " << cls::rank_features(too_few, regression).ok() << "\n";

  cls::LabeledBatch constant_target{};
  constant_target.features.x = x;
  constant_target.y = pluss::make_series({5.0, 5.0}, {2}, f64);
  std::cout << "constant target RegressionF ok = " << cls::rank_features(constant_target, regression).ok() << "\n";

  return 0;
}
```

Output

```text
NaN label ok           = false
single-class ClassF ok = false
1 sample RegressionF ok = false
constant target RegressionF ok = false
```

## Changelog

- **0.1** — `rank_features`, `FeatureRankOptions`, `FeatureScoreMode`,
  `FeatureRankReport`.

## See Also

- [`pick_features`](pick_features.md) — the same scoring, reduced to a
  top-k/top-fraction `FeatureBatch`.
- [`f_score_rank`](f_score_rank.md) / [`f_regression_rank`](f_regression_rank.md)
  — thin `top_k`-only convenience wrappers, one per mode.
- [`LabeledBatch`](../data/LabeledBatch.md) — the input type.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
