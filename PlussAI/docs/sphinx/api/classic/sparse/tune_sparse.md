# `pluss::ai::classic::tune_sparse_binary` / `tune_sparse_regression` / `tune_sparse_multiclass`

Rank a list of [`LinearFamilyOptions`](../families/LinearFamilyOptions.md)
candidates for a sparse task by cross-validated score — one function per
task, each running [`cross_assess_sparse_*`](cross_assess_sparse.md) once
per candidate and returning a [`SparseTuneReport`](SparseTuneReport.md).
Unlike dense [`tune`](../tune/tune.md), there is **no search-strategy
concept** here at all — no `TuneOptions`, no `SearchKind`, no budgeted
subset, no successive halving, no Bayesian search. Every candidate you pass
is evaluated, every time. `sweep_sparse_binary`/`_regression`/`_multiclass`
([`sweep_sparse`](sweep_sparse.md)) call these three functions directly with
the same arguments and return their result unchanged — the two names are
presently synonyms, a real, honest limitation: reach for whichever name
reads better at the call site, since there is currently nothing that
distinguishes them.

## Signature

```cpp
classic::StatusOr<classic::SparseTuneReport> classic::tune_sparse_binary(
    /* a range of LinearFamilyOptions -- pass a std::vector<LinearFamilyOptions> or braced list */ candidates,
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::SparseObjectivePolicy objective_policy = classic::SparseObjectivePolicy::Auto);

classic::StatusOr<classic::SparseTuneReport> classic::tune_sparse_regression(
    /* a range of LinearFamilyOptions */ candidates,
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Mse,
    classic::SparseObjectivePolicy objective_policy = classic::SparseObjectivePolicy::Auto);

classic::StatusOr<classic::SparseTuneReport> classic::tune_sparse_multiclass(
    /* a range of LinearFamilyOptions */ candidates,
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::SparseObjectivePolicy objective_policy = classic::SparseObjectivePolicy::Auto);
```

Each of the three also has a second overload taking
`std::initializer_list<LinearFamilyOptions>` in place of the range — the
common case when candidates are spelled directly as a braced `{...}` list at
the call site; it simply forwards to the first overload.

## Parameters

