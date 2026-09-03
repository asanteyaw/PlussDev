# `lymph::options::MinMaxScaler`

`options::MinMaxScaler` configures `prep::MinMaxScaler` and the
[`prep::stage::minmax_scaler`](../stage/minmax_scaler.md) stage — the two share
this struct. `lymph::prep::MinMaxScalerOptions` is the family-scoped alias for the
same type.

## Fields

### `lower`

`double lower = 0.0` — the value the learned column minimum is mapped to. With
the default `upper`, the fitted range becomes `[0, 1]`.

### `upper`

`double upper = 1.0` — the value the learned column maximum is mapped to. Set
`lower`/`upper` together to target a different interval, e.g. `{-1, 1}`. Keep
`lower < upper`.

### `clip`

`bool clip = false` — when `true`, a value outside the learned `[min, max]` seen
at replay time is held to `[lower, upper]` instead of mapping past it. Matters
only when later frames can exceed the fitted range.

### `range_floor`

`double range_floor = 1e-12` — lower bound on `max - min`, so a near-constant
column does not produce an exploding divisor. Rarely changed.

## Guidance

Reach for min-max scaling when a downstream step expects inputs in a known,
bounded interval, or when the distribution's shape should be preserved (no
re-centring). For zero-mean / unit-variance scaling use
[`options::StandardScaler`](StandardScalerOptions.md); to divide by magnitude
without shifting zero use [`options::MaxAbsScaler`](MaxAbsScalerOptions.md).

## See Also

- [`prep::stage::minmax_scaler`](../stage/minmax_scaler.md)
