# `pluss::ai::classic::TrainOptions`

`TrainOptions` is the per-fit configuration every `learn(...)` accepts,
regardless of estimator family — `ClassifierBase::learn`, `Flow::learn`,
`sweep`/`tune`'s inner fit, the sparse linear learners. It covers five
independent concerns: how missing and unknown-category cells are handled,
solver iteration limits, class weighting, score calibration, and row
balancing. Every field has a default that reproduces the simplest possible
fit — you only set the fields your problem actually needs.

## Fields

### `layout`

`ObservationLayout layout = ObservationLayout::Rows`

Whether the feature matrix has one sample per row (`Rows`, the default and
what every example on this reference uses) or one sample per column
(`Columns`). Set this only when you are handing a pre-transposed `Series`
straight to a `FeatureBatch` / `LabeledBatch` — a `DataFrame` path never needs
it (rows are always samples there).

### `missing`

`MissingPolicy missing = MissingPolicy::Reject`

What to do about missing (`NaN`) cells in the feature matrix at fit time:
`Reject` (the default — a `Status::shape_mismatch` failure if any cell is
missing), `Drop` (drop the offending rows before fitting), `ImputeMean` /
`ImputeMedian` (fill with the column's mean/median, computed on this fit's
data), or `KeepNaN` (pass `NaN` straight through to the estimator — only
correct for a family whose kernel is written to handle it; most are not).
`Reject` is deliberately the default: silently dropping or imputing rows is a
data-shape decision your pipeline should make explicitly, usually with a
[`prep` imputer](../prep/imputation.md) stage rather than this flag.

### `unknown_categorical`

`UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error`

What to do when a categorical feature holds a value the fit has never seen
before — relevant only when the batch came through a category-encoding path.
`Error` (the default — fails the fit), `MissingBucket` (routes it to a
reserved "unknown" code), `Ignore` (drops the distinction, folding it into
whatever bucket the encoder falls back to).

### `max_iterations`

`std::int64_t max_iterations = 1000`

Hard cap on solver iterations for any family that fits iteratively (linear
models under `Logistic`/`Hinge`/`RobustLeast`, k-means, PCA's iterative
solver, the isolation forest's tree count is separate — see
[`EnsembleFamilyOptions`](../families/EnsembleFamilyOptions.md) instead). A
closed-form fit (ordinary least squares) ignores this. Raising it lets a slow
convergence finish; it never changes the answer once the solver has already
converged.

### `tolerance`

`double tolerance = 1e-6`

The convergence threshold paired with `max_iterations` — the solver stops
early once successive iterations change by less than this. Tighter (smaller)
values trade more iterations for a more precise fit; looser values stop
sooner at a coarser one.

### `warm_start`

`bool warm_start = false`

`false` (the default): `learn` always starts from a fresh, untrained state,
even if the estimator already held a previous fit — the previous state is
discarded first. `true`: reuse the estimator's current parameters as the
solver's starting point instead of reinitializing, so a second `learn` call
refines rather than restarts. Only meaningful for iterative families; the
receiving estimator must already be trained for there to be anything to warm
-start from.

### `track_diagnostics`

`bool track_diagnostics = true`

Whether the solver records per-iteration diagnostics (loss trajectory,
convergence flags) that some estimators expose after fitting. Turning it off
saves a small amount of bookkeeping; it never changes the fitted parameters.

### `random`

`RandomState random{}`

The seed and determinism pair for every stochastic part of the fit — shuffling
before an iterative solve, k-means' restarts, the isolation forest's sampling.
See [`RandomState`](RandomState.md).

### `sample_weights`

`Series sample_weights = Series::undefined()`

Optional per-row weights, `[n_samples]`, applied during the fit. Undefined
(the default) means every row counts equally. When both this field and a
`DataFrameBatchOptions::sample_weight_feature` name a weight source, the
resolved weight is the two multiplied together — see
[`balance::resolve_training_weights`](../balance/resolve_weight_plan.md).

### `class_weight`

`ClassWeightMode class_weight = ClassWeightMode::None`

Classification only. `None` (the default — every row weighted equally by
class), `Balanced` (each class weighted by the inverse of its frequency, so a
rare class contributes as much total loss as a common one), `Custom` (read the
explicit per-class weights from `class_weights` below).

### `class_weights`

`Series class_weights = Series::undefined()`

Consulted only when `class_weight == Custom`. For a binary classifier, a
2-element `[negative_class_weight, positive_class_weight]` vector — a
multiclass one-vs-rest family applies this same pair to each one-vs-rest leg.

### `calibration`

`ScoreCalibration calibration = ScoreCalibration::None`

Post-fit probability calibration: `None` (the default — raw scores), `Platt`
(a logistic fit on the scores), `Isotonic` (a monotonic non-parametric fit),
`Temperature` (a single scalar rescale). Calibration changes only how scores
are mapped to probabilities for `infer`/`infer_scores` — it never changes
which class a hard-label `infer` picks under the default threshold, and it
never changes the underlying decision boundary.

