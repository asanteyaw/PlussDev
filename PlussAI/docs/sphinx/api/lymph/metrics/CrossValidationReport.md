<!-- pluss-hand-authored -->

# `lymph::metrics::CrossValidationReport`

[Back to `metrics`](../metrics.md)

`CrossValidationReport` holds the outcome of a cross-validation run: every
fold's assessment, the metric **mean** and **stdev** across folds, and a
`board()` table. It is what a validation [`Plan::run`](../selection/validate/Plan/run.md)
returns.

## Scope And Alias

The type is `lymph::metrics::CrossValidationReport`, re-exported at `lymph::`
scope with the umbrella alias `lymph::reports::CrossVal`. On `selection` pages it
is usually spelled `reports::CrossVal`; here in its home family the full name is
fine. All three name one type.

## Fields

`CrossAssessReport report`
: The per-fold detail. `report.folds` is one entry per fold (each with its own
  metric values and train / test index split); `report.metrics` is the metric
  order; `report.mean_metrics` / `report.std_metrics` are the raw aggregate
  `Series`.

`MetricReport mean`
: The across-fold **mean** of every requested metric, as a
  [`MetricReport`](MetricReport.md) — read `mean.accuracy.item<double>()`,
  `mean.f1`, `mean.r2`, and so on, or `mean.values` for all of them in
  `report.metrics` order.

`MetricReport stdev`
: The across-fold **standard deviation**, same shape as `mean`.

## Methods

### `board`

```cpp
pluss::table::DataFrame board() const;
```

A three-column table — `metric`, `mean`, `stdev` — one row per requested
metric, titled *Cross-Validation Results*. This is the surface you print or
export.

#### Example

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

### Reading `mean` / `stdev` by name

`mean` and `stdev` are [`MetricReport`](MetricReport.md)s. `report.folds` and
`report.metrics` describe the run.

#### Example

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

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options, {}, assess);

  std::cout << "folds        = " << report.report.folds.size() << "\n";
  std::cout << "metrics      = " << report.report.metrics.size() << "\n";
  std::cout << "mean.accuracy  = " << report.mean.accuracy.item<double>() << "\n";
  std::cout << "stdev.accuracy = " << report.stdev.accuracy.item<double>() << "\n";
  std::cout << "mean.f1        = " << report.mean.f1.item<double>() << "\n";
  std::cout << "stdev.f1       = " << report.stdev.f1.item<double>() << "\n";

  return 0;
}
```

Output

```text
folds        = 4
metrics      = 2
mean.accuracy  = 0.854167
stdev.accuracy = 0.14878
mean.f1        = 0.825
stdev.f1       = 0.204634
```

### Walking `mean.values` / `stdev.values`

`mean.values` and `stdev.values` are aligned to `report.metrics` — the row
order `board()` prints.

#### Example

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

  const pluss::Series step = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12.,
       13., 14., 15., 16., 17., 18., 19., 20.}, {20}, f64);
  const pluss::Series demand = pluss::make_series(
      {12., 13.5, 15., 16.6, 18., 19.4, 21., 22.5, 24., 25.4,
       27., 28.6, 30., 31.5, 33., 34.4, 36., 37.5, 39., 40.6}, {20}, f64);
  const DataFrame frame({{"step", Slate(step)}, {"demand", Slate(demand)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"step"};
  data_options.label_feature = "demand";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::R2, lymph::MetricKind::Mse, lymph::MetricKind::Mae};

  const lymph::reports::CrossVal report =
      lymph::selection::cross().folds(4).seed(20260901ULL)
          .run(lymph::models::linear_regression(), frame, data_options, {}, assess);

  const std::vector<double> means = report.mean.values.host<double>();
  const std::vector<double> stdevs = report.stdev.values.host<double>();
  for (std::size_t i = 0; i < report.report.metrics.size(); ++i) {
    std::cout << "metric[" << i << "] mean=" << means[i]
              << " stdev=" << stdevs[i] << "\n";
  }

  return 0;
}
```

Output

```text
metric[0] mean=0.999853 stdev=9.34929e-05
metric[1] mean=0.00410799 stdev=0.00136384
metric[2] mean=0.0451075 stdev=0.0115999
```

## Changelog

- **0.1** — `report` (per-fold `CrossAssessReport`), `mean` / `stdev`
  `MetricReport`s, and `board()`; umbrella alias `reports::CrossVal`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
