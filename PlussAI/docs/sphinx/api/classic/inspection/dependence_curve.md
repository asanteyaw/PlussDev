# `pluss::ai::classic::dependence_curve`

A one-feature partial-dependence curve: fits a model from `spec`, then
sweeps one feature across an evenly-spaced grid, holding every other
feature at its own real value in `eval`, and averages the model's
prediction across all of `eval`'s rows at each grid point — the classic
"set every row's feature `i` to `v`, re-predict, average" partial-
dependence method. Unlike [`permutation_impact`](permutation_impact.md),
nothing is shuffled and no metric is involved; this shows *how* a
prediction moves as one feature changes, not *whether* the feature
matters.

## Signature

```cpp
classic::StatusOr<classic::DependenceCurve> classic::dependence_curve(
    const classic::ModelSpec& spec,
    const classic::LabeledBatch& train,
    const classic::FeatureBatch& eval,
    std::int64_t feature_index,
    std::int64_t grid_points = 32);
```

## What It Does

Fits a model from `spec` on `train`, then for each of `grid_points`
evenly-spaced values of one feature, sets that feature to the grid value
across every row of `eval` (leaving every other feature at its own real
value), re-predicts, and averages the result -- tracing out how the
model's prediction moves as that one feature changes, independent of
everything else in the row.

## Parameters

`spec`
: A fresh classifier or regressor is built from `spec` and fit on `train`
  internally — there is no overload accepting an already-fitted model.
  **Classification requires a model whose `infer_scores` produces exactly
  one score per row** — a binary classifier. A multiclass classifier is
  rejected outright rather than silently averaging across classes; see
  [Example 5](#example-5).

`train`
: Must pass [`validate_labeled`](../data/LabeledBatch.md); the model is
  fit on this once.

`eval`
: Must pass [`validate_features`](../data/FeatureBatch.md); every row here
  provides the "held fixed" values for every feature except
  `feature_index`, and every grid point's mean prediction averages across
  all of these rows.

`feature_index`
: Which column of `eval` to sweep. Must be in `[0, eval.feature_count())`.

`grid_points` (`std::int64_t`, default `32`)
: How many evenly-spaced points to sweep between that feature's own
  observed minimum and maximum in `eval` (inclusive of both endpoints).
  Must be `> 1`. If the feature is constant across every row in `eval`
  (`min == max`), every grid point is that same constant value instead of
  attempting a zero-length step — see [Example 4](#example-4).

## Returns

A `StatusOr<DependenceCurve>`:

```cpp
struct classic::DependenceCurve {
  std::int64_t feature_index;   // echoes the argument back
  pluss::Series grid;           // Float64, length grid_points
  pluss::Series mean_prediction; // Float64, length grid_points
};
```

For a classifier, `mean_prediction[i]` is the mean **predicted
probability** (`sigmoid(infer_scores(...))`, averaged across `eval`'s
rows) at `grid[i]` — always in `[0, 1]`, regardless of what `eval.y`'s own
label values are. For a regressor, `mean_prediction[i]` is the mean raw
prediction (`infer(...)`, averaged across `eval`'s rows), in the target's
own units.

## Errors

- `grid_points <= 1` — `Status::invalid_argument`.
- `train` fails [`validate_labeled`](../data/LabeledBatch.md), or `eval`
  fails [`validate_features`](../data/FeatureBatch.md) — whatever that
  validation returns.
- `feature_index` outside `[0, eval.feature_count())` —
  `Status::shape_mismatch`.
- Every value of the swept feature in `eval` is non-finite (so no finite
  `min`/`max` exists) — `Status::numerical_failure`.
- `spec` fails to build a model, or `model.learn(train)` fails — whatever
  that call returns.
- A classification `spec` whose `infer_scores` produces more than one
  score per row (a multiclass classifier) —
  `Status::invalid_argument`.

## Practical Examples

A classification curve showing the predicted-probability transition
across a decision boundary, a regression curve in the target's own units,
`grid_points` controlling resolution, the constant-feature edge case, and
the multiclass rejection.

### Example 1

A clean linear decision boundary: as `feature_index = 0` sweeps from low
to high, the mean predicted probability rises from near `0` to near `1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 0.1, 1.1, 0.2, 0.9, 10.0, 2.0, 10.1, 2.1, 10.2, 1.9},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceCurve curve = cls::dependence_curve(spec, batch, batch.features, 0, 5).value();
  std::cout << "grid = " << curve.grid << "\n";
  std::cout << "mean_prediction = " << curve.mean_prediction << "\n";

  return 0;
}
```

Output

```text
grid = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 2.55, 5.1, 7.65, 10.2]
mean_prediction = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[8.839801e-07, 0.0009441432, 0.5000798, 0.99905646, 0.99999912]
```

### Example 2

A regression curve — `mean_prediction` is in the target's own units, not
`[0, 1]`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 2.0, 1.0, 3.0, 9.0, 4.0, 2.0, 5.0, 8.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::LeastSquares;
  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts};

  const cls::DependenceCurve curve = cls::dependence_curve(spec, batch, batch.features, 0, 4).value();
  std::cout << "grid = " << curve.grid << "\n";
  std::cout << "mean_prediction = " << curve.mean_prediction << "\n";

  return 0;
}
```

Output

```text
grid = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2.3333333, 3.6666667, 5]
mean_prediction = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0000004, 4.6666668, 7.3333332, 9.9999996]
```

### Example 3

`grid_points` controls resolution alone — `grid`'s own endpoints (the
feature's real observed min/max) stay the same regardless.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 0.1, 1.1, 0.2, 0.9, 10.0, 2.0, 10.1, 2.1, 10.2, 1.9},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceCurve coarse = cls::dependence_curve(spec, batch, batch.features, 0, 3).value();
  const cls::DependenceCurve fine = cls::dependence_curve(spec, batch, batch.features, 0, 9).value();
  std::cout << "coarse grid (3 points)  = " << coarse.grid << "\n";
  std::cout << "fine grid   (9 points)  = " << fine.grid << "\n";
  std::cout << "both endpoints match (min=0.0, max=10.2) regardless of grid_points\n";

  return 0;
}
```

