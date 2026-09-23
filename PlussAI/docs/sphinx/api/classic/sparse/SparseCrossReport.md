# `pluss::ai::classic::SparseCrossReport`

Every fold's metric values from a
[`cross_assess_sparse_*`](cross_assess_sparse.md) run — **both** the test
side and, uniquely among the cross-validation reports in this reference,
the train side too, since every fold is scored on its own training rows a
second time. The mean/std summary is provided for both.

## Signature

```cpp
std::vector<classic::SparseFoldReport> classic::SparseCrossReport::folds;
std::vector<classic::MetricKind> classic::SparseCrossReport::metrics;
pluss::Series classic::SparseCrossReport::mean_metrics;
pluss::Series classic::SparseCrossReport::std_metrics;
pluss::Series classic::SparseCrossReport::mean_train_metrics;
pluss::Series classic::SparseCrossReport::std_train_metrics;
pluss::Series classic::SparseCrossReport::fit_time_seconds;
pluss::Series classic::SparseCrossReport::score_time_seconds;

// One entry of `folds`:
pluss::Series classic::SparseFoldReport::metrics;
pluss::Series classic::SparseFoldReport::train_metrics;
pluss::Series classic::SparseFoldReport::train_indices;
pluss::Series classic::SparseFoldReport::test_indices;
double classic::SparseFoldReport::fit_time_seconds;
double classic::SparseFoldReport::score_time_seconds;
```

## Construction

Produced only by [`cross_assess_sparse_binary`/`_regression`/`_multiclass`](cross_assess_sparse.md)
(and, internally, by [`tune_sparse`](tune_sparse.md)/[`sweep_sparse`](sweep_sparse.md),
which run one per candidate). A plain value type — copy or store it freely.

## State

`folds` — `std::vector<SparseFoldReport>`
: One entry per fold, in fold-plan order — folds always run sequentially
  for the sparse learners (no `max_parallel` concept; see
  [`cross_assess_sparse`'s `split`](cross_assess_sparse.md#parameters)).
  Each `SparseFoldReport` holds:

  `metrics` — `pluss::Series`
  : `Float64`, length `metrics.size()` (the outer field below) — this
    fold's metric values scored on its **test** rows.

  `train_metrics` — `pluss::Series`
  : `Float64`, same length and metric order — the same metrics scored on
    this fold's own **training** rows instead. A wide gap between
    `train_metrics` and `metrics` on the same fold is the direct signal
    of overfitting; see the [Practical Example](#practical-example) below.

  `train_indices` / `test_indices` — `pluss::Series`
  : `Int64` — the exact row split this fold trained and scored on.

  `fit_time_seconds` / `score_time_seconds` — `double`
  : Wall-clock seconds for that fold's `learn()` and its **test-side**
    `assess()` call only — the extra train-side `assess()` call's own time
    is not tracked separately.

`metrics` — `std::vector<MetricKind>`
: Which metrics were requested — `AssessOptions::metrics` as given, or the
  task's default (`{Accuracy}` for binary/multiclass, `{Mse, Rmse, R2}` for
  regression) when that was empty.

`mean_metrics` / `std_metrics` — `pluss::Series`
: `Float64`, length `metrics.size()` — across-fold mean and population
  standard deviation of the **test**-side metrics.

`mean_train_metrics` / `std_train_metrics` — `pluss::Series`
: The same, for the **train**-side metrics.

`fit_time_seconds` / `score_time_seconds` — `pluss::Series`
: `Float64`, length `folds.size()` — every fold's own timing, gathered into
  one Series (unlike [`CrossAssessReport`](../metrics/CrossAssessReport.md),
  which leaves timing inside each `FoldAssessResult` only).

## Methods

None — `SparseCrossReport` is a plain data holder, read directly.

## Practical Example

A 3-fold cross-assessment on cleanly linearly-separable data, so train and
test accuracy match exactly in every fold — read out fold by fold
alongside both mean/std summaries. See
[`SparseTuneReport`'s own example](SparseTuneReport.md) for a case where a
trial's fit fails outright instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      12, 2,
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {12}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::SparseCrossReport report = cls::cross_assess_sparse_binary(x, y, split).value();
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i
              << ": test=" << report.folds[i].metrics
              << " train=" << report.folds[i].train_metrics << "\n";
  }
  std::cout << "mean test  = " << report.mean_metrics << "\n";
  std::cout << "mean train = " << report.mean_train_metrics << "\n";

  return 0;
}
```

Output

```text
fold 0: test=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] train=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
fold 1: test=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] train=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
fold 2: test=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] train=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
mean test  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
mean train = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

## Changelog

- **0.1** — `folds` (each a `SparseFoldReport`, with `train_metrics`),
  `metrics`, `mean_metrics`/`std_metrics`, `mean_train_metrics`/
  `std_train_metrics`, `fit_time_seconds`/`score_time_seconds`.

## See Also

- [`cross_assess_sparse`](cross_assess_sparse.md) — the producing functions.
- [`SparseTuneReport`](SparseTuneReport.md) — wraps one `SparseCrossReport`
  per trial.
- [`CrossAssessReport`](../metrics/CrossAssessReport.md) — the dense
  counterpart (test-side metrics only).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
