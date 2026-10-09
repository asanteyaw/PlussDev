<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c72c6caf8294 -->

# `lymph::selection::validate::Plan::run`

[Back to `Plan`](../Plan.md)

`run` fits and scores a model across every fold the plan defines, then returns
the pooled report. It does not touch the model you pass — each fold trains a
fresh clone.

## Signature

```cpp
// Model or ModelSpec first argument; data in any of the accepted shapes.
reports::CrossVal run(const models::Model& model,
                      const pluss::table::DataFrame& frame,
                      const DataFrameBatchOptions& data_options,
                      TrainOptions train_options = {},
                      AssessOptions assess_options = {}) const;

reports::CrossVal run(const models::Model& model,
                      const pluss::table::DataFrame& x, const pluss::Series& y,
                      TrainOptions = {}, AssessOptions = {}) const;      // (x, y) pair

reports::CrossVal run(const models::Model& model, const LabeledBatch& batch,
                      TrainOptions = {}, AssessOptions = {}) const;

reports::CrossVal run(const models::Model& model, const workflow::Payload& payload,
                      TrainOptions = {}, AssessOptions = {}) const;      // carries the time role

reports::CrossVal run(const workflow::Workflow& workflow,
                      const workflow::Payload& payload,
                      TrainOptions = {}, AssessOptions = {}) const;      // validates prep + model

// Rank a Builder's candidates under this plan.
selection::CompareResult run(const models::Builder& builder, /* same data shapes */,
                             TrainOptions = {}, AssessOptions = {},
                             selection::Scorer objective = MetricKind::Accuracy,
                             TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                             selection::FitOptions fit_options = {}) const;
```

The first argument is a `models::Model`, a `ModelSpec`, a `workflow::Workflow`,
or a `models::Builder`. The data is `(frame, data_options)`, an `(x, y)` pair
with `y` a `Series` or `Slate`, a `LabeledBatch`, or a
[`workflow::Payload`](../../../workflow/Payload.md). For a `time` plan the `Payload`
overload also carries the timeline role.

## What It Does

The model/spec overloads clone the estimator per fold, fit it on that fold's
train rows, score it on the held-out rows, and pool the per-fold metrics into a
[`lymph::reports::CrossVal`](../../../metrics/CrossValidationReport.md) — the
same type the `metrics` family calls `CrossValidationReport`. The
`workflow::Workflow` overload additionally re-fits the workflow's attached
preparation inside each fold, so leakage from prep fitted on the whole set is
caught.

The `models::Builder` overload is
[`Builder::benchmark`](../../../models/builder/Builder/benchmark.md) reached
through a `Plan`: every candidate is cross-validated under this one policy and
ranked, returning a `selection::CompareResult` (the ranked board plus the fitted
winner when `fit_options.fit_top > 0`).

## Parameters

`model` / `spec` / `workflow` / `builder`
: What to validate. A `Model` or `ModelSpec` is cloned per fold; a `Workflow`
  brings its prep; a `Builder` supplies several candidates to rank.

`frame` + `data_options` — or `x` + `y`, or `batch`, or `payload`
: The labelled data. `(x, y)` needs no `data_options` — every column of `x` is a
  feature. `payload` is required for a `time` plan run through a
  time-aware workflow.

`train_options` — `lymph::options::Train`
: Passed through to each per-fold fit.

`assess_options` — `lymph::options::Assess`
: Which metrics to compute. Defaults to the model's task-appropriate set.

`objective` / `objective_policy` / `fit_options` *(builder overload only)*
: The [`Scorer`](../../Scorer.md) candidates are ranked on (a `MetricKind`, or
  `selection::make_scorer("name", fn)`), whether higher or lower wins, and how
  many top candidates to re-fit on the full data.

## Returns

`reports::CrossVal` for the model / spec / workflow overloads — `report.folds`,
`report.metrics`, `report.mean_metrics`, and `board()` for the summary table.

`selection::CompareResult` for the builder overload — `report.board()`,
`report.best_name`, `report.trials`, and `best()` for the re-fitted winner.

## Errors

- A fold whose test block ends up single-class fails at assess time for a
  classification metric — a real property of the data, not a bug. Use
  `stratified` (or `grouped` with a balanced key) to avoid it.
- `run` on a `time` plan needs the frame already sorted oldest-first when the
  data comes in as `(frame, data_options)`.
- A `groups` length that does not match the row count (grouped plans) raises
  here.

## Practical Examples

<!-- these examples establish: k-fold run -> board, the (x, y) shape, and the builder-ranking overload. -->

### Example 1

`cross().folds(4)` against a logistic model. `run` returns a `reports::CrossVal`;
its `board()` is the per-metric summary.

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

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(4).shuffle(true).seed(20260901ULL);

  const lymph::reports::CrossVal report =
      plan.run(lymph::models::logistic_regression(), frame, data_options, {}, assess);

  std::cout << "folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
folds = 4

===========================================
     | metric    mean          stdev       
-------------------------------------------
0    | accuracy  0.8541666667  0.1487797589
1    | f1        0.825         0.2046338193
-------------------------------------------
[2 cs x 3 feat]
```

### Example 2

The `(x, y)` overload: `x` is a features-only frame, `y` a label `Series`. No
`DataFrameBatchOptions` — every column of `x` is a feature.

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

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);

  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(4).seed(20260901ULL);

  const lymph::reports::CrossVal report =
      plan.run(lymph::models::logistic_regression(), x, y, {}, assess);

  std::cout << "folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
folds = 4

===========================================
     | metric    mean          stdev       
-------------------------------------------
0    | accuracy  0.8541666667  0.1487797589
1    | f1        0.825         0.2046338193
-------------------------------------------
[2 cs x 3 feat]
```

### Example 3

The `models::Builder` overload ranks candidates under one plan and returns a
`selection::CompareResult` — spelled with the family-local `CrossValOptions`
type on the plan for contrast.

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

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  lymph::models::Builder candidates = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  const lymph::selection::Plan plan =
      lymph::selection::stratified().folds(4).seed(20260901ULL);

  const lymph::selection::CompareResult ranked =
      plan.run(candidates, frame, data_options, {}, {},
               lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "best = " << ranked.report.best_name
            << ", trials = " << ranked.report.trials.size()
            << ", winner trained = " << std::boolalpha << ranked.best().trained() << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
```

Output

```text
best = logreg, trials = 2, winner trained = true

===============================================
     | name    objective     rank  accuracy    
-----------------------------------------------
0    | logreg  0.8541666667  1     0.8541666667
1    | tree    0.7916666667  2     0.7916666667
-----------------------------------------------
[2 cs x 4 feat]
```

## See Also

- [`scores` / `score`](scores.md) — one metric per fold, or its mean, without
  the full report.
- [`splits` / `last`](splits.md) — the fold boundaries of a `time` plan.
- [`reports::CrossVal`](../../../metrics/CrossValidationReport.md) — the return
  type, at home in `metrics`.
- [`Builder::benchmark`](../../../models/builder/Builder/benchmark.md) — the
  builder overload, reached without a `Plan`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="scores.html">
    <span class="label">Next</span>
    <span class="title">scores</span>
  </a>
</div>
<!-- apiref:nav:end -->
