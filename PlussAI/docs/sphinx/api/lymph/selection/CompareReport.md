<!-- pluss-hand-authored -->

# `lymph::selection::CompareReport`

[Back to `selection`](../selection.md)

`CompareReport` is the **ranked-trial report** every comparison in `selection`
produces — [`compare`](compare.md), [`search::Search::run`](search/Search/run.md),
and [`spotcheck::Suite::run`](spotcheck/Suite/run.md) all return it (directly, or
wrapped in a `CompareResult`). It keeps the whole ranking, not just the winner:
who placed where, on what objective, with which per-metric means.

## Fields

`std::vector<Trial> trials`
: One [`Trial`](#the-trial-value-type) per candidate, in **candidate order** (not
  ranked order).

`std::int64_t best_index` / `std::string best_name`
: The winning trial's position in `trials`, and its name.

`MetricKind objective_metric` / `std::string objective_name` /
`TuneObjectivePolicy objective_policy` / `bool objective_minimize`
: What the ranking was computed on. `objective_metric` is the built-in metric
  (or `Accuracy` as a placeholder when the objective was a custom
  [`Scorer`](Scorer.md)); `objective_name` is the human-readable label
  (`"f1"`, or the `make_scorer` name); `objective_minimize` is the resolved
  direction the ranking used.

`std::vector<std::int64_t> objective_rank`
: Per-trial rank (`1` = best), aligned with `trials`. Ties share a rank.

`std::vector<std::int64_t> ranked_indices`
: `trials` indices in ranked order, best first — `ranked_indices.front() ==
  best_index`.

## The `Trial` value type

```cpp
struct Trial {
  std::string name{};
  double objective = /* NaN until computed */;
  metrics::CrossValidationReport cv{};
};
```

One trial is one candidate's outcome: its `name`, its `objective` (the
objective fold mean), and the full `cv` report behind that number —
`trial.cv.report.folds`, `trial.cv.board()`, and the rest of
[`reports::CrossVal`](../metrics/CrossValidationReport.md).

## `CompareResult`

`compare` / `Suite::run` return a `CompareResult`, which wraps the report with
fitted models:

```cpp
struct CompareResult {
  CompareReport report{};
  std::optional<models::Model> best_model{};
  std::optional<models::Model> worst_model{};
  std::vector<models::Model> fitted{};          // top FitOptions.fit_top, ranked
  std::vector<models::Model> bottom_fitted{};   // bottom FitOptions.fit_bottom

  const models::Model& best() const;            // throws if unset
  const models::Model& worst() const;           // throws if unset
  std::vector<std::pair<std::string, models::Model>> top(std::int64_t k) const;
  std::vector<std::pair<std::string, models::Model>> bottom(std::int64_t k) const;
};
```

`best()` / `worst()` are the single re-fitted extremes (always available once a
comparison ran). `top(k)` / `bottom(k)` need `k <= FitOptions.fit_top` /
`fit_bottom` — only that many models were re-fitted — and return owned
`(name, Model)` pairs.

[`search::Result`](search/Result.md) is a parallel wrapper with `best_model` /
`fitted` but no `worst` side.

## Methods

### `board`

```cpp
pluss::table::DataFrame board() const;
```

The ranked table: one row per trial, columns for `name`, `objective`, `rank`,
and every requested metric's fold mean. This is the surface you print, export,
or attach to experiment notes.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression());
  candidates.emplace_back("bayes", lymph::models::gaussian_nb_classifier());
  candidates.emplace_back("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::CompareResult ranked =
      lymph::selection::compare(std::move(candidates), frame, data_options, cv,
                                lymph::TrainOptions{}, assess, lymph::MetricKind::Accuracy,
                                lymph::TuneObjectivePolicy::Maximize,
                                lymph::selection::FitOptions{});

  const lymph::selection::CompareReport& report = ranked.report;
  std::cout << "best_index = " << report.best_index
            << ", best_name = " << report.best_name << "\n";
  std::cout << "ranked_indices =";
  for (const auto i : report.ranked_indices) std::cout << " " << i;
  std::cout << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
best_index = 0, best_name = logreg
ranked_indices = 0 2 1

======================================================
     | name    objective  rank  accuracy  f1          
------------------------------------------------------
0    | logreg  0.9375     1     0.9375    0.9285714286
1    | bayes   0.875      3     0.875     0.873015873 
2    | tree    0.9375     1     0.9375    0.9444444444
------------------------------------------------------
[3 cs x 5 feat]
```

### Reading `trials`

`report.trials` is the per-candidate detail behind the board.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression());
  candidates.emplace_back("bayes", lymph::models::gaussian_nb_classifier());
  candidates.emplace_back("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::CompareResult ranked =
      lymph::selection::compare(std::move(candidates), frame, data_options, cv,
                                lymph::TrainOptions{}, lymph::AssessOptions{},
                                lymph::MetricKind::Accuracy,
                                lymph::TuneObjectivePolicy::Maximize,
                                lymph::selection::FitOptions{});

  for (const lymph::selection::Trial& trial : ranked.report.trials) {
    std::cout << trial.name << ": objective=" << trial.objective
              << ", folds=" << trial.cv.report.folds.size() << "\n";
  }

  return 0;
}
```

Output

```text
logreg: objective=0.9375, folds=2
bayes: objective=0.875, folds=2
tree: objective=0.9375, folds=2
```

### Promoting the winner

`CompareResult::best()` is the winning candidate re-fitted on the whole dataset
— trained, ready to infer.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression());
  candidates.emplace_back("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::CompareResult ranked =
      lymph::selection::compare(std::move(candidates), frame, data_options, cv,
                                lymph::TrainOptions{}, lymph::AssessOptions{},
                                lymph::MetricKind::Accuracy,
                                lymph::TuneObjectivePolicy::Maximize,
                                lymph::selection::FitOptions{.fit_top = 1});

  const lymph::models::Model& winner = ranked.best();
  std::cout << "winner = " << ranked.report.best_name
            << ", trained = " << std::boolalpha << winner.trained() << "\n";
  const pluss::Series predicted = winner.infer(frame.features({"x0", "x1"}));
  std::cout << "predictions = " << predicted.numel() << "\n";

  return 0;
}
```

Output

```text
winner = logreg, trained = true
predictions = 16
```

## Changelog

- **0.1** — `CompareReport` (`trials` / `best_index` / `best_name` /
  `objective_metric` / `objective_name` / `objective_policy` / `objective_rank`
  / `board()`), the `Trial` value type, and the `CompareResult` wrapper
  (`best` / `worst` / `top` / `bottom`).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
