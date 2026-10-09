<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 88faa97ec836 -->

# `lymph::selection::spotcheck::Suite::strategy`

[Back to `Suite`](../Suite.md)

`strategy` registers one **named training strategy** on the suite. Each call
stacks another; [`run`](run.md) then forms one candidate per strategy × model.
Calls chain — each returns `*this`.

## Signature

```cpp
Suite& strategy(std::string name, TrainOptions options = {});
Suite& strategy(std::string name, BalanceTarget balance);
Suite& strategy(std::string name, BalanceTarget balance, TrainOptions options);
```

- **`(name, options)`** — the general form. `options` is the `TrainOptions`
  every model runs under for this strategy: iteration budget, class weights,
  calibration, a balance target, and so on.
- **`(name, balance)`** — shorthand for a strategy whose only change is class
  balancing. Equivalent to `(name, TrainOptions{.balance = balance})`.
- **`(name, balance, options)`** — both: `options` with its `.balance` field
  overwritten by `balance`.

## What It Does

Appends a `{name, TrainOptions}` entry to the suite's strategy list. Nothing is
validated against the models yet. At [`run`](run.md) time each strategy is
paired with each model from the `models::Builder`, producing a candidate named
`"<strategy>__<model>"`.

`name` must be non-empty (raises otherwise). Names are yours to choose — they
are what shows up in the ranked board, so keep them short and comparable
(`"plain"`, `"balanced"`, `"calibrated"`).

## Parameters

`name` — `std::string`
: The strategy label. Non-empty; appears in every candidate name it produces.

`options` — `lymph::options::Train`
: The training configuration for this strategy. Defaulted in the first
  overload.

`balance` — `lymph::BalanceTarget`
: A class-balancing target — `{kind, ratio, growth, …}`. `kind` is `Keep`
  (default), `Equalize`, `Ratio`, or `Counts`.

## Returns

`Suite&` — the same suite, for chaining.

## Practical Examples

<!-- these examples establish: stacking TrainOptions strategies, the BalanceTarget shorthand, and the combined overload. -->

### Example 1

Two `TrainOptions` strategies over two models — four trials, named
`strategy__model`.

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

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::TrainOptions calibrated{};
  calibrated.calibration = lymph::ScoreCalibration::Platt;

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .strategy("calibrated", calibrated)
          .run(frame, data_options, cv, lymph::AssessOptions{},
               lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{});

  std::cout << "trials = " << ranked.report.trials.size() << "\n";
  for (const auto& trial : ranked.report.trials) {
    std::cout << "  " << trial.name << "\n";
  }

  return 0;
}
```

Output

```text
trials = 4
  plain__bayes
  plain__tree
  calibrated__bayes
  calibrated__tree
```

### Example 2

The `BalanceTarget` shorthand — `plain` vs. minority-class `equalized` on an
imbalanced label (12 negatives, 4 positives), ranked by F1.

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

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 2., 3., 3., 4., 4., 5., 5., 6., 6., 7., 12., 13., 14., 15.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::BalanceTarget equalize{};
  equalize.kind = lymph::BalanceTargetKind::Equalize;

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .strategy("equalized", equalize)
          .run(frame, data_options, cv, lymph::AssessOptions{},
               lymph::MetricKind::F1, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
```

Output

```text
trials = 4, best = plain__bayes

==============================================
     | name              objective  rank  f1  
----------------------------------------------
0    | plain__bayes      1          1     1   
1    | plain__tree       0.9        3     0.9 
2    | equalized__bayes  1          1     1   
3    | equalized__tree   0.9        3     0.9 
----------------------------------------------
[4 cs x 4 feat]
```

### Example 3

`strategy(name, balance, options)` sets both. Three strategies × two models —
six trials in one `run`.

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

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 2., 3., 3., 4., 4., 5., 5., 6., 6., 7., 12., 13., 14., 15.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::BalanceTarget equalize{};
  equalize.kind = lymph::BalanceTargetKind::Equalize;

  lymph::TrainOptions calibrated{};
  calibrated.calibration = lymph::ScoreCalibration::Platt;

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .strategy("equalized", equalize)
          .strategy("equalized_calibrated", equalize, calibrated)
          .run(frame, data_options, cv, lymph::AssessOptions{},
               lymph::MetricKind::F1, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{});

  std::cout << "strategies x models = " << ranked.report.trials.size() << "\n";
  for (const auto& trial : ranked.report.trials) {
    std::cout << "  " << trial.name << "\n";
  }

  return 0;
}
```

Output

```text
strategies x models = 6
  plain__bayes
  plain__tree
  equalized__bayes
  equalized__tree
  equalized_calibrated__bayes
  equalized_calibrated__tree
```

## See Also

- [`run`](run.md) — cross-validate the strategy × model grid.
- [`Suite`](../Suite.md) — construction via `suite(builder)`.
- `lymph::options::Train` — the `TrainOptions` each strategy carries.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
