# `pluss::ai::classic::EnsembleFamilyOptions`

`EnsembleFamilyOptions` configures an ensemble of trees: `method` picks the
combination strategy (bagging, random forest, gradient boosting, AdaBoost),
and the remaining three fields tune how many base learners are combined and
how aggressively boosting proceeds.

## Fields

### `method`

`EnsembleMethod method = EnsembleMethod::RandomForest`

The ensemble strategy: `Bagging` (bootstrap-aggregated trees, each on a
resampled subset of rows), `RandomForest` (bagging plus per-split feature
subsampling — the default and the usual first choice), `GradientBoost`
(sequential trees, each fit to the previous ensemble's residual/gradient),
`AdaBoost` (sequential trees, each up-weighting the previous ensemble's
mistakes).

### `learners`

`std::int64_t learners = 100`

The number of base trees in the ensemble. `> 0`. More learners generally
improve `Bagging` / `RandomForest` accuracy with diminishing returns (and no
overfitting risk from the count alone); for `GradientBoost` / `AdaBoost`, too
many learners *can* overfit, especially paired with a high `learning_rate`.

### `subsample`

`double subsample = 1.0`

The row fraction each base learner trains on. `0 < subsample <= 1`. `1.0`
(the default) uses every row for every learner under `RandomForest` /
`GradientBoost`'s own internal resampling; lowering it adds an extra layer of
randomness (stochastic gradient boosting-style) that can reduce overfitting
at the cost of more variance per learner.

### `learning_rate`

`double learning_rate = 0.1`

Only meaningful for the boosting methods (`GradientBoost`, `AdaBoost`) —
shrinks each successive learner's contribution to the ensemble. `> 0`. Smaller
values need more `learners` to reach the same fit but generalize better;
larger values fit faster (fewer learners needed) but overfit more easily.
Ignored by `Bagging` / `RandomForest`, which do not have a sequential
correction step to shrink.

## Guidance

Start with `RandomForest` and its defaults — it is the most forgiving method
to mis-tune. Move to `GradientBoost` when you need the extra accuracy boosting
often provides and are willing to tune `learners` and `learning_rate`
together (they trade off directly: halving `learning_rate` roughly doubles
the `learners` needed for a comparable fit).

## Practical Example

`learners` and `method` together — a random forest's accuracy improving with
more trees, on a pattern a single tree under-fits.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 0.1, 0.1, 0.9, 0.9,
       0.0, 1.0, 0.0, 1.0, 0.1, 0.9, 0.1, 0.9},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series({0, 1, 1, 0, 0, 1, 1, 0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](std::int64_t learners) {
    cls::EnsembleFamilyOptions opts{};
    opts.method = cls::EnsembleMethod::RandomForest;
    opts.learners = learners;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Ensemble, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "learners = 1   train accuracy: " << fit_and_score(1) << "\n";
  std::cout << "learners = 100 train accuracy: " << fit_and_score(100) << "\n";

  return 0;
}
```

Output

```text
learners = 1   train accuracy: 0.625
learners = 100 train accuracy: 0.75
```

## Changelog

- **0.1** — `method`, `learners`, `subsample`, `learning_rate`.

## See Also

- [`TreeFamilyOptions`](TreeFamilyOptions.md) — the single-tree base this
  ensembles.
- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
