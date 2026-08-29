# `lymph::prep::StandardScalerOptions`

`StandardScalerOptions` configures how numeric features are centered and scaled.

## Field Reference

### `bool with_centering = true`

If `true`, the learned mean is subtracted from each feature before any scaling.

This is the option that recenters features around zero. It is usually the right
default for optimization-sensitive learners.

### `bool with_scaling = true`

If `true`, each centered feature is divided by a learned scale.

This is the option that removes unit-driven magnitude imbalance across numeric
features.

### `double variance_floor = 1e-12`

This is the numerical safety floor for near-constant features.

It prevents unstable division when a feature’s variance is extremely small.

## Guidance

The default settings are the standard full-standardization shape:

- center
- scale
- guard against near-zero variance

Turn off centering or scaling only when you have a specific downstream reason,
not because the defaults are arbitrary.

## Related Pages

- [StandardScaler](StandardScaler.md)
- [../stage/standard_scaler.md](../stage/standard_scaler.md)
