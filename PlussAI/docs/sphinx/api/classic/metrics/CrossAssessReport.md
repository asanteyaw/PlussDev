# `pluss::ai::classic::CrossAssessReport`

Every fold's metric values from a [`cross_assess`](cross_assess.md) run,
plus the mean and standard deviation across folds — the full record behind
`cross_assess`'s "how reliable is this model choice" answer, not just its
summary numbers.

## Signature

```cpp
std::vector<classic::FoldAssessResult> classic::CrossAssessReport::folds;
std::vector<classic::MetricKind> classic::CrossAssessReport::metrics;
pluss::Series classic::CrossAssessReport::mean_metrics;
pluss::Series classic::CrossAssessReport::std_metrics;

// One entry of `folds`:
pluss::Series classic::FoldAssessResult::metrics;
pluss::Series classic::FoldAssessResult::train_indices;
pluss::Series classic::FoldAssessResult::test_indices;
double classic::FoldAssessResult::fit_time_seconds;
double classic::FoldAssessResult::score_time_seconds;
```

## Construction

Produced only by [`cross_assess`](cross_assess.md). A plain value type —
copy or store it freely.

## State

`folds` — `std::vector<FoldAssessResult>`
: One entry per fold, in the [`SplitPlan`](../split/SplitPlan.md) order
  `cross_assess` evaluated them (fold order is preserved even when folds
  ran in parallel — see [`cross_assess`](cross_assess.md#parameters)'s
  `max_parallel`). Each `FoldAssessResult` holds that fold's own:

  `metrics` — `pluss::Series`
  : `Float64`, length `metrics.size()` (the outer field below) — this
    fold's own metric values, in the same order.

  `train_indices` / `test_indices` — `pluss::Series`
  : `Int64` — the exact [`IndexSplit`](../split/IndexSplit.md) this fold was
    fit and scored on, carried along so a fold's result can be traced back
    to the rows that produced it.

  `fit_time_seconds` / `score_time_seconds` — `double`
  : Wall-clock seconds spent in that fold's own `learn()` call and its
    `assess()` call, timed separately. Real measured time — it varies
    between runs and between machines, so treat it as a rough cost signal
    (which fold, or which candidate in [`tune`](../tune.md), was
    noticeably slower to fit or to score), never as a reproducible value.

`metrics` — `std::vector<MetricKind>`
: Which metrics were requested — either `AssessOptions::metrics` as given,
  or the task's default set (`{Accuracy}` for classification, `{Mse, Rmse,
  R2}` for regression) when that was empty. Aligned with every fold's own
  `metrics` field and with `mean_metrics`/`std_metrics`.

`mean_metrics` — `pluss::Series`
: `Float64`, length `metrics.size()` — the across-fold mean of each metric.

`std_metrics` — `pluss::Series`
: `Float64`, length `metrics.size()` — the across-fold **population**
  standard deviation of each metric (divided by the fold count, not
  `fold count - 1`).

## Methods

None — `CrossAssessReport` is a plain data holder, read directly.

## Practical Example

A 3-fold cross-validation run, its per-fold accuracy values, and the
mean/std summary.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 2.0, 2.1, 2.2, 2.3, 3.0, 3.1, 3.2, 3.3}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::CrossAssessReport report =
      cls::cross_assess(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear},
                        batch, split).value();

  std::cout << "fold count = " << report.folds.size() << "\n";
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i << " accuracy = " << report.folds[i].metrics << "\n";
  }
  std::cout << "mean = " << report.mean_metrics << "\n";
  std::cout << "std  = " << report.std_metrics << "\n";

  return 0;
}
```

Output

```text
fold count = 3
fold 0 accuracy = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
fold 1 accuracy = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
fold 2 accuracy = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
mean = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
std  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0]
```

## Changelog

- **0.1** — `folds` (each a `FoldAssessResult`), `metrics`, `mean_metrics`,
  `std_metrics`.

## See Also

- [`cross_assess`](cross_assess.md) — the producing function.
- [`SplitPlan`](../split/SplitPlan.md) / [`IndexSplit`](../split/IndexSplit.md) —
  what each fold's `train_indices`/`test_indices` come from.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
