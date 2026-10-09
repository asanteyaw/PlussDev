# `lymph::selection::search::Search`

`Search` is the **hyperparameter-search driver** of the
[`selection::search`](../search.md) subfamily. It holds a search space (a list of
[`GridAxis`](grid.md)) and a split policy as a value; its one method,
[`run`](Search/run.md), visits the space, cross-validates a model built at each
point, and ranks the trials.

The improvement over a bare spec sweep: `run` takes a `make_model` closure that
builds a **full `models::Model`** — prep stages included — from each grid point,
so every trial cross-validates a genuine end-to-end pipeline, not an unscaled
estimator core.

## Scope And Alias

The type is `lymph::selection::search::Search`, re-exported one scope up as
`lymph::selection::Search`. Both names are the same type.

Run a grid search over one regularization axis with two-fold cross-validation, keeping both fitted candidates. The program prints whether the best model trained and how many top models came back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace lymph = pluss::ai::lymph;
using pluss::table::DataFrame;
using pluss::table::Slate;

lymph::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const lymph::LabeledBatch batch = fixture_batch(cfg);
  const std::vector<lymph::selection::GridAxis> axes = {
      lymph::selection::GridAxis{.name = "regularization", .values = {0.0, 1.0}},
  };
  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  lymph::selection::Search search = lymph::selection::grid(axes, cv);
  auto make = [](const lymph::selection::GridPoint& point) {
    lymph::LinearFamilyOptions options{};
    options.regularization_strength = point.at("regularization");
    return lymph::models::logistic_regression(options);
  };
  lymph::selection::FitOptions fit{};
  fit.fit_top = 2;
  const auto result = search.run(make, batch, {}, {}, lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Auto, fit);
  std::cout << "best trained = " << (result.best().trained() ? "true" : "false") << "\n";
  std::cout << "top models = " << result.top(2).size() << "\n";
  return 0;
}
```

Output

```text
best trained = true
top models = 2
```

## Construction

Built by a factory, not by hand — its constructor is private. All three live in
`lymph::selection::search` and re-export to `lymph::selection`:

`grid(std::vector<GridAxis> axes, SplitOptions | CrossValOptions)`
: Exhaustive — one trial per point in the cross product of the axes.

`random(std::vector<GridAxis> axes, std::int64_t trials, …)`
: Samples `trials` points from the space. Overloads take a `RandomState` +
  `SplitOptions`, or a `CrossValOptions` (which also seeds the sampler), or
  both.

`build(std::vector<GridAxis> axes, TuneOptions options)`
: From an assembled `TuneOptions` — search kind, trial count, seed, split,
  worker count in one struct.

[`grid` / `random` / `build`](grid.md) has the full signatures and worked
examples.

## Methods

| Method | Summary |
|--------|---------|
| [`run`](Search/run.md) | Call `make_model` at each grid point, cross-validate that full model, rank the trials → [`search::Result`](Result.md). Many data-shape overloads, each with an optional `SplitOptions` override. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Search/Introduction
Search/run
```
<!-- apiref:toctree:end -->

## Changelog

- **0.1** — `Search` with `grid` / `random` / `build` factories and `run`
  (model-factory × [`LabeledBatch` / `(x, y)` / `(frame, data_options)` /
  `(workflow, payload)`] × [stored split | explicit `SplitOptions`]).
