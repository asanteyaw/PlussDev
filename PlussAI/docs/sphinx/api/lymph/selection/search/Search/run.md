<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7dcfcb64e15c -->

# `lymph::selection::search::Search::run`

[Back to `Search`](../Search.md)

`run` is `Search`'s only method. It calls the `make_model` closure at every point
the search visits, cross-validates that full `models::Model` (prep stages
included), ranks the trials by an objective metric, and returns a
[`search::Result`](../Result.md).

## Signature

```cpp
// make_model is always first: a closure from one grid point to a full Model.
using MakeModel = std::function<models::Model(const GridPoint&)>;

search::Result run(const MakeModel& make_model,
                   const pluss::table::DataFrame& frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& train_options = {},
                   const AssessOptions& assess_options = {},
                   selection::Scorer objective = MetricKind::Accuracy,
                   TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                   FitOptions fit_options = {}) const;

// Each data shape also has a `(…, const SplitOptions& split, …)` form that
// overrides the split policy stored on the Search for this one call.
search::Result run(const MakeModel&, const pluss::table::DataFrame& frame,
                   const DataFrameBatchOptions& data_options,
                   const SplitOptions& split, /* …trailing args… */) const;
```

Data shapes accepted, each with and without the explicit `split` argument:

| Shape | Notes |
|-------|-------|
| `(frame, data_options)` | column-named frame + `DataFrameBatchOptions` |
| `(x, y)` — `y` a `Series` | features-only frame, label series |
| `(x, y)` — `y` a `Thread` | same, label as a `Thread` |
| `LabeledBatch` | pre-split features / labels (GPU-resident data) |
| `(workflow::Workflow design, workflow::Payload payload)` | each trial's model is wrapped in `design` before the run |

## What It Does

The `Search` was built by [`grid`](../grid.md) (visit every point),
[`random`](../grid.md) (sample `trials` points), or [`build`](../grid.md) (from a
`TuneOptions`). `run` expands that into a list of `GridPoint`s, and for each
point:

1. calls `make_model(point)` — this builds a real `Model`, prep stages and all
2. cross-validates it under the split policy (stored, or the `split` override)
3. records a trial: the point's `label()`, its objective value, its fold report

Trials are ranked by `objective` (`objective_policy` decides higher- or
lower-is-better; `Auto` picks the convention for the metric). The winner is
re-fitted on the whole dataset. If `fit_options.fit_top > k`, the top *k* are
each re-fitted so [`Result::top(k)`](../Result.md) can hand them back.

The `make_model` cost model is deliberate: every trial rebuilds and refits its
whole model, prep included, per fold — the same cost
`selection::compare` already pays per candidate.

## Parameters

`make_model` — `std::function<models::Model(const GridPoint&)>`
: Builds a full `Model` from a grid point. Read axis values with
  `point.at("axis_name")`. Attach prep (`m.standard_scaler()`, …) here — it is
  refit per fold, so there is no leakage.

`frame` + `data_options` — or `x` + `y`, or `batch`, or `design` + `payload`
: The labelled data, in any shape from the table above.

`split` — `lymph::SplitOptions` *(the overloaded forms only)*
: Overrides the split policy stored on the `Search` for this call.

`train_options` / `assess_options`
: Passed through to every per-fold fit and assessment. An empty
  `assess_options.metrics` is filled with the `objective` metric (or the task
  default when `objective` is a custom scorer).

`objective` — `lymph::selection::Scorer`
: What trials are ranked on -- implicitly a `MetricKind` (default `Accuracy`), or
  `selection::make_scorer("name", fn)` for a custom callable. See
  [`Scorer`](../../Scorer.md).

