# `lymph::options::StandardScaler`

`options::StandardScaler` configures [`prep::StandardScaler`](StandardScaler.md)
and the [`prep::stage::standard_scaler`](../stage/standard_scaler.md) stage — the
two share this struct. `lymph::prep::StandardScalerOptions` is the family-scoped
alias for the same type.

## Fields

### `with_centering`

`bool with_centering = true` — subtract the learned column mean before scaling.
This is the step that re-centres a feature on zero; it is usually the right
default for optimization-sensitive learners.

### `with_scaling`

`bool with_scaling = true` — divide the centred feature by its learned standard
deviation. This is the step that removes unit-driven magnitude imbalance across
numeric features.

### `variance_floor`

`double variance_floor = 1e-12` — numerical safety floor for near-constant
features. It prevents unstable division when a feature's variance is extremely
small.

## Guidance

The defaults are full standardization: centre, scale, and guard against
near-zero variance. Turn off centering or scaling only for a specific downstream
reason. When a column has outliers, [`options::RobustScaler`](RobustScalerOptions.md)
resists them where this struct does not.

## See Also

- [`prep::stage::standard_scaler`](../stage/standard_scaler.md)
- [`options::MinMaxScaler`](MinMaxScalerOptions.md),
  [`options::RobustScaler`](RobustScalerOptions.md),
  [`options::MaxAbsScaler`](MaxAbsScalerOptions.md)
