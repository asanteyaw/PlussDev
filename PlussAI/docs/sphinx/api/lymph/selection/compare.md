<!-- pluss-hand-authored -->

# `lymph::selection::compare`

[Back to `selection`](../selection.md)

`compare` ranks a **fixed list of already-built candidates** by cross-validation.
Each candidate is a `{name, model, optional TrainOptions}` triple you assembled
yourself — this is model *selection*, not a hyperparameter *search* (for that,
see [`selection::search`](search.md)).

## Signature

```cpp
lymph::selection::CompareResult lymph::selection::compare(
    std::vector<lymph::selection::Candidate> candidates,
    /* data */,
    lymph::selection::CrossValOptions cv = {},
    lymph::TrainOptions train_options = {},
    lymph::AssessOptions assess_options = {},
    lymph::selection::Scorer objective = lymph::MetricKind::Accuracy,
    lymph::TuneObjectivePolicy objective_policy = lymph::TuneObjectivePolicy::Auto,
    lymph::selection::FitOptions fit_options = {});
```

`objective` is a [`Scorer`](Scorer.md) — implicitly a `MetricKind`
(`objective = MetricKind::F1`), or `selection::make_scorer("name", fn)` for a
custom callable ranked on each fold's `(truth, prediction)`. See
[`Scorer`](Scorer.md).

The `/* data */` slot is one of:

| Shape | Notes |
|-------|-------|
| `const LabeledBatch& batch` | pre-split features / labels (GPU-resident data path) |
| `const pluss::table::DataFrame& x, const Series& y` | features-only frame + label series |
| `const pluss::table::DataFrame& x, const pluss::table::Slate& y` | same, label as a `Slate` |
| `const pluss::table::DataFrame& frame, const DataFrameBatchOptions& data_options` | column-named frame + `DataFrameBatchOptions` (the one shape that can carry a `note_policy`) |

## What It Does

Every candidate is cross-validated under the one `cv` policy, its objective
value (the `objective` fold mean) recorded, and the candidates ranked.
The winner is re-fitted on the whole dataset; `fit_options.fit_top` /
`fit_bottom` re-fit that many ranked leaders and laggards so
[`CompareResult::top(k)`](CompareReport.md) / `bottom(k)` can return them.

A candidate whose `train_options` field is set trains under *its own* settings;
the rest use the shared `train_options` argument. That per-candidate override is
what makes `compare` usable when one model needs calibration or a class weight
the others do not.

## The `Candidate` value type

```cpp
struct Candidate {
  std::string name{};
  models::Model model;                        // cheap-copy handle
  std::optional<TrainOptions> train_options{};  // nullopt = use compare()'s shared train_options
  Candidate(std::string name, models::Model model,
            std::optional<TrainOptions> train_options = std::nullopt);
};
```

Construct in place: `candidates.emplace_back("logreg",
lymph::models::logistic_regression())`, optionally with a third `TrainOptions`
argument. `name` is what shows up in the ranked board.

## Parameters

`candidates` — `std::vector<lymph::selection::Candidate>`
: The models to rank, each named. Moved in.

`batch` / `x` + `y` / `frame` + `data_options`
: The labelled data, in any shape from the table above.

`cv` — `lymph::selection::CrossValOptions`
: The one cross-validation policy every candidate is judged under.

`train_options` — `lymph::options::Train`
: The shared per-fold training configuration. A `Candidate` with its own
  `train_options` ignores this.

`assess_options` — `lymph::options::Assess`
: Metrics for the board. An empty set is filled from the `objective` metric, or
  from the task default when `objective` is a custom [`Scorer`](Scorer.md).

`objective` — `lymph::selection::Scorer`
: What candidates are ranked on. Implicitly a `MetricKind`; or
  `selection::make_scorer("name", fn, greater_is_better)` for a callable scored
  on each fold's `(truth, prediction)`. See [`Scorer`](Scorer.md).

`objective_policy` — `lymph::TuneObjectivePolicy`
: `Auto` (the metric's own convention, or the custom scorer's
  `greater_is_better`), `Maximize`, or `Minimize` — the last two override the
  objective's direction.

