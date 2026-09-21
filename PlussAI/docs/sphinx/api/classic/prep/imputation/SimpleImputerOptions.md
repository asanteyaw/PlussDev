# `pluss::ai::classic::prep::SimpleImputerOptions`

The fill strategy, constant value, and empty-feature fallback consumed by
[`SimpleImputer::learn`](SimpleImputer.md).

## Fields

### `strategy`

`SimpleImputerStrategy strategy = SimpleImputerStrategy::Mean`

Which statistic `learn` fits per feature:

- `Mean` (the default) — the arithmetic mean of the feature's non-`NaN`
  values. Sensitive to outliers, like any mean.
- `Median` — the linearly-interpolated median, resistant to outliers the
  way [`RobustScaler`](../scaling/RobustScaler.md) is.
- `MostFrequent` — the single most common value, ties broken toward the
  smaller one for a deterministic result.
- `Constant` — always `constant_value`, ignoring the training data (aside
  from checking whether the feature is entirely missing — see
  `empty_feature_fallback`).

### `constant_value`

`double constant_value = 0.0`

The fill value used when `strategy = Constant`. Ignored by every other
strategy.

### `empty_feature_fallback`

`double empty_feature_fallback = 0.0`

The fill value fitted for a feature with **zero** non-`NaN` values in the
training batch, regardless of `strategy` — there is no mean, median, or
mode to compute from an empty column, so every strategy falls back to this
one shared value instead.

## Guidance

`Mean` is the right default for a genuinely continuous feature; switch to
`Median` the moment you suspect outliers are present. `MostFrequent` is the
natural choice for a feature that is really categorical (already numerically
encoded) rather than continuous. Reach for `Constant` when a specific
sentinel value (rather than any data-derived statistic) is what a downstream
model or reporting step expects to see in place of a missing value.

## See Also

- [`SimpleImputer`](SimpleImputer.md) — the consuming type.
- [`imputation/Introduction`](Introduction.md) — why every imputer in this
  sub-family shares an `empty_feature_fallback`-shaped field.
