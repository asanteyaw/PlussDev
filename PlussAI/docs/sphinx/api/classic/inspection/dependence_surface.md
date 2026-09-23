# `pluss::ai::classic::dependence_surface`

The two-feature generalization of [`dependence_curve`](dependence_curve.md):
fits a model from `spec`, then sweeps **two** features jointly across
independent grids, holding every other feature at its own real value in
`eval`, and averages the model's prediction across all of `eval`'s rows at
every `(grid_a[i], grid_b[j])` pair — the same "set-and-average"
partial-dependence method, over a 2-D grid instead of a 1-D one.

## Signature

```cpp
classic::StatusOr<classic::DependenceSurface> classic::dependence_surface(
    const classic::ModelSpec& spec,
    const classic::LabeledBatch& train,
    const classic::FeatureBatch& eval,
    std::int64_t feature_a,
    std::int64_t feature_b,
    std::int64_t grid_points_a = 16,
    std::int64_t grid_points_b = 16);
```

## Parameters

`spec` / `train` / `eval`
: Identical contract to
  [`dependence_curve`'s own parameters](dependence_curve.md#parameters),
  including the same rejection of a classification `spec` whose
  `infer_scores` produces more than one score per row (a multiclass
  classifier) — see [Example 4](#example-4).

`feature_a` / `feature_b`
: Which two columns of `eval` to sweep jointly. Both must be in `[0,
  eval.feature_count())`, and **must be distinct** — `feature_a ==
  feature_b` is rejected outright, unlike `dependence_curve`, which only
  ever sweeps one feature and has no such possible conflict.

`grid_points_a` / `grid_points_b` (`std::int64_t`, default `16` each)
: How many evenly-spaced points to sweep for `feature_a` and `feature_b`
  respectively — independently sized, need not match. Each must be `>
  1`. Same min/max-derived, constant-feature-safe grid construction as
  [`dependence_curve`'s own `grid_points`](dependence_curve.md#parameters).

## Returns

A `StatusOr<DependenceSurface>`:

```cpp
struct classic::DependenceSurface {
  std::int64_t feature_a;         // echoes the argument back
  std::int64_t feature_b;         // echoes the argument back
  pluss::Series grid_a;           // Float64, length grid_points_a
  pluss::Series grid_b;           // Float64, length grid_points_b
  pluss::Series mean_prediction;  // Float64, [grid_points_a, grid_points_b]
};
```

`mean_prediction[i, j]` is the mean prediction with `feature_a` held at
`grid_a[i]` and `feature_b` held at `grid_b[j]` across every other row's
own value elsewhere — same classification-probability-in-`[0,1]` vs.
regression-raw-units distinction as
[`dependence_curve`'s own `mean_prediction`](dependence_curve.md#returns).

## Errors

- `grid_points_a <= 1` or `grid_points_b <= 1` — `Status::invalid_argument`.
- `feature_a == feature_b` — `Status::invalid_argument`.
- `train` fails [`validate_labeled`](../data/LabeledBatch.md), or `eval`
  fails [`validate_features`](../data/FeatureBatch.md) — whatever that
  validation returns.
- `feature_a` or `feature_b` outside `[0, eval.feature_count())` —
  `Status::shape_mismatch`.
- `spec` fails to build a model, or `model.learn(train)` fails — whatever
  that call returns.
- A classification `spec` whose `infer_scores` produces more than one
  score per row (a multiclass classifier) — `Status::invalid_argument`.

## Practical Examples

A classification surface over two features, a regression surface,
independently-sized grids, the multiclass rejection, and an error roundup
covering both grid-size and distinct-feature-index requirements.

### Example 1

Two features swept jointly — the surface rises toward the class-`1`
corner.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 0.1, 0.1, 0.0, 0.2, 10.0, 10.0, 10.1, 10.1, 10.0, 9.9},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceSurface surface = cls::dependence_surface(spec, batch, batch.features, 0, 1, 3, 3).value();
  std::cout << "grid_a = " << surface.grid_a << "\n";
  std::cout << "grid_b = " << surface.grid_b << "\n";
  std::cout << "mean_prediction = " << surface.mean_prediction << "\n";

  return 0;
}
```

Output

```text
grid_a = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 5.05, 10.1]
grid_b = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 5.05, 10.1]
mean_prediction = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[6.9753195e-07, 0.00081120784, 0.48584563],
  [0.00088167913, 0.50668505, 0.9991642],
  [0.52750127, 0.99923101, 0.99999934]]
```

### Example 2

A regression surface — `mean_prediction` in the target's own units.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 4.0, 4.0, 5.0, 5.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::LeastSquares;
  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts};

  const cls::DependenceSurface surface = cls::dependence_surface(spec, batch, batch.features, 0, 1, 3, 2).value();
  std::cout << "grid_a (3 points) = " << surface.grid_a << "\n";
  std::cout << "grid_b (2 points) = " << surface.grid_b << "\n";
  std::cout << "mean_prediction shape = [" << surface.mean_prediction.sizes()[0] << ", " << surface.mean_prediction.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
grid_a (3 points) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 3, 5]
grid_b (2 points) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 5]
mean_prediction shape = [3, 2]
```

### Example 3

`grid_points_a` and `grid_points_b` are independently sized — here `4`
and `2`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 0.1, 0.1, 0.0, 0.2, 10.0, 10.0, 10.1, 10.1, 10.0, 9.9},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceSurface surface = cls::dependence_surface(spec, batch, batch.features, 0, 1, 4, 2).value();
  std::cout << "mean_prediction shape = [" << surface.mean_prediction.sizes()[0] << ", " << surface.mean_prediction.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
mean_prediction shape = [4, 2]
```

### Example 4

A multiclass classifier is rejected outright — the same rejection as
[`dependence_curve`'s own Example 5](dependence_curve.md#example-5),
here for the two-feature surface.

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

  const cls::StatusOr<cls::DependenceSurface> rejected = cls::dependence_surface(spec, batch, batch.features, 0, 1, 3, 3);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "dependence_surface requires a classifier that produces exactly one score per row (binary classification); this ModelSpec's infer_scores produced multiple columns per row, which has no single well-defined mean prediction."
```

### Example 5

An error roundup: a grid size `<= 1`, and `feature_a == feature_b`.

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

  std::cout << "grid_points_b=1 ok = " << std::boolalpha << cls::dependence_surface(spec, batch, batch.features, 0, 1, 3, 1).ok() << "\n";
  std::cout << "feature_a == feature_b ok = " << cls::dependence_surface(spec, batch, batch.features, 0, 0, 3, 3).ok() << "\n";

  return 0;
}
```

Output

```text
grid_points_b=1 ok = false
feature_a == feature_b ok = false
```

## Changelog

- **0.1** — `dependence_surface`, `DependenceSurface`.
- **0.2** — Fixed a silent-wrong-answer bug: a classification `spec`
  whose `infer_scores` produced more than one score per row (any
  multiclass classifier) previously averaged the sigmoid of **every**
  class's score across **every** row into one number, with no error and
  no warning. Now rejected with a graceful `Status::invalid_argument`
  naming the cause — the same fix applied to
  [`dependence_curve`](dependence_curve.md), which had the same gap but
  crashed outright instead of returning a wrong number.

## See Also

- [`dependence_curve`](dependence_curve.md) — the one-feature version of
  this same method.
- [`permutation_impact`](permutation_impact.md) — measures whether a
  feature matters; this shows how a prediction moves as two features
  change jointly.
- [`ModelSpec`](../families/ModelSpec.md) — how `spec` builds the fitted
  model.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
