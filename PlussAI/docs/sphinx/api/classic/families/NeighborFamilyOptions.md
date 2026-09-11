# `pluss::ai::classic::NeighborFamilyOptions`

`NeighborFamilyOptions` configures a k-nearest-neighbors estimator: how many
neighbors to consult (or what radius to search), how each neighbor's vote is
weighted, and whether features are standardized before distance is computed.

## Fields

### `k`

`std::int64_t k = 5`

The neighbor count for `query_mode == KNearest`. `> 0`. Small values follow
the training data closely (higher variance, more sensitive to noisy
neighbors); large values smooth the decision more (higher bias, less
sensitive to any one point).

### `query_mode`

`NeighborQueryMode query_mode = NeighborQueryMode::KNearest`

`KNearest` (the default): always use exactly `k` neighbors. `Radius`: use
every point within `radius` of the query, whatever that count happens to be —
appropriate when the right neighbor count varies by region of feature space
rather than being fixed.

### `radius`

`double radius = 1.0`

Only consulted for `query_mode == Radius`. The search radius in feature
space. `> 0`. Since this is a raw distance threshold, it is scale-sensitive —
pair it with `standardize = true` (the default) unless your features are
already on comparable scales.

### `radius_fallback_k`

`std::int64_t radius_fallback_k = 1`

Only consulted for `query_mode == Radius`. When a radius query finds zero
neighbors for some point, fall back to its nearest `radius_fallback_k`
neighbors instead of leaving that prediction undefined.

### `distance_weighted`

`bool distance_weighted = false`

`false` (the default): every neighbor's vote counts equally. `true`: closer
neighbors count more (inverse-distance weighting) — usually a strict
improvement in accuracy at negligible extra cost, but it changes predictions
enough that it is not the default.

### `standardize`

`bool standardize = true`

`true` (the default): features are standardized (zero mean, unit variance)
before distance is computed, so no single large-scale feature dominates the
distance metric. `false`: raw feature values are used directly — only
appropriate when you have already scaled the features yourself or specifically
want the raw-scale distance.

## Guidance

`standardize = true` should stay on for almost every real dataset — nearest-
neighbor distance is scale-sensitive by construction, and an unscaled feature
with a large numeric range will silently dominate every distance computation.
Turn `distance_weighted` on as a low-cost accuracy improvement once the basic
`k` is chosen.

## Practical Example

`standardize` mattering when one feature's scale dwarfs another's.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Feature 0 is on a 0-1 scale and actually separates the classes;
  // feature 1 is on a 0-1000 scale and is pure noise. Unscaled, feature 1
  // dominates every distance.
  const cls::Series x = pluss::make_series(
      {0.1, 900.0, 0.15, 100.0, 0.85, 500.0, 0.9, 300.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](bool standardize) {
    cls::NeighborFamilyOptions opts{};
    opts.k = 1;
    opts.standardize = standardize;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Neighbor, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "standardize = false train accuracy: " << fit_and_score(false) << "\n";
  std::cout << "standardize = true  train accuracy: " << fit_and_score(true) << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_NEIGHBORFAMILYOPTIONS_1
```

## Changelog

- **0.1** — `k`, `query_mode` (+ `radius` / `radius_fallback_k`),
  `distance_weighted`, `standardize`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.
- [`neighbors`](../neighbors.md) — `SpatialIndex`, the lower-level exact
  k-nearest-neighbor structure this family's kernel is built around.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
