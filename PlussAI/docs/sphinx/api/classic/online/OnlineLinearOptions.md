# `pluss::ai::classic::online::OnlineLinearOptions`

Everything [`OnlineLinearClassifier`](OnlineLinearClassifier.md) /
[`OnlineLinearRegressor`](OnlineLinearRegressor.md) need to configure their
gradient-descent fit: which linear objective, how strongly regularized,
and the learning-rate schedule driving every
[`advance`](OnlineLinearClassifier.md#methods) step. Fixed for the
lifetime of the estimator — set once at construction (directly, or via
[`make_online_linear_classifier`](make_online_linear_classifier.md) /
[`make_online_linear_regressor`](make_online_linear_regressor.md)), never
changed afterward.

## Fields

### `family`

`LinearFamilyOptions family = {}`

Reuses [`LinearFamilyOptions`](../families/LinearFamilyOptions.md), but
only five of its fields are ever read here — `elastic_l1_ratio` and every
other field on that struct is silently ignored:

- `objective` — which of the four `LinearObjective` values is accepted
  depends on which class reads it:
  [`OnlineLinearClassifier`](OnlineLinearClassifier.md) requires
  `Logistic` or `Hinge`; [`OnlineLinearRegressor`](OnlineLinearRegressor.md)
  requires `LeastSquares` or `RobustLeast`. Every other value —
  including the two the *other* class would accept — is rejected. See
  each class's own `## Errors`.
- `regularization` — only `RegularizationKind::None` or `::L2` is
  supported by **either** class; `L1`/`Elastic` are rejected outright
  (unlike the batch `Linear` family, which supports all four).
- `regularization_strength` — the L2 penalty weight, read only when
  `regularization = L2`. Must be finite and `>= 0`.
- `fit_intercept` — whether a constant `1.0` column is prepended to the
  design matrix before every fit/update step, exactly as in the batch
  `Linear` family.
- `robust_transition` — read only by
  [`OnlineLinearRegressor`](OnlineLinearRegressor.md), and only used when
  `objective = RobustLeast` — but **validated unconditionally** regardless
  of `objective`, so an invalid `robust_transition` breaks a plain
  `LeastSquares` online regressor too, even though that objective never
  reads it. See
  [`OnlineLinearRegressor`'s Example 7](OnlineLinearRegressor.md#example-7).

### `learning_rate`

`double learning_rate = 5e-2`

The base step size every [`advance`](OnlineLinearClassifier.md#methods)
call scales by — see `decaying_rate` below for exactly how. Must be finite
and `> 0`.

### `decaying_rate`

`bool decaying_rate = true`

Whether the effective learning rate shrinks as more updates accumulate.
Both classes track an internal update counter (starting at `0`, one
[`advance`](OnlineLinearClassifier.md#methods) call — including every
internal call `learn` itself makes — increments it by exactly `1`) and
compute the step actually applied as:

- `true` (the default): `learning_rate / sqrt(1 + updates)` — the step
  shrinks over time, the classic Robbins-Monro schedule that lets an
  online fit converge instead of oscillating forever around the optimum.
- `false`: `learning_rate`, unchanged for the estimator's entire lifetime.

See
[`OnlineLinearClassifier`'s Example 7](OnlineLinearClassifier.md#example-7)
for the two schedules compared directly.

## Guidance

Leave `decaying_rate` at its default `true` for the ordinary case — a
constant rate that never shrinks risks oscillating around the optimum
once the fit is close, particularly on a batch fed through many
[`advance`](OnlineLinearClassifier.md#methods) calls. Reach for `false`
only when you specifically want every update to matter equally regardless
of how many have already happened — for instance, deliberately tracking a
target that drifts over time, where an old, heavily-decayed rate would
make the estimator too slow to adapt to new data.

## See Also

- [`OnlineLinearClassifier`](OnlineLinearClassifier.md) /
  [`OnlineLinearRegressor`](OnlineLinearRegressor.md) — the two consumers.
- [`make_online_linear_classifier`](make_online_linear_classifier.md) /
  [`make_online_linear_regressor`](make_online_linear_regressor.md) — the
  entry points that validate these fields eagerly, before any data is
  seen.
- [`LinearFamilyOptions`](../families/LinearFamilyOptions.md) — the full
  struct `family` embeds, including the fields this module never reads.