`fit_options` — `lymph::selection::FitOptions`
: `fit_top` / `fit_bottom` — how many ranked leaders / laggards to re-fit on the
  full data. `0` still re-fits the single `best()` and `worst()`.

## Returns

A [`lymph::selection::CompareResult`](CompareReport.md): `.report` (a
[`CompareReport`](CompareReport.md) — `board()`, `best_name`, `trials`),
`.best()` / `.worst()`, `.top(k)` / `.bottom(k)`.

## Errors

- An empty candidate list → raises.
- A metric needing both classes on a single-class fold → raises at assess time.
  The `cv` reliability defaults stratify the folds, which normally prevents it.

## Practical Examples

<!-- these examples establish: the frame board, the (x, y) shape, per-candidate TrainOptions, and top/bottom selection. -->

### Example 1

Three named candidates, cross-validated under one policy over `(frame,
data_options)`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

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
                                lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
```

Output

```text
trials = 3, best = logreg

========================================
     | name    objective  rank  accuracy
----------------------------------------
0    | logreg  0.9375     1     0.9375  
1    | bayes   0.875      3     0.875   
2    | tree    0.9375     1     0.9375  
----------------------------------------
[3 cs x 4 feat]
```

### Example 2

The `(x, y)` overload — `x` a features-only frame, `y` a label `Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame x({{"x0", Slate(x0)}, {"x1", Slate(x1)}});

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression());
  candidates.emplace_back("bayes", lymph::models::gaussian_nb_classifier());

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::CompareResult ranked =
      lymph::selection::compare(std::move(candidates), x, y, cv,
                                lymph::TrainOptions{}, lymph::AssessOptions{},
                                lymph::MetricKind::Accuracy,
                                lymph::TuneObjectivePolicy::Maximize,
                                lymph::selection::FitOptions{});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
```

Output

```text
trials = 2, best = logreg

========================================
     | name    objective  rank  accuracy
----------------------------------------
0    | logreg  0.9375     1     0.9375  
1    | bayes   0.875      2     0.875   
----------------------------------------
[2 cs x 4 feat]
```

### Example 3

A per-`Candidate` `TrainOptions` — calibration on the logistic candidate only,
while the shared `train_options` argument stays default.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  lymph::TrainOptions calibrated{};
  calibrated.calibration = lymph::ScoreCalibration::Platt;

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg_calibrated", lymph::models::logistic_regression(), calibrated);
  candidates.emplace_back("bayes_plain", lymph::models::gaussian_nb_classifier());

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

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
```

Output

```text
trials = 2, best = logreg_calibrated

===================================================
     | name               objective  rank  accuracy
---------------------------------------------------
0    | logreg_calibrated  0.9375     1     0.9375  
1    | bayes_plain        0.875      2     0.875   
---------------------------------------------------
[2 cs x 4 feat]
```

### Example 4

`FitOptions.fit_top` / `fit_bottom` — read the ranked leaders and laggards back
as owned models.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

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
                                lymph::selection::FitOptions{.fit_top = 2, .fit_bottom = 1});

  std::cout << "best = " << ranked.report.best_name
            << " (trained=" << std::boolalpha << ranked.best().trained() << ")\n";
  std::cout << "worst trained = " << ranked.worst().trained() << "\n";

  for (const auto& [name, model] : ranked.top(2)) {
    std::cout << "  top: " << name << " trained=" << model.trained() << "\n";
  }
  for (const auto& [name, model] : ranked.bottom(1)) {
    std::cout << "  bottom: " << name << " trained=" << model.trained() << "\n";
  }

  return 0;
}
```

Output

```text
best = logreg (trained=true)
worst trained = true
  top: logreg trained=true
  top: tree trained=true
  bottom: bayes trained=true
```

## See Also

- [`CompareReport`](CompareReport.md) — the report / `CompareResult` types this
  returns.
- [`Scorer`](Scorer.md) — the `objective` type; `make_scorer` for a custom
  ranking metric.
- [`selection::spotcheck`](spotcheck.md) — builds the candidate list for you
  from a `models::Builder` × strategy grid.
- [`selection::search`](search.md) — a parameter grid instead of a fixed list.
- [`Plan::run(builder, …)`](validate/Plan/run.md) — `Builder::benchmark`
  through a validation `Plan`.
