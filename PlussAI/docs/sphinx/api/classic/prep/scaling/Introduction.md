# `classic::prep::scaling` Introduction

## The Shared Contract

All four scalers — [`StandardScaler`](StandardScaler.md),
[`MinMaxScaler`](MinMaxScaler.md), [`RobustScaler`](RobustScaler.md),
[`MaxAbsScaler`](MaxAbsScaler.md) — implement
[`TransformerBase`](../../estimators/TransformerBase.md) identically in
shape:

- **`learn(batch)`** computes exactly one center value and one spread value
  per feature column (a scaler with no centering step, like `MaxAbsScaler`,
  still tracks a spread; a scaler with centering disabled via its options
  still tracks a center of `0`, not "no center"). Fitting is per-feature and
  independent — column `i`'s statistics never depend on column `j`'s values.
- **`map(batch)`** applies `(value - center) / spread` (or, for
  `MinMaxScaler`, the equivalent min-max affine form) to every non-`NaN`
  entry, using the fitted numbers from the most recent `learn`. `map` never
  looks at the data's actual min/max/mean at call time — it always replays
  the fitted statistics, which is what makes calling `map` on held-out test
  data (rather than the training data `learn` saw) meaningful.
- **A missing value stays missing.** `NaN` entries are excluded from the
  statistics `learn` computes, and pass through `map` unchanged rather than
  being scaled — a scaler is not an imputer; pair one with
  [`imputation`](../imputation.md) first if the downstream model cannot
  tolerate `NaN`.
- **`map` before a successful `learn` fails** with `Status::precondition`,
  message `"<ClassName>::map requires learn() before map()."`.
- **`learn` re-fits from scratch** — calling it again on a new batch (of the
  same feature count) discards the previous fit and replaces it, it does not
  average the two.

## Degenerate-Feature Fallback

Every scaler has a *floor* option guarding its spread computation against a
feature that carries no usable variation — every value identical, or every
non-`NaN` value dropped, or (for `MaxAbsScaler`) every value zero. Rather
than dividing by (near) zero and producing `inf`/`NaN` output, each scaler
falls back to a spread of `1` for that feature, leaving the (already-centered,
where applicable) value unchanged:

| Scaler | Spread it fits | Floor option | Degenerate condition |
| --- | --- | --- | --- |
| [`StandardScaler`](StandardScaler.md) | standard deviation | `variance_floor` (default `1e-12`) | `stddev <= variance_floor`, non-finite, or every value missing |
| [`MinMaxScaler`](MinMaxScaler.md) | `max - min` | `range_floor` (default `1e-12`) | `abs(max - min) < range_floor` — maps every value to `lower` instead of dividing |
| [`RobustScaler`](RobustScaler.md) | interquartile range | `range_floor` (default `1e-12`) | `abs(q_high - q_low) < range_floor`, or every value missing |
| [`MaxAbsScaler`](MaxAbsScaler.md) | max absolute value | `floor` (default `1e-12`) | `max(abs(value)) < floor` — including an all-zero or entirely-missing feature |

Each floor option itself must be a positive, finite number — an invalid floor
(zero, negative, `NaN`, or infinite) is rejected by `learn` before any data is
read, with `Status::invalid_argument`.

## Persistence

`dump_state()` / `load_state(state)` follow the same round-trip pattern as
[`EstimatorBase::dump_state`](../../estimators/EstimatorBase/dump_state.md):
a fitted scaler's state is `[metadata, center_or_min, spread_or_max]` (two
fitted arrays plus metadata — `MaxAbsScaler` alone has only one fitted array,
since it does not center), one `pluss::Series` per element, in that fixed
order. Loading state from a scaler with a different `feature_count` than the
one currently constructed still succeeds — `load_state` replaces
`feature_count()` along with everything else. See
[`StandardScaler`'s Example 5](StandardScaler.md#example-5) for the full
round trip, including the `map`-before-`learn` failure every scaler here
shares.

## Related Pages

- [`prep`](../../prep.md) — the parent family.
- [`compose`](../../compose.md) — `Flow::add_standard_scaler` and its three
  siblings.
- [`estimators`](../../estimators.md) — `TransformerBase`, the shared
  interface.