### `calibration_iterations`

`std::int64_t calibration_iterations = 400`

Iteration cap for the calibrator's own solver. Ignored when
`calibration == None`.

### `calibration_tolerance`

`double calibration_tolerance = 1e-6`

Convergence tolerance for the calibrator's own solver. Ignored when
`calibration == None`.

### `parameter_lower_bounds`

`Series parameter_lower_bounds = Series::undefined()`

### `parameter_upper_bounds`

`Series parameter_upper_bounds = Series::undefined()`

Optional per-coefficient bounds (the pair, above and below), consulted only by
bound-constrained gradient-based solvers (currently the linear classification
family). Leave both undefined for an unconstrained fit; set one or both to
clamp specific
coefficients — for example forcing every coefficient non-negative with an
all-zero lower bound and an undefined upper bound.

### `balance`

`BalanceTarget balance{}`

Row rebalancing applied *before* the estimator sees the data — resampling,
synthetic-neighbor growth, and/or cleaning of a classification target's class
mix. The default `BalanceTarget{}` (`kind = Keep`) leaves the data exactly as
given. See [`BalanceTarget`](../balance/BalanceTarget.md) in
[`balance`](../balance.md) for the full field set.

## Guidance

Most fits need none of these fields touched — `TrainOptions{}` is a reasonable
default for a first pass on clean, complete data. The fields fall into two
groups worth knowing about even when you leave them alone:

- **Data-shape guards** (`missing`, `unknown_categorical`) default to the
  strict `Reject` / `Error` choice on purpose, so a shape problem surfaces as
  a clear `Status::shape_mismatch` at fit time instead of silently training on
  corrupted or unexpectedly-imputed data.
- **Class imbalance has two independent levers**: `class_weight` /
  `class_weights` re-weight the loss function without touching the data;
  `balance` actually resamples the rows before fitting. They compose — you can
  balance the rows *and* weight the loss — but reach for one first
  (`class_weight = Balanced` is the cheaper, non-destructive starting point)
  before adding the other.

## Practical Examples

### Example 1

The defaults, and the two most commonly touched fields —
`max_iterations`/`tolerance` for solver control.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {8},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU)).reshape({8, 1});
  const cls::Series y = pluss::make_series(
      {2.1, 4.0, 6.1, 7.9, 10.2, 11.9, 14.1, 15.8}, {8},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  cls::TrainOptions defaults{};
  const cls::Status s1 = made.value()->learn(batch, defaults);
  std::cout << "default fit ok = " << std::boolalpha << s1.ok() << "\n";

  cls::TrainOptions tight{};
  tight.max_iterations = 5000;
  tight.tolerance = 1e-10;
  const cls::Status s2 = made.value()->learn(batch, tight);
  std::cout << "tight fit ok   = " << s2.ok() << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_TRAINOPTIONS_1
```

### Example 2

`class_weight = Balanced` on an imbalanced binary target — the minority class
stops being outvoted.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 negatives, 1 positive, and the one positive sits right at the boundary.
  cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 5.0}, {10}, f64).reshape({10, 1});
  cls::Series y = pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, {10}, f64);

  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge

  auto fit_and_predict = [&](cls::ClassWeightMode mode) {
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
        cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
    cls::TrainOptions opts{};
    opts.class_weight = mode;
    made.value()->learn(batch, opts);
    cls::StatusOr<cls::Series> pred = made.value()->infer(batch.features);
    return pred.value();
  };

  const cls::Series unweighted = fit_and_predict(cls::ClassWeightMode::None);
  const cls::Series balanced = fit_and_predict(cls::ClassWeightMode::Balanced);

  std::cout << "unweighted predictions = " << unweighted << "\n";
  std::cout << "balanced predictions   = " << balanced << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_TRAINOPTIONS_2
```

### Example 3

`warm_start` — a second `learn` call refines the existing fit instead of
starting over, versus the default that discards it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64).reshape({6, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0, 12.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::TrainOptions fresh{};
  fresh.max_iterations = 1;  // deliberately under-converged
  made.value()->learn(batch, fresh);
  const bool trained_after_one_pass = made.value()->trained();

  cls::TrainOptions warm{};
  warm.warm_start = true;
  warm.max_iterations = 1;
  made.value()->learn(batch, warm);  // continues from the under-converged fit

  std::cout << "trained after 1 iteration = " << std::boolalpha << trained_after_one_pass << "\n";
  std::cout << "still trained after warm-started pass = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_TRAINOPTIONS_3
```

## Changelog

- **0.1** — the 17-field per-fit configuration: shape guards, solver limits,
  RNG, weighting, calibration, coefficient bounds, row balancing.

## See Also

- [`InferOptions`](InferOptions.md) / [`AssessOptions`](AssessOptions.md) —
  the prediction-time and scoring-time counterparts.
- [`estimators`](../estimators.md) — every `learn` signature that takes this.
- [`balance`](../balance.md) — the full row-rebalancing machinery behind
  `.balance`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
