<!-- pluss-hand-authored -->

# `lymph::selection::Scorer`

[Back to `selection`](../selection.md)

A `Scorer` is the **objective** that `compare`, `Search::run`,
`spotcheck::Suite::run`, `Builder::benchmark`, and
[`Plan::score` / `Plan::scores`](validate/Plan/scores.md) rank / score models
by. It is *either* a built-in [`MetricKind`](../metrics/MetricReport.md) —
scored on the normal assessment path — *or* a user callable evaluated on every
fold's `(truth, prediction)` pair.

`Scorer` is implicitly constructible from `MetricKind`, so
`objective = MetricKind::F1` and every existing call keeps working unchanged.
`make_scorer(...)` builds the custom form.

## Scope

`lymph::selection::Scorer`. No umbrella alias. It is a plain value type — copy
it freely.

## Fields

`std::string name`
: A label for the objective. For a built-in it is the metric name
  (`"accuracy"`, `"f1"`, …); for `make_scorer` it is the name you passed. Ends
  up on `CompareReport::objective_name`.

`bool greater_is_better` *(custom only)*
: Whether a larger value ranks higher. Ignored for a built-in, which carries
  its own natural direction (`Accuracy` up, `LogLoss` down, …). An explicit
  `TuneObjectivePolicy::Minimize` / `Maximize` on the call still overrides it.

`std::optional<MetricKind> builtin`
: Set when the objective is a built-in metric. `is_custom()` is `!builtin`.

`std::function<double(const Series& truth, const Series& prediction)> fn`
: The custom callable. Receives that fold's ground-truth `Series` and the
  model's hard-prediction `Series` (from `model.infer(...)`), returns one
  scalar. Runs once per `(candidate, fold)` cell, on the worker thread that
  trained that fold — it **must be safe to call concurrently** when
  `CrossValOptions::max_parallel != 1`. A non-finite return drops that fold
  from the candidate's mean (all folds non-finite → the candidate is unranked).

## Construction

```cpp
// implicit — a built-in objective
lymph::selection::Scorer objective = lymph::MetricKind::F1;

// custom — name, callable, direction
lymph::selection::Scorer lymph::selection::make_scorer(
    std::string name,
    std::function<double(const pluss::Series& truth, const pluss::Series& prediction)> fn,
    bool greater_is_better = true);
```

## How It Is Used

A candidate's objective is the **fold mean** of the scorer's per-fold values —
the same aggregation a built-in metric gets. The winner is the best mean under
the effective direction. `CompareReport::board()`'s `objective` column holds
that mean; companion metric columns still come from `assess_options.metrics`
(seeded to the task default when a custom objective leaves it empty).

Built-in path: zero overhead versus naming the `MetricKind` directly. Custom
path: one extra `model.infer(...)` per `(candidate, fold)` cell, inside the
existing parallel pass — no extra training.

## Not Covered

- `Plan::score` / `Plan::scores` take a custom `Scorer` on the `models::Model`
  and `workflow::Workflow` overloads; the `ModelSpec` overloads, and
  `tune_specs` / `probe_specs`, stay `MetricKind`-only — they run through a
  compiled single-metric cross-validation path that takes no host callback.
- On the bare `LabeledBatch` shape a custom `Scorer` needs a stage-free model
  (a nameless batch cannot route appended prep) — use `(frame, data_options)`
  or `(x, y)` for staged models.
- The callable sees hard predictions, not probabilities. A score-based custom
  objective is not available yet; for the standard score metrics use
  `MetricKind::RocAuc` / `AveragePrecision` / `Brier` / `LogLoss` as a built-in
  objective.

## Example

Rank two models by a **cost-weighted accuracy** — a false negative costs 5× a
false positive — a metric with no `MetricKind`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace selection = lymph::selection;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const Series x1 = pluss::make_series(
      {2., 1., 4., 3., 6., 5., 8., 7., 9., 11., 10., 13., 12., 15., 14., 16.}, {16}, f64);
  const Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  // (5 * false-negatives + false-positives), turned into a [0, 1] "accuracy" by
  // dividing by the worst possible weighted error on the fold.
  const auto cost_weighted_accuracy = [](const Series& truth, const Series& prediction) -> double {
    const Series t = truth.reshape({truth.numel()}).to(ScalarType::Float64);
    const Series p = prediction.reshape({prediction.numel()}).to(ScalarType::Float64);
    const Series is_pos = t.eq(1.0).to(ScalarType::Float64);
    const Series is_neg = t.eq(0.0).to(ScalarType::Float64);
    const double false_neg = is_pos.mul(p.eq(0.0).to(ScalarType::Float64)).sum().item<double>();
    const double false_pos = is_neg.mul(p.eq(1.0).to(ScalarType::Float64)).sum().item<double>();
    const double positives = is_pos.sum().item<double>();
    const double negatives = static_cast<double>(t.numel()) - positives;
    const double worst = 5.0 * positives + negatives;
    return worst > 0.0 ? 1.0 - (5.0 * false_neg + false_pos) / worst : 1.0;
  };

  std::vector<selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression({.fit_intercept = true}));
  candidates.emplace_back("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));

  selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const selection::CompareResult ranked = selection::compare(
      std::move(candidates), frame, data_options, cv, lymph::TrainOptions{}, lymph::AssessOptions{},
      selection::make_scorer("cost_weighted_accuracy", cost_weighted_accuracy),
      lymph::TuneObjectivePolicy::Auto, selection::FitOptions{.fit_top = 1});

  std::cout << "objective = " << ranked.report.objective_name << "\n";
  std::cout << "best      = " << ranked.report.best_name << "\n";
  for (const auto& trial : ranked.report.trials) {
    std::cout << "  " << trial.name << " -> " << trial.objective << "\n";
  }

  return 0;
}
```

Output

```text
objective = cost_weighted_accuracy
best      = tree
  logreg -> 0.895833
  tree -> 0.979167
```

## Methods

None. `Scorer` is a plain value type — `is_custom()` is its only member
function. The factory `lymph::selection::make_scorer(name, fn,
greater_is_better = true)` is a free function; the `MetricKind` form needs no
factory (the implicit constructor handles it).

## Changelog

- **0.1** — `Scorer` + `make_scorer`; accepted as the `objective` argument of
  `selection::compare`, `Search::run`, `spotcheck::Suite::run`,
  `Builder::benchmark` / `validate::Plan::run(builder, …)`, and
  `validate::Plan::score` / `Plan::scores` (`models::Model` / `workflow`
  overloads).

## See Also

- [`compare`](compare.md) / [`search`](search.md) / [`spotcheck`](spotcheck.md)
  — the three APIs that take a `Scorer` objective.
- [`CompareReport`](CompareReport.md) — carries `objective_name` and the ranked
  trials.
- [`metrics`](../metrics.md) — the built-in `MetricKind` set.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