`objective_policy` — `lymph::TuneObjectivePolicy`
: `Auto` (metric's own convention), `Maximize`, or `Minimize`.

`fit_options` — `lymph::selection::FitOptions`
: `fit_top` — how many of the ranked leaders to re-fit on the full data. `0`
  still re-fits the single winner for `Result::best()`.

## Returns

A [`search::Result`](../Result.md): `.report` (a `CompareReport` — `board()`,
`best_name`, `trials`), `.best()` (the re-fitted winner), `.top(k)`.

## Errors

- The search space is empty (an axis has zero values) → raises.
- `fit_options.fit_top < 0` → raises.
- A `random` search with `trials <= 0` → raises.
- A fold or holdout block that ends up single-class fails a classification
  metric at assess time — pass a `CrossValOptions`-built `Search` (its
  reliability defaults stratify) or supply already-balanced folds.

## Practical Examples

<!-- these examples establish: grid + cv run -> board, the split override, and the (x, y) shape with top(k). -->

### Example 1

`grid(axes, cv)` — one axis, three values — run against `(frame, data_options)`.
`make_model` attaches a `standard_scaler`, so `best().stage_count()` is `1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Thread(x1)}, {"x2", Thread(x2)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
  };
  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = true,
    });
    m.standard_scaler();
    return m;
  };

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.shuffle = false;

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 200;
  train_options.tolerance = 1e-6;

  const lymph::selection::Search search = lymph::selection::grid(axes, cv);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, train_options, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name
            << ", best trained = " << std::boolalpha << tuned.best().trained()
            << ", best stages = " << tuned.best().stage_count() << "\n\n";
  std::cout << tuned.report.board();

  return 0;
}
```

Output

```text
trials = 3, best = regularization_strength=0.010000, best trained = true, best stages = 1

========================================================
     | name                    objective  rank  accuracy
--------------------------------------------------------
0    | regularization_streng~  1          1     1       
1    | regularization_streng~  1          1     1       
2    | regularization_streng~  1          1     1       
--------------------------------------------------------
[3 cs x 4 feat]
```

### Example 2

The same `Search` carries a CV policy, but this `run` passes an explicit
`SplitOptions` — a single 30% holdout — overriding it for this call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Thread(x1)}, {"x2", Thread(x2)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
  };
  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = true,
    });
    m.standard_scaler();
    return m;
  };

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.shuffle = false;
  const lymph::selection::Search search = lymph::selection::grid(axes, cv);

  lymph::SplitOptions holdout{};
  holdout.plan = lymph::CVPlanKind::Holdout;
  holdout.holdout_fraction = 0.3;
  holdout.shuffle = false;

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 200;

  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, holdout, train_options,
                 lymph::AssessOptions{}, lymph::MetricKind::Accuracy,
                 lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name << "\n\n";
  std::cout << tuned.report.board();

  return 0;
}
```

Output

```text
trials = 3, best = regularization_strength=0.010000

========================================================
     | name                    objective  rank  accuracy
--------------------------------------------------------
0    | regularization_streng~  1          1     1       
1    | regularization_streng~  1          1     1       
2    | regularization_streng~  1          1     1       
--------------------------------------------------------
[3 cs x 4 feat]
```

### Example 3

A two-axis grid run through the `(x, y)` shape. `FitOptions.fit_top = 3` means
[`Result::top(2)`](../Result.md) can return two owned `(name, Model)` pairs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame x({{"x1", pluss::table::Thread(x1)}, {"x2", pluss::table::Thread(x2)}});

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = point.at("fit_intercept") != 0.0,
    });
    m.standard_scaler();
    return m;
  };

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.shuffle = false;

  const lymph::selection::Search search = lymph::selection::grid(axes, cv);
  const lymph::selection::search::Result tuned =
      search.run(make_model, x, y, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{.fit_top = 3});

  std::cout << "grid points = " << tuned.report.trials.size() << "\n";
  const auto top2 = tuned.top(2);
  std::cout << "top(2):\n";
  for (const auto& [name, model] : top2) {
    std::cout << "  " << name << "  trained=" << std::boolalpha << model.trained() << "\n";
  }

  return 0;
}
```

Output

```text
grid points = 4
top(2):
  regularization_strength=0.010000, fit_intercept=0.000000  trained=true
  regularization_strength=1.000000, fit_intercept=0.000000  trained=true
```

## See Also

- [`Result`](../Result.md) — what `run` returns.
- [`Scorer`](../../Scorer.md) — the `objective` type; `make_scorer` for a
  custom ranking metric.
- [`grid` / `random` / `build`](../grid.md) — the factories that build a
  `Search`.
- [`Plan::run`](../../validate/Plan/run.md) — the fixed-model cross-validation
  counterpart.
- [`selection::spotcheck`](../../spotcheck.md) — a strategy × model grid rather
  than a hyperparameter grid.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
</div>
<!-- apiref:nav:end -->
