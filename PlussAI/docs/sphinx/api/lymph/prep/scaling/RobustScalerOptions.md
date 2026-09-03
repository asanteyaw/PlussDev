# `lymph::options::RobustScaler`

`options::RobustScaler` configures `prep::RobustScaler` and the
[`prep::stage::robust_scaler`](../stage/robust_scaler.md) stage — the two share
this struct. `lymph::prep::RobustScalerOptions` is the family-scoped alias for the
same type.

## Fields

### `with_centering`

`bool with_centering = true` — subtract the column median. Turn off to scale by
spread while leaving the column's location in place.

### `with_scaling`

`bool with_scaling = true` — divide by the quantile range. Turn off to centre on
the median without changing the unit of measure.

### `lower_quantile`

`double lower_quantile = 0.25` — the lower percentile of the spread window.

### `upper_quantile`

`double upper_quantile = 0.75` — the upper percentile of the spread window. The
default `0.25` / `0.75` is the interquartile range; a wider band such as
`0.10` / `0.90` gives a larger divisor and smaller scaled values. Keep
`lower_quantile < upper_quantile`, both inside `(0, 1)`.

### `range_floor`

`double range_floor = 1e-12` — lower bound on `upper_quantile - lower_quantile`
value spread, guarding the division for a near-constant column.

## Guidance

Robust scaling exists for columns with outliers: the median and quartiles are
barely moved by a few extreme values, so the bulk of the column keeps a usable
spread. When the column is clean, [`options::StandardScaler`](StandardScalerOptions.md)
is simpler and equivalent in spirit.

## See Also

- [`prep::stage::robust_scaler`](../stage/robust_scaler.md)
