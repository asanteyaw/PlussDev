# `lymph::search::Search`

## Summary

`Search` is the Lymph hyperparameter-search surface.

Use it when you want a reusable object that explores a search space and records
the result in a model-aware way.

## What Makes Lymph Search Different

The central design point is this:

`Search` does not only vary a bare estimator spec. It varies a model-producing
factory.

That means each trial can build a full `Model`, including prep stages, rather than
only a naked learner core.

This is a much more realistic search surface for actual projects because the
winning configuration often depends on more than the learner hyperparameters
alone.

## Core Idea

The central input to `run(...)` is a trial factory:

- one grid point in
- one `Model` out

That means search is not limited to numeric parameter mutation. A trial can change:

- learner hyperparameters
- attached prep stages
- stage options
- branch-local structure

as long as the result is still a valid `Model`.

## Signature

```cpp
lymph::Search lymph::search::grid(
    std::vector<lymph::GridAxis> axes,
    lymph::ValidationOptions cv);
lymph::Search lymph::search::random(...);
lymph::search::Result lymph::Search::run(...) const;
```

## Practical Example

This page uses the exact verifier program `lymph_tune_docverify_demo.cpp`.

### Example 1

This example defines one search axis, builds the search object, runs the trial
factory, and prints the resulting ranking board.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classic = pluss::ai::classic;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::ValidationOptions cv{};
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 20260825ULL;
  cv.deterministic = true;

  const std::vector<lymph::GridAxis> axes = {
      {"regularization_strength", {1e-4, 1e-3, 1e-2}},
  };
  const lymph::Search search = lymph::search::grid(axes, cv);

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::search::Result search_result =
      search.run(
          [](const lymph::GridPoint& point) {
            classic::LinearFamilyOptions tuned{};
            tuned.regularization = classic::RegularizationKind::L2;
            tuned.regularization_strength =
                point.at("regularization_strength");
            tuned.fit_intercept = true;
            return lymph::model::logistic_regression(tuned);
          },
          frame,
          data_options,
          train_options,
          assess_options,
          lymph::MetricKind::F1,
          lymph::TuneObjectivePolicy::Auto,
          {.fit_top = 1});

  std::cout << "== search_result ==\n";
  std::cout << "best=" << search_result.report.best_name << "\n";
  std::cout << search_result.report.board() << "\n";

  return 0;
}
```

Output

```text
== search_result ==
best=regularization_strength=0.000100
==============================================================
     | name                    objective  rank  accuracy  f1
--------------------------------------------------------------
0    | regularization_streng~  1          1     1         1
1    | regularization_streng~  1          1     1         1
2    | regularization_streng~  1          1     1         1
--------------------------------------------------------------
[3 cs x 5 feat]
```

The exact winner in this tiny demo is less important than the shape of the result:

- every grid point becomes a named trial
- the ranking board is retained, not discarded
- the winning model can be promoted directly from the result object

## Related Pages

- [Suite](Suite.md)
- [TuneReport](TuneReport.md)
- [Builder](../model/Builder.md)
