# `pluss::ai::classic::ClusterFamilyOptions`

`ClusterFamilyOptions` configures a clustering estimator — currently k-means:
how many clusters to find, how long the solver may iterate, and how many
random restarts guard against a bad initialization.

## Fields

### `method`

`ClusterMethod method = ClusterMethod::KMeans`

The clustering algorithm. `KMeans` is the only value today.

### `clusters`

`std::int64_t clusters = 8`

The number of clusters to find (k-means' "k"). `> 0`. Chosen by you, not
learned from the data — there is no automatic cluster-count selection here.

### `max_iterations`

`std::int64_t max_iterations = 300`

Iteration cap for the k-means update loop, per restart. `> 0`. Raising it lets
a slow-converging restart finish; it never changes the answer once converged.

### `restarts`

`std::int64_t restarts = 4`

The number of independent random initializations to try; the restart with the
best inertia (tightest clusters) is kept. `> 0`. K-means' result depends on
its starting centroids, so more restarts reduce the chance of settling on a
poor local optimum, at a proportional cost in fitting time.

### `tolerance`

`double tolerance = 1e-4`

Convergence threshold — a restart stops early once successive iterations
move the centroids by less than this. `> 0`.

## Guidance

`restarts` is the field most worth raising if clustering results seem
unstable between runs (different `RandomState` seeds landing on visibly
different clusterings) — that instability is a symptom of a bad local
optimum, and more restarts is the direct fix.

## Practical Example

`clusters` shaping the output — the transformer maps each point onto its
assigned cluster.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Two obvious clusters, near (0, 0) and near (10, 10).
  const cls::Series x = pluss::make_series(
      {0.0, 0.1, 0.2, 0.1, 10.0, 10.1, 9.9, 10.2}, {8}, f64).reshape({4, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  cls::ClusterFamilyOptions opts{};
  opts.clusters = 2;
  opts.restarts = 4;
  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(
      cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  made.value()->learn(batch);
  const cls::StatusOr<pluss::Series> assignments = made.value()->map(batch);

  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "cluster assignments = " << assignments.value() << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_CLUSTERFAMILYOPTIONS_1
```

## Changelog

- **0.1** — `method`, `clusters`, `max_iterations`, `restarts`, `tolerance`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative; note the `Transform` task, not `Classification`/`Regression`.
- [`estimators`](../estimators.md) — `make_transformer`, `TransformerBase`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
