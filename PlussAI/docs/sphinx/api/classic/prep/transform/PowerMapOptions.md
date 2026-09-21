# `pluss::ai::classic::prep::PowerMapOptions`

The transform family, standardization toggle, λ (or its search range), and
variance floor consumed by [`PowerMap::learn`](PowerMap.md) / `map`.

## Fields

### `mode`

`PowerMapMode mode = PowerMapMode::YeoJohnson`

Which power-transform family to fit:

- `YeoJohnson` (the default) — handles any real-valued input, including
  zero and negative values, by branching internally on the value's sign.
- `BoxCox` — the classic Box-Cox transform; requires every value (at both
  `learn` and `map` time) to be strictly positive.

### `standardize`

`bool standardize = true`

Whether the power-transformed output is further standardized to zero mean
and unit variance, the same step [`StandardScaler`](../scaling/StandardScaler.md)
performs. `true` (the default) is the usual choice when the transform feeds
a model sensitive to input scale; set `false` to inspect or use the power
transform's own output directly.

### `lambda`

`std::optional<double> lambda = std::nullopt`

An explicit λ, bypassing the grid search entirely when set. `std::nullopt`
(the default) fits λ per feature via `lambda_min`/`lambda_max`/
`lambda_grid`. Must be finite when supplied.

### `lambda_min`

`double lambda_min = -5.0`

The lower end of the search range for λ when it is not supplied explicitly.
Must be finite, and less than `lambda_max`. Widening the range considers
more extreme transforms; narrowing it restricts the search to milder ones.

### `lambda_max`

`double lambda_max = 5.0`

The upper end of the search range for λ. Must be finite, and greater than
`lambda_min`.

### `lambda_grid`

`std::int64_t lambda_grid = 201`

How many evenly-spaced λ candidates the grid search evaluates between
`lambda_min` and `lambda_max`. Must be `>= 2`. A finer grid (larger value)
finds a more precise λ at proportionally higher fit cost; ignored entirely
when `lambda` is supplied.

### `variance_floor`

`double variance_floor = 1e-12`

Only consulted when `standardize` is on — the same degenerate-variance
guard [`StandardScalerOptions::variance_floor`](../scaling/StandardScalerOptions.md#variance_floor)
documents, applied to the power-transformed values' fitted standard
deviation.

## Guidance

Leave `mode = YeoJohnson` unless you specifically want the classic Box-Cox
formula and know every value is strictly positive. Leave `lambda` unset for
the ordinary case; supply it directly only when you already know the right
λ (from a prior fit, domain knowledge, or wanting a reproducible, search-free
transform) and want to skip the grid search's cost and any run-to-run
sensitivity to the search range.

## See Also

- [`PowerMap`](PowerMap.md) — the consuming type.
- [`StandardScalerOptions`](../scaling/StandardScalerOptions.md) — the
  standardization step's own options, mirrored here via `variance_floor`.
