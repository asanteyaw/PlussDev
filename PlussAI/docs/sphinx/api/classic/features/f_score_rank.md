# `pluss::ai::classic::f_score_rank`

The narrowest ranking entry point in this family: ANOVA F-score feature
ranking for a classification target, returning **only** the top-`k`
feature indices as a flat `Series` — no scores, no full ranking, no
reduced batch. A thin, fixed-`top_k`-only wrapper over
[`rank_features`](rank_features.md#returns) with `mode` forced to
`ClassF`; reach for [`pick_features`](pick_features.md) instead when you
also need the reduced `FeatureBatch` itself, or a `fraction`-based count
rather than a fixed `top_k`.

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::f_score_rank(
    const classic::LabeledBatch& batch,
    const classic::ScoreRankOptions& options = {});
```

## What It Does

Scores every feature in `batch` by its ANOVA F-statistic against a
classification target (`batch.y`), then returns only the indices of the
`top_k` highest-scoring features -- a direct call into
[`rank_features`](rank_features.md) with `mode` fixed to `ClassF`, for
callers who just want the winning indices without setting up the more
general call themselves.

## Parameters

`batch`
: Identical contract to [`rank_features`'s `batch`](rank_features.md#parameters)
  — `batch.y`'s distinct values are treated as class labels; at least two
  are required.

`options` (`ScoreRankOptions`, default `{}`)
: ```cpp
  struct classic::ScoreRankOptions {
    std::int64_t top_k = 10;
    bool absolute_score = true;
  };
  ```

  `top_k` (default `10`) — how many top-ranked feature indices to return,
  capped at `feature_count()` (never an error to ask for more than
  exist). Unlike [`FeaturePickOptions`](pick_features.md#parameters),
  there is **no `fraction` field** here at all — `f_score_rank` only
  supports a fixed count. Must be `> 0` — checked explicitly by this
  function itself, before `rank_features` ever runs.

  `absolute_score` — identical contract to
  [`rank_features`'s own field](rank_features.md#parameters), including
  having no observable effect on the ANOVA F-statistic, which is already
  non-negative. See [Example 3](#example-3).

## Returns

A `StatusOr<pluss::Series>` — `Int64`, length `min(options.top_k,
feature_count())`, the same values as
[`rank_features`'s](rank_features.md#returns) own
`ranked_feature_indices`, truncated to the first `top_k` entries. See
[Example 2](#example-2) for the exact equivalence, proven directly against
[`pick_features`](pick_features.md#returns)'s own `selected_feature_indices`.

## Errors

- `options.top_k <= 0` — `Status::invalid_argument`, checked before
  `batch` is even validated.
- Everything [`rank_features`](rank_features.md#errors) with `mode =
  ClassF` can already fail with (a `NaN` label, fewer than two classes,
  a `batch` validation failure).

## Practical Examples

The default `top_k = 10` capped at a smaller real feature count, the
exact equivalence with `pick_features(mode = ClassF)`'s own
`selected_feature_indices`, `absolute_score` proven to change nothing, and
the `top_k <= 0` error.

### Example 1

Three features, only two class-relevant — `top_k`'s default `10` is
silently capped at `feature_count() = 3`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 1.0, 1.0, 0.1, 2.0, 9.0, 0.2, 3.0,
       2.0, 10.0, 4.0, 8.0, 10.1, 5.0, 3.0, 10.2, 6.0},
      {18}, f64).reshape({6, 3});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const pluss::Series ranked = cls::f_score_rank(batch).value();
  std::cout << "length (capped at feature_count()=3) = " << ranked.numel() << "\n";
  std::cout << "ranked = " << ranked << "\n";

  return 0;
}
```

Output

```text
length (capped at feature_count()=3) = 3
ranked = pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 0]
```

### Example 2

`f_score_rank`'s output is exactly
[`pick_features(mode = ClassF)`'s](pick_features.md) own
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
      {5.0, 0.0, 1.0, 1.0, 0.1, 2.0, 9.0, 0.2, 3.0,
       2.0, 10.0, 4.0, 8.0, 10.1, 5.0, 3.0, 10.2, 6.0},
      {18}, f64).reshape({6, 3});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions score_options{};
  score_options.top_k = 2;
  const pluss::Series via_f_score_rank = cls::f_score_rank(batch, score_options).value();

  cls::FeaturePickOptions pick_options{};
  pick_options.top_k = 2;
  const pluss::Series via_pick_features = cls::pick_features(batch, pick_options).value().selected_feature_indices;

  const double diff = (via_f_score_rank.to(pluss::ScalarType::Float64)
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

`absolute_score = false` produces the same ranked indices as the default
`true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 1.0, 1.0, 0.1, 2.0, 9.0, 0.2, 3.0,
       2.0, 10.0, 4.0, 8.0, 10.1, 5.0, 3.0, 10.2, 6.0},
      {18}, f64).reshape({6, 3});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions absolute{};
  cls::ScoreRankOptions signed_scores{};
  signed_scores.absolute_score = false;

  const pluss::Series a = cls::f_score_rank(batch, absolute).value();
  const pluss::Series b = cls::f_score_rank(batch, signed_scores).value();
  const double diff = (a.to(pluss::ScalarType::Float64) - b.to(pluss::ScalarType::Float64))
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (absolute_score has no effect) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (absolute_score has no effect) = 0
```

### Example 4

`top_k <= 0` is rejected before `batch` is even validated.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LabeledBatch empty_batch{};
  cls::ScoreRankOptions zero_top_k{};
  zero_top_k.top_k = 0;
  const cls::StatusOr<pluss::Series> rejected = cls::f_score_rank(empty_batch, zero_top_k);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "f_score_rank requires ScoreRankOptions.top_k > 0."
```

## Changelog

- **0.1** — `f_score_rank`, `ScoreRankOptions`.

## See Also

- [`f_regression_rank`](f_regression_rank.md) — the regression-target
  counterpart, sharing `ScoreRankOptions`.
- [`rank_features`](rank_features.md) — the full ranking this narrows.
- [`pick_features`](pick_features.md) — the richer top-k/fraction
  reduction, also returning the reduced `FeatureBatch`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
