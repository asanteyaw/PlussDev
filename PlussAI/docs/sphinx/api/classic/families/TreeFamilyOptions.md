# `pluss::ai::classic::TreeFamilyOptions`

`TreeFamilyOptions` configures a single decision tree: `criterion` picks the
split-quality measure, and the three remaining fields cap how large the tree
is allowed to grow.

## Fields

### `criterion`

`TreeCriterion criterion = TreeCriterion::Gini`

The split-quality measure: `Gini` (Gini impurity, classification), `Entropy`
(information gain, classification), `Mse` (mean squared error, regression),
`Mae` (mean absolute error, regression — more robust to outlier targets than
`Mse`, at higher computational cost).

**The task/criterion pairing is required, and — like
[`LinearFamilyOptions::objective`](LinearFamilyOptions.md#objective) —
checked at `learn(...)` rather than at construction:** `make_classifier`
accepts any `criterion` value, but a classifier's first `learn` call rejects
`Mse`/`Mae` (`"Tree classifier currently supports TreeCriterion::Gini or
TreeCriterion::Entropy only."`), and symmetrically a regressor's `learn`
rejects `Gini`/`Entropy` (`"Tree regressor currently supports
TreeCriterion::Mse or TreeCriterion::Mae."`). Set `criterion` to match
`.task` explicitly.

### `max_depth`

`std::int64_t max_depth = -1`

The deepest a tree may grow. `-1` (the default) means unbounded — the tree
grows until every leaf is pure or hits `min_samples_leaf` /
`min_samples_split`. A small positive value (a "stump" at `1`, a shallow tree
at `2`-`4`) trades fitting power for resistance to overfitting and a much
more interpretable tree.

### `min_samples_leaf`

`std::int64_t min_samples_leaf = 1`

The minimum number of samples a leaf must contain; a candidate split that
would leave a leaf with fewer samples than this is rejected. `>= 1`. Raising
it smooths the tree's decision boundary and reduces overfitting to individual
noisy points, at the cost of some fitting power.

### `min_samples_split`

`std::int64_t min_samples_split = 2`

The minimum number of samples a node must hold to be considered for
splitting at all. `>= 2`. Distinct from `min_samples_leaf` — this gates
*whether a node is a split candidate*, not the size of the resulting leaves.

## Guidance

`max_depth` is the single most effective lever against overfitting for a
single tree — an unbounded tree (`-1`) on real data will typically memorize
training noise. `min_samples_leaf` and `min_samples_split` are the finer-
grained versions of the same idea, useful when you want to cap tree
complexity without committing to a fixed depth.

## Practical Example

`max_depth` controlling whether a tree can separate a pattern that needs more
than one split.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // A checkerboard: no single split separates the classes.
  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 0.1, 0.1, 0.9, 0.9,
       0.0, 1.0, 0.0, 1.0, 0.1, 0.9, 0.1, 0.9},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series({0, 1, 1, 0, 0, 1, 1, 0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](std::int64_t depth) {
    cls::TreeFamilyOptions opts{};
    opts.max_depth = depth;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Tree, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "max_depth = 1 train accuracy : " << fit_and_score(1) << "\n";
  std::cout << "max_depth = -1 train accuracy: " << fit_and_score(-1) << "\n";

  return 0;
}
```

Output

```text
max_depth = 1 train accuracy : 0.625
max_depth = -1 train accuracy: 0.75
```

## Changelog

- **0.1** — `criterion`, `max_depth`, `min_samples_leaf`, `min_samples_split`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.
- [`EnsembleFamilyOptions`](EnsembleFamilyOptions.md) — forests and boosted
  ensembles of trees built from this same shape.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
