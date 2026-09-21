# `pluss::ai::classic::LinearFamilyOptions`

`LinearFamilyOptions` configures every linear estimator: `objective` picks
which linear model this actually is (least-squares, logistic, hinge/SVM-style,
or a robust regression), `regularization` and `regularization_strength`
control coefficient shrinkage, and `fit_intercept` decides whether a bias term
is fit.

## Fields

### `objective`

`LinearObjective objective = LinearObjective::LeastSquares`

Which linear model this is: `LeastSquares` (ordinary/ridge/lasso-style linear
regression, depending on `regularization` — the default, and only valid for
`TaskKind::Regression`), `Logistic` (logistic regression), `Hinge` (a linear
SVM-style classifier), `RobustLeast` (Huber-style robust regression, less
sensitive to outliers than `LeastSquares`; regression only).

**A `TaskKind::Classification` spec requires `objective = Logistic` or
`Hinge`** — a linear classifier built with the default `LeastSquares` (or
`RobustLeast`) constructs successfully (`make_classifier` does not check
`objective`) but fails at `learn(...)` with `"Linear classifier currently
supports LinearObjective::Logistic or LinearObjective::Hinge."` Set `objective`
explicitly on every `ModelSpec{TaskKind::Classification, ModelFamily::Linear,
...}` — the default is not a safe choice for classification the way it is for
the other eight families.

### `regularization`

`RegularizationKind regularization = RegularizationKind::L2`

The penalty added to the loss: `None` (no shrinkage — ordinary least
squares/logistic), `L1` (lasso-style — drives some coefficients exactly to
zero, useful for feature selection), `L2` (ridge-style — shrinks all
coefficients smoothly, the default), `Elastic` (a blend of both, weighted by
`elastic_l1_ratio`).

### `regularization_strength`

`double regularization_strength = 1e-6`

The penalty weight. `>= 0`; `0` is equivalent to `regularization = None`
regardless of the enum value. Larger values shrink coefficients harder —
toward zero for `L2`, toward exact zero (more of them) for `L1`. The default
`1e-6` is deliberately tiny — close enough to unregularized that it mainly
guards against numerical degeneracy rather than meaningfully shrinking
anything.

### `elastic_l1_ratio`

`double elastic_l1_ratio = 0.5`

Only consulted when `regularization == Elastic`. `1.0` = pure L1, `0.0` =
pure L2, values between blend the two penalties.

### `robust_transition`

`double robust_transition = 1.35`

Only consulted when `objective == RobustLeast`. The Huber-style transition
point: residuals with `|r| <= robust_transition` use the ordinary quadratic
loss, residuals beyond it use a linear loss instead — the mechanism that
makes `RobustLeast` less sensitive to a few large outliers than
`LeastSquares`. Smaller values treat more residuals as outliers (down-weight
them sooner); larger values approach plain `LeastSquares` behavior.

### `fit_intercept`

`bool fit_intercept = true`

Whether to fit a bias/intercept term alongside the feature coefficients.
`true` (the default) almost always the right choice — set `false` only when
you know the data is already centered and a bias term would be redundant or
actively wrong (some encoded-feature setups guarantee this).

## Guidance

`regularization_strength` is the field worth sweeping first when tuning a
linear model — see [`tune`](../tune.md) for ranking several values against
each other via cross-validation rather than guessing. Reach for
`regularization = L1` specifically when you want automatic feature
selection (many coefficients driven to exactly zero); reach for `Elastic`
when you want some of that sparsity without L1's instability under correlated
features.

## Practical Examples

### Example 1

`regularization_strength` shrinking a fitted linear regression's prediction
toward the target mean — a large value pulls hard, a tiny one barely moves
it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});
  const cls::Series y = pluss::make_series(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}, {10}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});

  auto fit_and_predict = [&](double strength) {
    cls::LinearFamilyOptions opts{};
    opts.regularization = cls::RegularizationKind::L2;
    opts.regularization_strength = strength;
    cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
        cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts});
    made.value()->learn(batch);
    return made.value()->infer(probe).value().item<double>();
  };

  std::cout << "true value at x = 10       : 100\n";
  std::cout << "strength = 1e-6 prediction : " << fit_and_predict(1e-6) << "\n";
  std::cout << "strength = 1e3  prediction : " << fit_and_predict(1e3) << "\n";

  return 0;
}
```

Output

```text
true value at x = 10       : 100
strength = 1e-6 prediction : 100
strength = 1e3  prediction : 58.4296
```

### Example 2

`objective = RobustLeast` versus `LeastSquares` on data with one large
outlier — the robust fit is pulled far less.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // y = 2x, except the last point is a wild outlier.
  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64).reshape({6, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0, 40.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({7.0}, {1}, f64).reshape({1, 1});

  auto fit_and_predict = [&](cls::LinearObjective objective) {
    cls::LinearFamilyOptions opts{};
    opts.objective = objective;
    opts.regularization_strength = 1e-6;
    cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
        cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts});
    made.value()->learn(batch);
    return made.value()->infer(probe).value().item<double>();
  };

  std::cout << "true trend at x = 7 (ignoring the outlier): 14\n";
  std::cout << "LeastSquares prediction : " << fit_and_predict(cls::LinearObjective::LeastSquares) << "\n";
  std::cout << "RobustLeast prediction  : " << fit_and_predict(cls::LinearObjective::RobustLeast) << "\n";

  return 0;
}
```

Output

```text
true trend at x = 7 (ignoring the outlier): 14
LeastSquares prediction : 32.6667
RobustLeast prediction  : 15.8653
```

## Changelog

- **0.1** — `objective`, `regularization` (+ `regularization_strength` /
  `elastic_l1_ratio`), `robust_transition`, `fit_intercept`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.
- [`estimators`](../estimators.md) — `make_classifier` / `make_regressor`
  consuming a `Linear`-family spec.
- [`online`](../online.md) — `OnlineLinearOptions` wraps this same struct for
  the incremental learners.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
