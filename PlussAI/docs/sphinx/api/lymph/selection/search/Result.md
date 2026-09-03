<!-- pluss-hand-authored -->

# `lymph::selection::search::Result`

[Back to `selection::search`](../search.md)

`search::Result` is what [`Search::run`](Search/run.md) returns: the ranked trial
report, the re-fitted best model, and `top(k)` access to the fitted leaders. It
is a plain value — copy it, let it outlive the `Search`.

## Scope And Alias

The type is `lymph::selection::search::Result`. It is usually named through the
subfamily scope, `lymph::selection::search::Result`; there is no shorter alias
(`lymph::selection::Result` is not defined — `search::` disambiguates it from
other result types in `selection`).

## Fields

`CompareReport report`
: The ranked trials. `report.board()` is the summary table;
  `report.best_name` / `report.best_index` name the winner; `report.trials` is
  one [`Trial`](../CompareReport.md) per visited grid point (`name`,
  `objective`, `cv` fold report); `report.ranked_indices` is the trial order,
  best first.

`std::optional<models::Model> best_model`
: The winning point's model, re-fitted on the whole dataset. Read it through
  [`best()`](#best), which throws a clear error if it is unset.

`std::vector<models::Model> fitted`
: The top `FitOptions.fit_top` models, re-fitted on the whole dataset, in ranked
  order. Empty when `fit_top` was `0`. Read through [`top(k)`](#top).

## Methods

### `best`

```cpp
const models::Model& best() const;
models::Model&       best();
```

The re-fitted winning model — trained, and carrying whatever prep the
`make_model` closure attached. Throws if no best model was computed.

#### Example

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

  const lymph::selection::search::Result tuned =
      lymph::selection::grid(axes, cv).run(
          make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
          lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
          lymph::selection::FitOptions{.fit_top = 1});

  const lymph::models::Model& best = tuned.best();
  std::cout << "best trained = " << std::boolalpha << best.trained()
            << ", stages = " << best.stage_count() << "\n";

  const pluss::Series predicted = best.infer(frame.features({"x1", "x2"}));
  std::cout << "predictions = " << predicted.numel() << "\n";

  return 0;
}
```

Output

```text
best trained = true, stages = 1
predictions = 16
```

### `top`

```cpp
std::vector<std::pair<std::string, models::Model>> top(std::int64_t k) const;
```

The `k` best trials as owned `(name, Model)` pairs — the name is the winning
`GridPoint`'s `label()`. Requires `k <= FitOptions.fit_top` (only that many
models were re-fitted); a larger `k`, or a negative one, throws.

#### Example

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

  const lymph::selection::search::Result tuned =
      lymph::selection::grid(axes, cv).run(
          make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
          lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
          lymph::selection::FitOptions{.fit_top = 2});

  const auto top2 = tuned.top(2);
  std::cout << "top(2):\n";
  for (const auto& [name, model] : top2) {
    std::cout << "  " << name << "  trained=" << std::boolalpha << model.trained() << "\n";
  }

  try {
    (void)tuned.top(3);  // only 2 were fitted
  } catch (const std::exception& e) {
    std::cout << "top(3) rejected: more than fit_top were requested\n";
  }

  return 0;
}
```

Output

```text
top(2):
  regularization_strength=0.010000  trained=true
  regularization_strength=0.100000  trained=true
top(3) rejected: more than fit_top were requested
```

### Reading the report

`report` is a `CompareReport`. Its `board()`, `best_name`, `best_index`, and
`trials` describe the ranking directly.

#### Example

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

  const lymph::selection::search::Result tuned =
      lymph::selection::grid(axes, cv).run(
          make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
          lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
          lymph::selection::FitOptions{});

  std::cout << "best_name = " << tuned.report.best_name
            << ", best_index = " << tuned.report.best_index
            << ", trials = " << tuned.report.trials.size() << "\n\n";
  std::cout << tuned.report.board();

  return 0;
}
```

Output

```text
best_name = regularization_strength=0.010000, best_index = 0, trials = 3

========================================================
     | name                    objective  rank  accuracy
--------------------------------------------------------
0    | regularization_streng~  1          1     1       
1    | regularization_streng~  1          1     1       
2    | regularization_streng~  1          1     1       
--------------------------------------------------------
[3 cs x 4 feat]
```

## Changelog

- **0.1** — `report` / `best_model` / `fitted` fields; `best()` and `top(k)`
  accessors.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
