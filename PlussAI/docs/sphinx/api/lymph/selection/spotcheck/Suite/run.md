<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 293927ddf7ff -->

# `lymph::selection::spotcheck::Suite::run`

[Back to `Suite`](../Suite.md)

`run` forms one candidate per **strategy × model**, cross-validates them all
under one policy, and returns a ranked [`CompareResult`](../../CompareReport.md).
It is [`selection::compare`](../../CompareReport.md) reached through the suite's
strategy grid.

## Signature

```cpp
using Result = selection::CompareResult;   // spotcheck::Result

Result run(const pluss::table::DataFrame& frame,
           const DataFrameBatchOptions& data_options,
           CrossValOptions cv = {},
           AssessOptions assess_options = {},
           MetricKind objective_metric = MetricKind::Accuracy,
           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
           FitOptions fit_options = {}) const;

Result run(const LabeledBatch& batch,
           CrossValOptions cv = {}, /* …same trailing args… */) const;

Result run(const workflow::Workflow& design, const workflow::Payload& payload,
           CrossValOptions cv = {}, /* …same trailing args… */) const;
```

Three data shapes: a column-named `(frame, data_options)`, a pre-split
`LabeledBatch`, or a `(workflow::Workflow, workflow::Payload)` pair — the
workflow form wraps every candidate in `design` first, so the spot-check judges
the whole designed pipeline. For a `time` payload the workflow's `TimeOptions`
merge into `cv`.

## What It Does

For every `strategy` registered on the suite and every model in the
`models::Builder` it was built from, `run` makes a `Candidate` named
`"<strategy>__<model>"`, carrying that model and the strategy's `TrainOptions`.
The full candidate list goes to `selection::compare`, which cross-validates each
under `cv`, ranks them by `objective_metric`, and re-fits the leaders
(`fit_options.fit_top`) and laggards (`fit_options.fit_bottom`) on the whole
set.

Nothing is fitted until `run`. The suite itself only holds the model entries and
the strategy list.

## Parameters

`frame` + `data_options` — or `batch`, or `design` + `payload`
: The labelled data.

`cv` — `lymph::selection::CrossValOptions`
: The per-candidate cross-validation policy — folds, stratification, seed,
  workers. Its reliability defaults stratify the folds, so contiguous-block
  labels stay scorable.

`assess_options` — `lymph::options::Assess`
: Metrics to compute. An empty set is filled from `objective_metric`.

`objective_metric` / `objective_policy`
: The metric candidates are ranked on, and whether higher or lower wins
  (`Auto` uses the metric's own convention).

`fit_options` — `lymph::selection::FitOptions`
: `fit_top` / `fit_bottom` — how many ranked leaders and laggards to re-fit on
  the full data for [`top(k)`](../../CompareReport.md) / `bottom(k)`. `0` still
  re-fits the single `best()` and `worst()`.

## Returns

A `selection::CompareResult` (`spotcheck::Result`): `.report` (a
`CompareReport` — `board()`, `best_name`, `trials`), `.best()` / `.worst()`, and
`.top(k)` / `.bottom(k)`.

## Errors

- No models on the builder, or no strategies registered → raises.
- A metric that needs both classes on a fold that has one → raises at assess
  time (the `cv` reliability defaults normally prevent this).

## Practical Examples

<!-- these examples establish: the strategy x model board, top/bottom selection, and the workflow overload. -->

### Example 1

One strategy, two models — a plain two-trial spot-check over `(frame,
data_options)`.

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

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .run(frame, data_options, cv, lymph::AssessOptions{},
               lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
```

Output

```text
trials = 2, best = plain__tree

==============================================
     | name          objective  rank  accuracy
----------------------------------------------
0    | plain__bayes  0.875      2     0.875   
1    | plain__tree   0.9375     1     0.9375  
----------------------------------------------
[2 cs x 4 feat]
```

### Example 2

The `LabeledBatch` overload, with `FitOptions.fit_top` / `fit_bottom` set so
`top(k)` and `bottom(k)` return owned models.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  lymph::LabeledBatch batch{};
  batch.features.x = pluss::make_series(
      std::vector<double>{1., 1., 2., 1., 3., 2., 4., 2., 5., 3., 6., 3., 7., 4., 8., 4.,
                          9., 8., 10., 8., 11., 9., 12., 9., 13., 10., 14., 10., 15., 11., 16., 11.},
      {16, 2}, f64);
  batch.features.layout = lymph::ObservationLayout::Rows;
  batch.y = pluss::make_series(
      std::vector<double>{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .run(batch, cv, lymph::AssessOptions{}, lymph::MetricKind::Accuracy,
               lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1, .fit_bottom = 2});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n";
  std::cout << "best trained = " << std::boolalpha << ranked.best().trained()
            << ", worst trained = " << ranked.worst().trained() << "\n";

  const auto top1 = ranked.top(1);
  const auto bottom2 = ranked.bottom(2);
  std::cout << "top(1): " << top1.front().first << "\n";
  std::cout << "bottom(2): " << bottom2.front().first << ", " << bottom2.back().first << "\n";

  return 0;
}
```

Output

```text
trials = 2, best = plain__bayes
best trained = true, worst trained = true
top(1): plain__bayes
bottom(2): plain__tree, plain__bayes
```

### Example 3

The `(Workflow, Payload)` overload — each candidate is wrapped in `design`
before cross-validation. The ranked result shape is unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = pluss::ai::lymph::workflow;
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

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  const workflow::Payload payload =
      workflow::payload::make(frame).features({"x0", "x1"}).label("target");
  const workflow::Workflow design = workflow::make(workflow::Policy{});

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .run(design, payload, cv, lymph::AssessOptions{},
               lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name
            << ", best trained = " << std::boolalpha << ranked.best().trained() << "\n";

  return 0;
}
```

Output

```text
trials = 2, best = plain__tree, best trained = true
```

## See Also

- [`strategy`](strategy.md) — register the training strategies `run` crosses
  with the models.
- [`CompareReport`](../../CompareReport.md) — the report type inside the
  result.
- [`Builder::benchmark`](../../../models/builder/Builder/benchmark.md) — the
  single-strategy version.
- [`Plan::run`](../../validate/Plan/run.md) — cross-validate one fixed model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="strategy.html">
    <span class="label">Next</span>
    <span class="title">strategy</span>
  </a>
</div>
<!-- apiref:nav:end -->
