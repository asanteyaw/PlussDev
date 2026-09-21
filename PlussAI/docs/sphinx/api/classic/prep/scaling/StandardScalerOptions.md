# `pluss::ai::classic::prep::StandardScalerOptions`

Centering / scaling toggles and the variance floor consumed by
[`StandardScaler::learn`](StandardScaler.md).

## Fields

### `with_centering`

`bool with_centering = true`

Whether the fitted center is the column's mean (`true`, the default) or `0`
(`false`, meaning `map` only divides, never shifts). Set `false` when the
column is already meaningfully centered (e.g. a signed residual, or a value
where `0` has intrinsic meaning you do not want to erase).

### `with_scaling`

`bool with_scaling = true`

Whether the fitted scale is the column's population standard deviation
(`true`, the default) or `1` (`false`, meaning `map` only shifts, never
divides). Set `false` when you want centering without changing the column's
spread — rare, but occasionally useful ahead of a model that is itself
scale-invariant.

### `variance_floor`

`double variance_floor = 1e-12`

The minimum standard deviation `learn` will actually divide by. A fitted
`stddev` that is non-finite or does not exceed this floor is replaced with
`1` instead — the mechanism that keeps a constant (zero-variance) or
entirely-missing feature from producing `inf`/`NaN` output. Must be a
positive, finite number; `learn` rejects any other value with
`Status::invalid_argument` before reading data. Raise it above the default
only if you specifically want to treat a near-constant feature (not just an
exactly-constant one) as degenerate too.

## Guidance

Leave both toggles `true` and `variance_floor` at its default for the
ordinary case — every field here exists for a specific, narrower situation
than "scale my features." Touch `with_centering` / `with_scaling`
independently only when you have a concrete reason a full standardization is
wrong for one column; touch `variance_floor` only if the default `1e-12` is
letting through a feature you consider degenerate for your own numerical
reasons.

## See Also

- [`StandardScaler`](StandardScaler.md) — the consuming type; see its
  `learn` for the exact fallback behavior when `variance_floor` is exceeded.
- [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback) — the
  fallback mechanism compared across all four scalers.
