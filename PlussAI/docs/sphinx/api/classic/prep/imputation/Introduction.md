# `classic::prep::imputation` Introduction

## The Shared Contract

Every imputer implements [`TransformerBase`](../../estimators/TransformerBase.md)
identically in shape: `learn` fits per-feature fill information from
non-`NaN` values only, `map` replaces every `NaN` it finds with that fitted
information and leaves every present value untouched. A feature with **no**
non-`NaN` values at all in the training batch fits its `empty_feature_fallback`
option instead of a real statistic — every imputer in this sub-family has
this field, since "every value in this column was missing" is a real
possibility a fit has to answer regardless of strategy.

## Choosing Among The Three

- [`SimpleImputer`](SimpleImputer.md) — the default choice. Cheap, one fixed
  number (or the most-frequent value) per feature, no relationship between
  features assumed or used.
- [`NeighborImputer`](NeighborImputer.md) — reach for this when rows with
  similar feature profiles genuinely tend to share similar values for the
  missing one too (the same assumption k-nearest-neighbors classification
  makes, applied to filling a value instead of predicting a label).
- [`IterativeImputer`](IterativeImputer.md) — reach for this when features
  are linearly correlated enough that a per-feature ridge regression against
  every other feature outperforms either fixed statistic or a
  neighbor-average — at the cost of being the most expensive of the three to
  fit and the only one whose features are not filled independently of each
  other.

## Reading Order

1. [`SimpleImputer`](SimpleImputer.md) — read this first; it introduces
   `empty_feature_fallback`, the field every imputer in this sub-family
   shares.
2. [`NeighborImputer`](NeighborImputer.md) / [`IterativeImputer`](IterativeImputer.md) —
   reached for by name once a project's data specifically calls for one.

## Related Pages

- [`prep`](../../prep.md) — the parent family.
- [`scaling`](../scaling.md) — a sibling sub-family with its own shared
  degenerate-input fallback pattern, the same spirit as
  `empty_feature_fallback` here.
- [`compose`](../../compose.md) — `Flow::add_simple_imputer` and its two
  siblings.