`candidates`
: At least one `LinearFamilyOptions`, each evaluated as its own trial. Note
  the default-constructed `LinearFamilyOptions{}` is a perfectly valid
  candidate for every task here — unlike dense `tune`'s `ModelSpec`
  candidates, there is no `objective` mismatch to trip over, since
  `tune_sparse_binary`/`_multiclass` always fit `Logistic` and
  `tune_sparse_regression` always fits `LeastSquares` regardless of what
  `candidates[i].objective` says (the same ignored-field contract as each
  estimator's own `learn` — see
  [`SparseLinearBinaryClassifier`'s construction](SparseLinearBinaryClassifier.md#construction)).

`x` / `y` / `split` / `train_options` / `assess_options`
: Forwarded unchanged to every candidate's own
  [`cross_assess_sparse_*`](cross_assess_sparse.md#parameters) call — same
  contract, same errors.

`objective_metric` (`MetricKind`, default `Accuracy` for binary/multiclass,
`Mse` for regression)
: Which metric in the resolved `assess_options.metrics` set to rank
  candidates by. Must appear among the metrics that ended up in each
  trial's `SparseCrossReport::metrics` (the requested set, or the task
  default when `assess_options.metrics` was empty) — see
  [Example 4](#example-4) for what happens when it does not.

`objective_policy` (`SparseObjectivePolicy`, default `Auto`)
: `Auto` derives ranking direction from `objective_metric` via the same
  fixed maximize/minimize table
  [`TuneReport::objective_policy`](../tune/TuneReport.md#state) documents
  for dense `tune`. `Maximize`/`Minimize` force a direction regardless of
  metric.

## Returns

A `StatusOr<SparseTuneReport>` — see [`SparseTuneReport`](SparseTuneReport.md).

## Errors

- `candidates` empty — `Status::invalid_argument`, checked before any
  candidate is evaluated.
- Any single candidate's own `cross_assess_sparse_*` call failing does
  **not** fail the whole `tune_sparse_*` call — that candidate's trial
  records the failing `Status` and a `NaN` objective, and evaluation
  continues with the next candidate (contrast
  [`cross_assess_sparse`'s own fold-failure behavior](cross_assess_sparse.md#errors),
  which aborts the whole call — a single bad `LinearFamilyOptions` here
  only knocks out its own trial).
- `objective_metric` not present among a (successful) trial's resolved
  metrics — that trial's own `Status::invalid_argument`, again isolated to
  the one trial, not the whole call.

## Practical Examples

A baseline regularization-strength sweep for each task, `objective_policy`
forcing a direction opposite `Auto`'s own table, and the isolated-trial
failure/metric-mismatch behavior.

### Example 1

`tune_sparse_binary` ranking four `regularization_strength` values by
`Accuracy` (the default objective for this task).

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

  std::vector<cls::LinearFamilyOptions> candidates{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.regularization = cls::RegularizationKind::L2;
    opts.regularization_strength = strength;
    candidates.push_back(opts);
  }

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::SparseTuneReport report = cls::tune_sparse_binary(candidates, x, y, split).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  std::cout << "best_index  = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
trial count = 4
best_index  = 0
```

### Example 2

The braced-list overload — candidates spelled directly, no intermediate
`std::vector`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::vector<std::int64_t> rows_v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      10, 1, rows_v, std::vector<std::int64_t>(10, 0),
      {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0},
      {10}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 5;
  split.random.seed = 0;

  cls::LinearFamilyOptions none{};
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 0.5;

  const cls::SparseTuneReport report =
      cls::tune_sparse_regression({none, l2}, x, y, split, {}, {}, cls::MetricKind::Mse).value();
  std::cout << "best_index (lower Mse wins) = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << " objective(Mse) = " << report.trials[i].objective << "\n";
  }

  return 0;
}
```

Output

```text
best_index (lower Mse wins) = 0
trial 0 objective(Mse) = 0.000583472
trial 1 objective(Mse) = 0.267237
```

### Example 3

`tune_sparse_multiclass` with `objective_policy = Maximize` forced
explicitly, even though `Accuracy`'s `Auto` direction is already maximize —
confirms passing the same direction `Auto` would have picked changes
nothing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      9, 3,
      {0, 1, 2, 3, 4, 5, 6, 7, 8},
      {0, 0, 0, 1, 1, 1, 2, 2, 2},
      {1.0, 2.0, 3.0, 1.0, 2.0, 3.0, 1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0},
      {9}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::StratifiedCrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::LinearFamilyOptions none{};
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;

  const cls::SparseTuneReport auto_policy =
      cls::tune_sparse_multiclass({none, l2}, x, y, split, {}, {}, cls::MetricKind::Accuracy).value();
  const cls::SparseTuneReport forced_max =
      cls::tune_sparse_multiclass({none, l2}, x, y, split, {}, {}, cls::MetricKind::Accuracy,
                                  cls::SparseObjectivePolicy::Maximize)
          .value();
  std::cout << "best_index matches = " << std::boolalpha << (auto_policy.best_index == forced_max.best_index) << "\n";

  return 0;
}
```

Output

```text
best_index matches = true
```

### Example 4

Two failure modes, both isolated to their own trial rather than failing the
whole call: one candidate whose `LinearFamilyOptions` is otherwise fine but
whose `TrainOptions` (forwarded to every candidate identically here) is
invalid, and a request for `objective_metric = RocAuc`, which
`SparseLinearBinaryClassifier::assess` does not support.

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

  // objective_metric not among the resolved (default) {Accuracy} set --
  // every trial fails identically, but the call itself still succeeds.
  const cls::SparseTuneReport report =
      cls::tune_sparse_binary({cls::LinearFamilyOptions{}}, x, y, split, {}, {}, cls::MetricKind::RocAuc).value();
  std::cout << "call ok, trial[0].status.ok() = " << std::boolalpha << report.trials[0].status.ok() << "\n";
  std::cout << "trial[0].objective (NaN)      = " << report.trials[0].objective << "\n";
  std::cout << "best_index (-1, nothing valid) = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
call ok, trial[0].status.ok() = false
trial[0].objective (NaN)      = nan
best_index (-1, nothing valid) = -1
```

## Changelog

- **0.1** — `tune_sparse_binary`/`_regression`/`_multiclass`, each with a
  range-based and an initializer-list overload.

## See Also

- [`sweep_sparse`](sweep_sparse.md) — presently identical to this function.
- [`SparseTuneReport`](SparseTuneReport.md) — the returned type.
- [`cross_assess_sparse`](cross_assess_sparse.md) — run once per candidate.
- [`tune`](../tune/tune.md) — the dense counterpart, with real
  search-strategy dispatch this family does not have.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
