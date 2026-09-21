# `classic::online` Introduction

## What This Family Owns

`online` owns two linear estimators fit by gradient descent, one step at a
time, rather than the closed-form or batch-iterative solvers every family
under [`families`](../families.md) uses:
[`OnlineLinearClassifier`](OnlineLinearClassifier.md) (binary, `Logistic`
or `Hinge`) and [`OnlineLinearRegressor`](OnlineLinearRegressor.md)
(`LeastSquares` or `RobustLeast`). Reach for this family instead of the
batch `Linear` family (under [`families`](../families.md)) specifically
when you want to keep updating a fitted model as new data arrives, without
ever re-fitting from the beginning.

`online` is deliberately **not** part of the [`estimators`](../estimators.md) /
[`compose`](../compose.md) / [`tune`](../tune.md) / [`io`](../io.md)
ecosystem every other family in this reference participates in: there is
no `ModelSpec` alternative for it, `make_classifier`/`make_regressor`
never build one, a [`Flow`](../compose/Flow.md) cannot attach one as its
model, and [`io::save`](../io/save.md) cannot persist one. It is a
self-contained pair of classes with their own `learn`/`advance`/`infer`/
`assess` methods, constructed directly or via
[`make_online_linear_classifier`](make_online_linear_classifier.md) /
[`make_online_linear_regressor`](make_online_linear_regressor.md).

## `learn` Is Not One Step

The single fact most likely to surprise a reader: calling `learn` does
**not** perform one gradient update. It resets all fitted state, then
internally calls [`advance`](OnlineLinearClassifier.md#methods)
`max(1, TrainOptions::max_iterations)` times against the *same* batch —
`max_iterations` defaults to `1000` (see
[`TrainOptions`](../core/TrainOptions.md#max_iterations)), so an ordinary
`learn()` call with default options is closer to a full batch fit than to
a single incremental nudge. `advance`, called on its own after `learn`,
is the one genuinely single-step operation — the "online" part of this
family's name describes `advance`, not `learn`. See
[`OnlineLinearClassifier`'s Example 3](OnlineLinearClassifier.md#example-3)
for `max_iterations` visibly changing how well-fit the result is.

## The Learning-Rate Schedule

Every `advance` step scales
[`OnlineLinearOptions::learning_rate`](OnlineLinearOptions.md#learning_rate)
by an internal update counter — see
[`OnlineLinearOptions::decaying_rate`](OnlineLinearOptions.md#decaying_rate)
for the exact formula. The counter is shared across every `advance` call
an estimator ever makes, including the ones `learn` itself runs
internally — so by the time `learn`'s default `1000`-step bootstrap
finishes, a `decaying_rate = true` estimator's effective rate has already
shrunk by roughly `1/sqrt(1000) ≈ 0.032`x before you ever call `advance`
yourself.

## Unscaled Features Can Stall The Classifier

Neither class standardizes features before fitting — unlike the batch
`Linear` family's iterative solver, `advance`'s gradient step is applied
directly to whatever `x` you pass. For
[`OnlineLinearRegressor`](OnlineLinearRegressor.md), this rarely matters:
the least-squares gradient never saturates, so it converges even from a
feature range far from zero. For
[`OnlineLinearClassifier`](OnlineLinearClassifier.md), it can matter a
great deal: with `decaying_rate = true` (the default) shrinking the step
size quickly, a `Logistic`/`Hinge` fit whose features all sit far from
zero can get the intercept and slope stuck pushing the decision boundary
to one side of every training point — every row predicted the same class,
accuracy at chance — because the sigmoid's gradient has already nearly
saturated by the time the rate has decayed enough to stop overshooting.
Centering features (or at least keeping them close to zero) before
fitting avoids this. Every classifier example on this page uses centered
features for exactly this reason — verified by comparing against the
same data left uncentered, which does get stuck this way.

## Two Classes, One Shape

`OnlineLinearClassifier` and `OnlineLinearRegressor` share the same method
shape (`learn`/`advance`/`infer`/`assess`, plus `trained`/`seen_samples`)
and the same [`OnlineLinearOptions`](OnlineLinearOptions.md), but are not
interchangeable: each validates a disjoint pair of `LinearObjective`
values (`Logistic`/`Hinge` for the classifier, `LeastSquares`/`RobustLeast`
for the regressor), and only the classifier's `learn` reads
[`TrainOptions::balance`](../core/TrainOptions.md#balance) at all —
the regressor's `learn` never rebalances rows, since row balancing is a
classification-imbalance concept with no regression analogue. See
[`OnlineLinearRegressor`'s Example 11](OnlineLinearRegressor.md#example-11)
for that asymmetry demonstrated directly.

## Reading Order

1. [`OnlineLinearOptions`](OnlineLinearOptions.md) — the shared
   configuration both classes take at construction.
2. [`OnlineLinearClassifier`](OnlineLinearClassifier.md) — the richer of
   the two (binary label handling, more supported metrics).
3. [`OnlineLinearRegressor`](OnlineLinearRegressor.md) — the same shape,
   without the label bookkeeping.
4. [`make_online_linear_classifier`](make_online_linear_classifier.md) /
   [`make_online_linear_regressor`](make_online_linear_regressor.md) —
   the eager-validation entry points.

## Related Pages

- [`families`](../families.md) — `LinearFamilyOptions`, the struct
  `OnlineLinearOptions::family` embeds.
- [`core`](../core.md) — `TrainOptions`, including `max_iterations` (the
  bootstrap-step count inside `learn`) and `balance` (the classifier-only
  row-rebalancing toggle).
- [`metrics`](../metrics.md) — the same metric formulas `assess` computes
  here, without the configurability (`ClassReportOptions`) the full
  `classification_summary` path offers.
