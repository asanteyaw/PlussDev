# `pluss::ai::classic::f_regression_rank`

The regression-target counterpart of [`f_score_rank`](f_score_rank.md):
correlation-based F-score feature ranking, returning only the top-`k`
feature indices. A thin, fixed-`top_k`-only wrapper over
[`rank_features`](rank_features.md#returns) with `mode` forced to
`RegressionF`; reach for [`pick_features`](pick_features.md) instead when
you also need the reduced `FeatureBatch` itself, or a `fraction`-based
count rather than a fixed `top_k`.

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::f_regression_rank(
    const classic::LabeledBatch& batch,
    const classic::ScoreRankOptions& options = {});
```

## What It Does

Scores every feature in `batch` by its correlation-based F-statistic
against a continuous target (`batch.y`), then returns only the indices of
the `top_k` highest-scoring features -- a direct call into
[`rank_features`](rank_features.md) with `mode` fixed to `RegressionF`, for
callers who just want the winning indices without setting up the more
general call themselves.

## Parameters

`batch`
: Identical contract to
  [`rank_features`'s `batch` under `RegressionF`](rank_features.md#parameters)
  — `batch.y` treated as a continuous target; at least 3 samples and a
  non-constant target are required.

`options` (`ScoreRankOptions`, default `{}`)
: Identical struct and contract to
  [`f_score_rank`'s own `options`](f_score_rank.md#parameters) — see that
  page for `top_k`/`absolute_score`'s exact meaning. `ScoreRankOptions` is
  shared between both functions; nothing about it is regression-specific.

## Returns

A `StatusOr<pluss::Series>` — `Int64`, length `min(options.top_k,
feature_count())`, the same values as
[`rank_features(mode = RegressionF)`'s](rank_features.md#returns) own
`ranked_feature_indices`, truncated to the first `top_k` entries. See
[Example 2](#example-2) for the exact equivalence, proven directly against
[`pick_features(mode = RegressionF)`](pick_features.md#returns)'s own
`selected_feature_indices`.

## Errors

- `options.top_k <= 0` — `Status::invalid_argument`, checked before
  `batch` is even validated.
- Everything [`rank_features`](rank_features.md#errors) with `mode =
  RegressionF` can already fail with (fewer than 3 samples, a constant
  target, a `batch` validation failure).

## Practical Examples

The default `top_k` on a correlated feature, the exact equivalence with
`pick_features(mode = RegressionF)`'s own `selected_feature_indices`, and
the `top_k <= 0` error.

### Example 1

Three features, one strongly correlated with a continuous target — that
one ranks first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 = y exactly; features 1 and 2 are unrelated.
  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 9.0, 2.0, 1.0, 8.0, 3.0, 9.0, 2.0, 4.0, 2.0, 6.0, 5.0, 8.0, 1.0},
      {15}, f64).reshape({5, 3});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions options{};
  options.top_k = 2;
  const pluss::Series ranked = cls::f_regression_rank(batch, options).value();
  std::cout << "ranked (feature 0 first) = " << ranked << "\n";

  return 0;
}
```

Output

```text
ranked (feature 0 first) = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 2]
```

### Example 2

`f_regression_rank`'s output is exactly
[`pick_features(mode = RegressionF)`'s](pick_features.md) own
`selected_feature_indices`, for the same `top_k`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 9.0, 2.0, 1.0, 8.0, 3.0, 9.0, 2.0, 4.0, 2.0, 6.0, 5.0, 8.0, 1.0},
      {15}, f64).reshape({5, 3});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions score_options{};
  score_options.top_k = 2;
  const pluss::Series via_f_regression_rank = cls::f_regression_rank(batch, score_options).value();

  cls::FeaturePickOptions pick_options{};
  pick_options.mode = cls::FeatureScoreMode::RegressionF;
  pick_options.top_k = 2;
  const pluss::Series via_pick_features = cls::pick_features(batch, pick_options).value().selected_feature_indices;

  const double diff = (via_f_regression_rank.to(pluss::ScalarType::Float64)
                        - via_pick_features.to(pluss::ScalarType::Float64))
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (identical index lists) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (identical index lists) = 0
```

### Example 3

`top_k <= 0` is rejected before `batch` is even validated.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LabeledBatch empty_batch{};
  cls::ScoreRankOptions negative_top_k{};
  negative_top_k.top_k = -1;
  const cls::StatusOr<pluss::Series> rejected = cls::f_regression_rank(empty_batch, negative_top_k);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "f_regression_rank requires ScoreRankOptions.top_k > 0."
```

## Changelog

- **0.1** — `f_regression_rank`.

## See Also

- [`f_score_rank`](f_score_rank.md) — the classification-target
  counterpart, sharing `ScoreRankOptions`.
- [`rank_features`](rank_features.md) — the full ranking this narrows.
- [`pick_features`](pick_features.md) — the richer top-k/fraction
  reduction, also returning the reduced `FeatureBatch`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
