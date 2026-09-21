# `pluss::ai::classic::prep::MinMaxScalerOptions`

The output interval, clamping behavior, and range floor consumed by
[`MinMaxScaler::learn`](MinMaxScaler.md) / `map`.

## Fields

### `lower`

`double lower = 0.0`

The output value a feature's fitted minimum maps to. Must be finite and
strictly less than `upper`; `learn` rejects any other combination with
`Status::invalid_argument`.

### `upper`

`double upper = 1.0`

The output value a feature's fitted maximum maps to. Together `lower` and
`upper` define the target interval — `[0, 1]` by default, but any finite,
strictly-increasing pair is valid (`[-1, 1]`, `[0, 100]`, …).

### `clip`

`bool clip = false`

Whether `map` clamps its output into `[lower, upper]` after the affine map.
`false` (the default) lets a `map` call on data outside the fitted
`[min, max]` produce output outside `[lower, upper]` too — sometimes exactly
what you want (the scaler is not silently hiding that test data extends
past training data); set `true` when a downstream consumer genuinely
requires the output to never leave `[lower, upper]`, at the cost of losing
that signal.

### `range_floor`

`double range_floor = 1e-12`

The minimum `|max - min|` `map` will actually divide by. Below this floor,
every value for that feature maps to `lower` instead — the mechanism that
keeps a constant (or entirely-missing) feature from producing `inf`/`NaN`
output. Must be a positive, finite number.

## Guidance

Set `lower` / `upper` to match whatever downstream consumer needs a bounded
range — most commonly `[0, 1]` (the default) or `[-1, 1]`. Reach for
`clip = true` only when you have confirmed you want out-of-range test data
silently clamped rather than surfaced; leaving it `false` is usually the
better default for catching distribution shift between train and inference
data.

## See Also

- [`MinMaxScaler`](MinMaxScaler.md) — the consuming type.
- [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback) — the
  fallback mechanism compared across all four scalers.
