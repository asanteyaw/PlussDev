# `classic::features` Introduction

## What This Family Owns

`features` owns six free functions for reducing a feature matrix's column
count — one unsupervised
([`variance_prune`](variance_prune.md), dropping near-constant columns),
four supervised and score-based
([`rank_features`](rank_features.md)/[`pick_features`](pick_features.md)/
[`f_score_rank`](f_score_rank.md)/[`f_regression_rank`](f_regression_rank.md),
all built on the same univariate F-statistic), and one plain index-based
selector ([`take_features`](take_features.md)) the other five all call
internally to materialize their own reduced result. Unlike every family
with a class hub (`estimators`, `sparse`, `text`), there is no persistent
object here at all — every function takes a
[`FeatureBatch`](../data/FeatureBatch.md)/[`LabeledBatch`](../data/LabeledBatch.md)
and a plain options struct, and returns a plain result struct. Nothing is
fitted, nothing is stored, nothing needs a second call.

## Four Functions, One Score

[`rank_features`](rank_features.md), [`pick_features`](pick_features.md),
[`f_score_rank`](f_score_rank.md), and
[`f_regression_rank`](f_regression_rank.md) all compute the exact same two
per-feature scores — a one-way ANOVA F-ratio for `FeatureScoreMode::ClassF`,
a correlation-based F-statistic for `FeatureScoreMode::RegressionF` — and
differ only in what they do with the result:

| Function | Returns | `top_k` or `fraction`? |
| --- | --- | --- |
| [`rank_features`](rank_features.md) | every feature's score, plus every feature ranked | neither — always full-length |
| [`pick_features`](pick_features.md) | a reduced `FeatureBatch` **and** the full ranking | either, via `fraction` overriding `top_k` |
| [`f_score_rank`](f_score_rank.md) | just the top-`k` indices, `ClassF` forced | `top_k` only, no `fraction` field |
| [`f_regression_rank`](f_regression_rank.md) | just the top-`k` indices, `RegressionF` forced | `top_k` only, no `fraction` field |

`f_score_rank`'s and `f_regression_rank`'s own output is always exactly
[`pick_features`'s](pick_features.md) `selected_feature_indices` for the
matching `mode`/`top_k` — confirmed directly, not just by reading the
source, in each page's own equivalence example.

## `absolute_score` Currently Does Nothing

Every one of the four score-based functions above exposes an
`absolute_score` toggle (`true` by default). Both F-statistic formulas —
a ratio of between-class and within-class sums of squares for `ClassF`,
`r^2 / (1 - r^2)` scaled by degrees of freedom for `RegressionF` — are
structurally non-negative by construction, so applying `std::abs` to
either one is a no-op. Verified directly (not just reasoned from the
formulas) in [`rank_features`'s Example 4](rank_features.md#example-4)
and [`f_score_rank`'s Example 3](f_score_rank.md#example-3): `true` and
`false` produce byte-identical scores. This is documented as a real,
honest current limitation on every affected page — not silently worked
around, and not hidden.

## Two Different "Reduced Batch" Column Orders

[`variance_prune`](variance_prune.md) and
[`pick_features`](pick_features.md) are this family's only two functions
that return a reduced `FeatureBatch` directly, and they order its columns
**differently**: `variance_prune`'s `reduced` keeps every surviving
column in ascending original index order (it never reorders by
variance); `pick_features`'s `reduced` is ordered by score descending (the
single best feature is always column `0` of the result, regardless of
where it started). Confirmed side by side on the same underlying data in
[`pick_features`'s Example 5](pick_features.md#example-5).

## Reading Order

1. [`take_features`](take_features.md) — the plain index-based selector
   every other function calls internally.
2. [`variance_prune`](variance_prune.md) — the unsupervised member.
3. [`rank_features`](rank_features.md) — the full-ranking scoring
   primitive.
4. [`pick_features`](pick_features.md) — the richer top-k/fraction
   reduction built on `rank_features`.
5. [`f_score_rank`](f_score_rank.md) / [`f_regression_rank`](f_regression_rank.md)
   — narrow, fixed-`top_k`-only convenience wrappers.

## Related Pages

- [`data`](../data.md) — [`FeatureBatch`](../data/FeatureBatch.md)/
  [`LabeledBatch`](../data/LabeledBatch.md), every function's input type.
- [`sparse`](../sparse.md) — a different, unrelated notion of feature
  selection (sparse matrix column subsetting has no equivalent here; this
  family works only on dense `FeatureBatch`/`LabeledBatch`).
