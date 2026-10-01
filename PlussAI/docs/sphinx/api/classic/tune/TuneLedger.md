# `pluss::ai::classic::TuneLedger`

[`TuneReport::trials`](TuneReport.md#state) restated as parallel
`pluss::Series` columns, one row per trial — for feeding results into
further `Series`-based analysis (sorting, filtering, plotting) instead of
looping over a `std::vector<TuneTrialResult>` by hand. Every field lines up
by row index with `TuneReport.trials` and with every other `TuneLedger`
field; row `i` here is trial `i` there.

## Signature

```cpp
std::vector<classic::MetricKind> classic::TuneLedger::metrics;
std::vector<std::string> classic::TuneLedger::encoded_param_feature_names;
pluss::Series classic::TuneLedger::objective;
pluss::Series classic::TuneLedger::objective_rank;
pluss::Series classic::TuneLedger::stage;
pluss::Series classic::TuneLedger::status_code;
pluss::Series classic::TuneLedger::is_success;
pluss::Series classic::TuneLedger::mean_test_metrics;
pluss::Series classic::TuneLedger::std_test_metrics;
pluss::Series classic::TuneLedger::split_test_metrics;
pluss::Series classic::TuneLedger::fit_time_seconds;
pluss::Series classic::TuneLedger::score_time_seconds;
pluss::Series classic::TuneLedger::encoded_params;
```

## Construction

Produced only as [`TuneReport::ledger`](TuneReport.md#state) — every
[`sweep`](sweep.md) / [`probe`](probe.md) / [`tune`](tune.md) call builds
one alongside its `trials`. Never constructed directly; a default
`TuneLedger{}` (every `Series` field `pluss::series::undefined()`, every
`std::vector` empty) only appears as `TuneReport.ledger` when `trials` is
itself empty.

## State

Every `Series` field below has one row per trial (`trials.size()`), in the
same order as `TuneReport.trials`.

`metrics` — `std::vector<MetricKind>`
: A copy of the first successful trial's `assess.metrics` — the metric
  each column of `mean_test_metrics` / `std_test_metrics` /
  `split_test_metrics` corresponds to, in order. Empty when no trial ever
  produced any metrics (every trial failed before reaching `assess`).

`encoded_param_feature_names` — `std::vector<std::string>`
: A fixed list of 40 column names, always the same regardless of which
  families appear in `trials` — what each column of `encoded_params`
  means. See `encoded_params` below for the full table.

`objective` — `pluss::Series`
: `Float64`, `[n]` — `trials[i].objective` for each row, `NaN` for a
  failed trial.

`objective_rank` — `pluss::Series`
: `Int64`, `[n]` — a copy of `TuneReport.objective_rank`.

`stage` — `pluss::Series`
: `Int64`, `[n]` — a copy of each trial's own `stage` (see
  [`TuneReport`](TuneReport.md#state) for what it means per `search`
  value).

`status_code` — `pluss::Series`
: `Int64`, `[n]` — each trial's `status.code` (an integer
  [`Status`](../core/Status.md) code; `0` is always success, matching
  `is_success` below).

`is_success` — `pluss::Series`
: `Float64`, `[n]` — `1.0` where `trials[i].status.ok()`, else `0.0`.

`mean_test_metrics` — `pluss::Series`
: `Float64`, `[n, metrics.size()]` — a copy of each successful trial's
  `assess.mean_metrics`; a failed trial's row is all `NaN`. Left
  `pluss::series::undefined()` (not a zero-row `Series`) when `metrics` is empty.

`std_test_metrics` — `pluss::Series`
: `Float64`, `[n, metrics.size()]` — the same shape, each successful
  trial's `assess.std_metrics`.

`split_test_metrics` — `pluss::Series`
: `Float64`, `[n, max_folds, metrics.size()]`, where `max_folds` is the
  largest `assess.folds.size()` seen across every successful trial — each
  successful trial's own per-fold metric values, one slab per trial.
  `pluss::series::undefined()` when no successful trial produced any folds.

`fit_time_seconds` / `score_time_seconds` — `pluss::Series`
: `Float64`, `[n, max_folds]` — each successful trial's own per-fold
  [`FoldAssessResult::fit_time_seconds` /
  `score_time_seconds`](../metrics/CrossAssessReport.md#state) (real
  wall-clock seconds — varies between runs, never reproducible).
  `pluss::series::undefined()` under the same condition as `split_test_metrics`.

`encoded_params` — `pluss::Series`
: `Float64`, `[n, 40]` — every trial's `spec`, turned into a fixed-width
  numeric row (built purely from `spec`, so even a failed trial gets a
  real row). Only the columns belonging to `spec.family`'s own
  `*FamilyOptions` are written; every other family's columns are `0.0`.
  A column suffixed `_slog` holds `sign(x) * log1p(|x|)` of the named
  field instead of its raw value — compressing a field that can span
  several orders of magnitude (a learner count, a tolerance, a
  regularization strength) onto a comparable numeric scale while
  preserving its sign and its distance from `0`. `task`/`family` and every
  boolean/enum field are stored as a plain cast to `double` (an enum's
  numeric ordinal, or `1.0`/`0.0` for a `bool`). `family = Detect` is
  never actually reachable through `sweep`/`probe`/`tune` (they require
  `TaskKind::Classification`/`Regression`, and no `Detect`-family spec
  satisfies either — see [`ModelSpec`](../families/ModelSpec.md#errors)),
  so no column is dedicated to it.

  | # | Column | Family | Source field |
  | - | --- | --- | --- |
  | 0 | `task` | — | `spec.task` |
  | 1 | `family` | — | `spec.family` |
  | 2 | `linear.objective` | Linear | `LinearFamilyOptions::objective` |
  | 3 | `linear.regularization` | Linear | `::regularization` |
  | 4 | `linear.regularization_strength_slog` | Linear | `::regularization_strength` |
  | 5 | `linear.fit_intercept` | Linear | `::fit_intercept` |
  | 6 | `tree.criterion` | Tree | `TreeFamilyOptions::criterion` |
  | 7 | `tree.max_depth` | Tree | `::max_depth` |
  | 8 | `tree.min_samples_leaf_slog` | Tree | `::min_samples_leaf` |
  | 9 | `tree.min_samples_split_slog` | Tree | `::min_samples_split` |
  | 10 | `ensemble.method` | Ensemble | `EnsembleFamilyOptions::method` |
  | 11 | `ensemble.learners_slog` | Ensemble | `::learners` |
  | 12 | `ensemble.subsample` | Ensemble | `::subsample` |
  | 13 | `neighbor.k_slog` | Neighbor | `NeighborFamilyOptions::k` |
  | 14 | `neighbor.distance_weighted` | Neighbor | `::distance_weighted` |
  | 15 | `neighbor.standardize` | Neighbor | `::standardize` |
  | 16 | `kernel.kind` | Kernel | `KernelFamilyOptions::kernel` |
  | 17 | `kernel.c_slog` | Kernel | `::c` |
  | 18 | `kernel.gamma_slog` | Kernel | `::gamma` |
  | 19 | `kernel.epsilon_slog` | Kernel | `::epsilon` |
  | 20 | `bayes.use_gaussian` | Bayes | `BayesFamilyOptions::use_gaussian` |
  | 21 | `bayes.smoothing_slog` | Bayes | `::smoothing` |
  | 22 | `cluster.method` | Cluster | `ClusterFamilyOptions::method` |
  | 23 | `cluster.clusters_slog` | Cluster | `::clusters` |
  | 24 | `cluster.max_iterations_slog` | Cluster | `::max_iterations` |
  | 25 | `cluster.restarts_slog` | Cluster | `::restarts` |
  | 26 | `cluster.tolerance_slog` | Cluster | `::tolerance` |
  | 27 | `decompose.method` | Decompose | `DecomposeFamilyOptions::method` |
  | 28 | `decompose.components_slog` | Decompose | `::components` |
  | 29 | `decompose.whiten` | Decompose | `::whiten` |
  | 30 | `decompose.max_iterations_slog` | Decompose | `::max_iterations` |
  | 31 | `decompose.tolerance_slog` | Decompose | `::tolerance` |
  | 32 | `linear.elastic_l1_ratio` | Linear | `LinearFamilyOptions::elastic_l1_ratio` |
  | 33 | `linear.robust_transition_slog` | Linear | `::robust_transition` |
  | 34 | `neighbor.query_mode` | Neighbor | `NeighborFamilyOptions::query_mode` |
  | 35 | `neighbor.radius_slog` | Neighbor | `::radius` |
  | 36 | `neighbor.radius_fallback_k_slog` | Neighbor | `::radius_fallback_k` |
  | 37 | `kernel.form` | Kernel | `KernelFamilyOptions::form` |
  | 38 | `kernel.nu` | Kernel | `::nu` |
  | 39 | `bayes.likelihood` | Bayes | `BayesFamilyOptions::likelihood` |

## Methods

None — `TuneLedger` is a plain data holder, read directly.

## Practical Example

A 3-candidate sweep, read back as ledger columns: `objective`/
`objective_rank`/`is_success` alongside `encoded_params`'s per-trial
feature row.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});
  const cls::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0}, {10}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions low_reg{};
  low_reg.regularization_strength = 1e-4;
  cls::LinearFamilyOptions high_reg{};
  high_reg.regularization_strength = 10.0;

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, low_reg},
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, high_reg},
  };

  const cls::TuneReport report =
      cls::sweep(candidates, batch, {}, {}, {}, cls::MetricKind::R2).value();
  const cls::TuneLedger& ledger = report.ledger;
  std::cout << "objective      = " << ledger.objective << "\n";
  std::cout << "objective_rank = " << ledger.objective_rank << "\n";
  std::cout << "is_success     = " << ledger.is_success << "\n";
  std::cout << "encoded_params = " << ledger.encoded_params << "\n";

  return 0;
}
```

Output

```text
objective      = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 0.98088136]
objective_rank = pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
is_success     = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
encoded_params = pluss::Series(shape=[3,40], strides=[40,1], dtype=float64, platform=cpu, numel=120, contiguous=true)
data=[[1, 0, ... , 0],
  [1, 0, ... , 0],
  [1, 0, ... , 0]]
```

## Changelog

- **0.1** — `metrics`, `encoded_param_feature_names`, `objective`,
  `objective_rank`, `stage`, `status_code`, `is_success`,
  `mean_test_metrics`, `std_test_metrics`, `split_test_metrics`,
  `fit_time_seconds`, `score_time_seconds`, `encoded_params`.

## See Also

- [`TuneReport`](TuneReport.md) — the owning report; `ledger` is one of
  its fields.
- [`CrossAssessReport`](../metrics/CrossAssessReport.md) — where
  `mean_test_metrics`/`std_test_metrics`/`split_test_metrics`/the timing
  fields are copied from.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
