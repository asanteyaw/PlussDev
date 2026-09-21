# `pluss::ai::classic::prep::RobustScalerOptions`

Centering / scaling toggles, the quantile bracket defining the spread, and
the range floor consumed by [`RobustScaler::learn`](RobustScaler.md).

## Fields

### `with_centering`

`bool with_centering = true`

Whether the fitted center is the column's median (`true`, the default) or `0`
(`false`). Mirrors [`StandardScalerOptions::with_centering`](StandardScalerOptions.md).

### `with_scaling`

`bool with_scaling = true`

Whether the fitted scale is the column's interquartile range (`true`, the
default) or `1` (`false`). Mirrors
[`StandardScalerOptions::with_scaling`](StandardScalerOptions.md).

### `lower_quantile`

`double lower_quantile = 0.25`

The lower edge of the quantile bracket whose width becomes the fitted spread
— `0.25` (the first quartile) by default. Must be in `[0, 1]` and strictly
less than `upper_quantile`.

### `upper_quantile`

`double upper_quantile = 0.75`

The upper edge of the quantile bracket — `0.75` (the third quartile) by
default, giving the classic interquartile range together with
`lower_quantile`'s default. Narrowing the bracket (moving the two closer
together) shrinks the fitted spread and makes the scaler more sensitive to
the data near the median; widening it moves the behavior closer to
`StandardScaler`.

### `range_floor`

`double range_floor = 1e-12`

The minimum `|upper_quantile value - lower_quantile value|` `learn` will
actually divide by. Below this floor, that feature's scale falls back to `1`
— the mechanism that keeps a constant (or entirely-missing) feature from
producing `inf`/`NaN` output. Must be a positive, finite number.

## Guidance

Leave `lower_quantile` / `upper_quantile` at the classic `[0.25, 0.75]`
quartile bracket unless you have a specific reason to change how much of the
distribution's tails influence the fitted spread — widening the bracket
trades outlier-resistance for closer agreement with `StandardScaler`;
narrowing it increases outlier-resistance further at the cost of relying on
fewer data points near the median.

## See Also

- [`RobustScaler`](RobustScaler.md) — the consuming type.
- [`StandardScalerOptions`](StandardScalerOptions.md) — the non-robust
  counterpart's equivalent toggles.
- [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback) — the
  fallback mechanism compared across all four scalers.
