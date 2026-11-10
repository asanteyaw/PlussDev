<!-- pluss-hand-authored -->

# `lymph::selection::search::grid`

[Back to `selection::search`](../search.md)

`grid` builds an **exhaustive** [`Search`](Search.md) — one trial per point in
the cross product of the axes. Its two relatives, `random` (a fixed sample of
points) and `build` (from an assembled `TuneOptions`), return the same
`Search` type and differ only in how the point list is chosen.

## Signature

```cpp
// exhaustive
lymph::selection::Search lymph::selection::grid(
    std::vector<lymph::selection::GridAxis> axes,
    lymph::SplitOptions split = {});
lymph::selection::Search lymph::selection::grid(
    std::vector<lymph::selection::GridAxis> axes,
    lymph::selection::CrossValOptions cv);

// sampled -- `trials` points drawn from the space
lymph::selection::Search lymph::selection::random(
    std::vector<lymph::selection::GridAxis> axes, std::int64_t trials,
    lymph::RandomState random = {}, lymph::SplitOptions split = {});
lymph::selection::Search lymph::selection::random(
    std::vector<lymph::selection::GridAxis> axes, std::int64_t trials,
    lymph::selection::CrossValOptions cv);
lymph::selection::Search lymph::selection::random(
    std::vector<lymph::selection::GridAxis> axes, std::int64_t trials,
    lymph::RandomState random, lymph::selection::CrossValOptions cv);

// from an assembled TuneOptions
lymph::selection::Search lymph::selection::build(
    std::vector<lymph::selection::GridAxis> axes, lymph::TuneOptions options);
```

All three live in `lymph::selection::search` and re-export to
`lymph::selection`.

## What It Does

Each factory returns a `Search` holding the axes, a search kind, and a split
policy. Nothing runs until [`Search::run`](Search/run.md).

- **`grid`** — search kind `Grid`. `run` visits every point;
  `lymph::selection::grid_size(axes)` is that count (the product of the axis
  lengths).
- **`random`** — search kind `Random`. `run` samples `trials` points. The seed
  comes from the `RandomState` argument, or from `cv.random_state` /
  `cv.deterministic` in the `CrossValOptions` overload.
- **`build`** — reads `options.search` (`Grid` or `Random`), `options.trials`,
  `options.random`, `options.split`, and `options.max_parallel` from one
  struct. Use it when the search configuration is already assembled as a value.

The `SplitOptions` overloads take a raw split; the `CrossValOptions` overloads
translate a validation plan into one and turn on its reliability defaults
(stratified folds), which is what keeps small classification folds scorable.

## A `GridAxis`

### `GridAxis` fields

| Field | Type | Default |
|---|---|---|
| `name` | `std::string` | — |
| `values` | `std::vector<double>` | — |

Values are always `double`. For a boolean or enum hyperparameter, sweep `{0.0,
1.0}` (or `{0, 1, 2}`) and convert inside `make_model`:
`point.at("fit_intercept") != 0.0`.

## Parameters

`axes` — `std::vector<lymph::selection::GridAxis>`
: The search space. An empty-valued axis makes the space empty and `run`
  raises.

`split` — `lymph::SplitOptions` / `cv` — `lymph::selection::CrossValOptions`
: The per-trial cross-validation policy stored on the `Search`.

`trials` — `std::int64_t` *(`random` only)*
: How many points to sample. Must be `> 0`.

`random` — `lymph::RandomState` *(`random` only)*
: `{seed, deterministic}` for the sampler.

`options` — `lymph::TuneOptions` *(`build` only)*
: `{search, trials, max_parallel, split, random}` in one struct.

## Returns

A `lymph::selection::Search`. Cheap to copy; holds no fitted state.

## Errors

- The factories do not throw. An empty search space, `trials <= 0` for a
  `random` search, or an unsupported `SearchKind` surface at `run` time.

## Practical Examples

<!-- these examples establish: grid_size vs trials run, random's sampling, the cv overload, and build from TuneOptions. -->

### Example 1

`grid` over a 3 × 2 space — six exhaustive trials.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Slate(x1)}, {"x2", Slate(x2)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  std::cout << "grid_size = " << lymph::selection::grid_size(axes) << "\n";

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
      search.run(make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{});

  std::cout << "trials run = " << tuned.report.trials.size() << "\n";

  return 0;
}
```

Output

```text
grid_size = 6
trials run = 6
```

### Example 2

`random` draws a fixed sample — three of the eight points. The `RandomState`
seed makes the draw reproducible.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Slate(x1)}, {"x2", Slate(x2)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.001, 0.01, 0.1, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  std::cout << "grid_size = " << lymph::selection::grid_size(axes)
            << ", sampling 3\n";

  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = point.at("fit_intercept") != 0.0,
    });
    m.standard_scaler();
    return m;
  };

  lymph::SplitOptions split{};
  split.plan = lymph::CVPlanKind::CrossVal;
  split.folds = 4;
  split.shuffle = false;

  const lymph::RandomState seed{.seed = 20260901ULL, .deterministic = true};
  const lymph::selection::Search search =
      lymph::selection::random(axes, /*trials=*/3, seed, split);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{});

  std::cout << "trials run = " << tuned.report.trials.size() << "\n";
  for (const auto& trial : tuned.report.trials) {
    std::cout << "  " << trial.name << "\n";
  }

  return 0;
}
```

Output

```text
grid_size = 8, sampling 3
trials run = 3
  regularization_strength=0.100000, fit_intercept=0.000000
  regularization_strength=1.000000, fit_intercept=0.000000
  regularization_strength=1.000000, fit_intercept=1.000000
```

### Example 3

`random(axes, trials, cv)` — the `CrossValOptions` overload. The plan doubles as
the sampler seed, so there is no separate `RandomState`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Slate(x1)}, {"x2", Slate(x2)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.001, 0.01, 0.1, 1.0}},
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
  cv.random_state = 20260901ULL;
  cv.deterministic = true;

  const lymph::selection::Search search =
      lymph::selection::random(axes, /*trials=*/3, cv);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{});

  std::cout << "trials run = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name << "\n";

  return 0;
}
```

Output

```text
trials run = 3, best = regularization_strength=0.100000, fit_intercept=0.000000
```

### Example 4

`build(axes, TuneOptions)` — the search config comes in as one struct.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Slate(x1)}, {"x2", Slate(x2)}, {"target", Slate(y)}});

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

  lymph::TuneOptions options{};
  options.search = lymph::SearchKind::Grid;
  options.max_parallel = 1;
  options.split.plan = lymph::CVPlanKind::CrossVal;
  options.split.folds = 4;
  options.split.shuffle = false;

  const lymph::selection::Search search = lymph::selection::build(axes, options);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{});

  std::cout << "trials run = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name << "\n";

  return 0;
}
```

Output

```text
trials run = 3, best = regularization_strength=0.010000
```

## See Also

- [`Search`](Search.md) — the object all three factories return.
- [`Search::run`](Search/run.md) — the sweep itself.
- [`Result`](Result.md) — what `run` hands back.
- [`selection::validate`](../validate.md) — `SplitOptions` / `CrossValOptions`
  and their `Plan` form.