Output

```text
coarse grid (3 points)  = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 5.1, 10.2]
fine grid   (9 points)  = pluss::Series(shape=[9], strides=[1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[0, 1.275, 2.55, 3.825, 5.1, 6.375, 7.65, 8.925, 10.2]
both endpoints match (min=0.0, max=10.2) regardless of grid_points
```

### Example 4

A feature that is exactly constant across every row in `eval`: every grid
point is that same constant, not an attempted (and undefined)
zero-length step.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 1 is exactly 7.0 for every row.
  const cls::Series x = pluss::make_series(
      {0.0, 7.0, 0.1, 7.0, 10.0, 7.0, 10.1, 7.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceCurve curve = cls::dependence_curve(spec, batch, batch.features, 1, 4).value();
  std::cout << "grid (constant feature, every point = 7.0) = " << curve.grid << "\n";

  return 0;
}
```

Output

```text
grid (constant feature, every point = 7.0) = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[7, 7, 7, 7]
```

### Example 5

A multiclass classifier — three classes, `infer_scores` produces one
score per class per row — is rejected outright rather than silently
averaging across both rows and classes into one meaningless number.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 0.1, 0.1, 5.0, 5.0, 5.1, 5.1, 10.0, 10.0, 10.1, 10.1},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::StatusOr<cls::DependenceCurve> rejected = cls::dependence_curve(spec, batch, batch.features, 0, 5);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "dependence_curve requires a classifier that produces exactly one score per row (binary classification); this ModelSpec's infer_scores produced multiple columns per row, which has no single well-defined mean prediction."
```

### Example 6

An error roundup: `grid_points <= 1` and an out-of-range `feature_index`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({0.0, 1.0, 10.0, 2.0}, {4}, f64).reshape({2, 2});
  const cls::Series y = pluss::make_series({0.0, 1.0}, {2}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  std::cout << "grid_points=1 ok = " << std::boolalpha << cls::dependence_curve(spec, batch, batch.features, 0, 1).ok() << "\n";
  std::cout << "feature_index out of range ok = " << cls::dependence_curve(spec, batch, batch.features, 5, 4).ok() << "\n";

  return 0;
}
```

Output

```text
grid_points=1 ok = false
feature_index out of range ok = false
```

## Changelog

- **0.1** — `dependence_curve`, `DependenceCurve`.
- **0.2** — Fixed a crash (`Series::view` uncaught exception) whenever
  `spec` built a classifier whose `infer_scores` produced more than one
  score per row (any multiclass classifier) — now rejected with a graceful
  `Status::invalid_argument` naming the cause, since averaging a
  multi-class score matrix into one number has no single well-defined
  meaning.

## See Also

- [`dependence_surface`](dependence_surface.md) — the two-feature
  generalization of this same method.
- [`permutation_impact`](permutation_impact.md) — measures whether a
  feature matters; this shows how a prediction moves as it changes.
- [`ModelSpec`](../families/ModelSpec.md) — how `spec` builds the fitted
  model.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
