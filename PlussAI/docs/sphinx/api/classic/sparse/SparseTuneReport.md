# `pluss::ai::classic::SparseTuneReport`

Every trial [`tune_sparse`](tune_sparse.md)/[`sweep_sparse`](sweep_sparse.md)
ran, in candidate order, plus the derived summary that answers "which one
won": `best_index`, a 1-based rank per trial, and a columnar
`SparseTuneLedger` restating the same trials as parallel `pluss::Series` —
the sparse counterpart of dense [`TuneReport`](../tune/TuneReport.md), minus
anything tied to a search *strategy* (there is no `stage`, since every
candidate is always evaluated exactly once — see
[`tune_sparse`](tune_sparse.md)'s own note on having no strategy concept).

## Signature

```cpp
std::vector<classic::SparseTuneTrial> classic::SparseTuneReport::trials;
std::int64_t classic::SparseTuneReport::best_index;
classic::MetricKind classic::SparseTuneReport::objective_metric;
classic::SparseObjectivePolicy classic::SparseTuneReport::objective_policy;
std::vector<std::int64_t> classic::SparseTuneReport::objective_rank;
classic::SparseTuneLedger classic::SparseTuneReport::ledger;

// One entry of `trials`:
classic::LinearFamilyOptions classic::SparseTuneTrial::options;
classic::Status classic::SparseTuneTrial::status;
classic::SparseCrossReport classic::SparseTuneTrial::assess;
double classic::SparseTuneTrial::objective;

// `ledger`:
std::vector<classic::MetricKind> classic::SparseTuneLedger::metrics;
std::vector<std::string> classic::SparseTuneLedger::encoded_param_feature_names;
pluss::Series classic::SparseTuneLedger::objective;
pluss::Series classic::SparseTuneLedger::objective_rank;
pluss::Series classic::SparseTuneLedger::status_code;
pluss::Series classic::SparseTuneLedger::is_success;
pluss::Series classic::SparseTuneLedger::mean_test_metrics;
pluss::Series classic::SparseTuneLedger::std_test_metrics;
pluss::Series classic::SparseTuneLedger::split_test_metrics;
pluss::Series classic::SparseTuneLedger::fit_time_seconds;
pluss::Series classic::SparseTuneLedger::score_time_seconds;
pluss::Series classic::SparseTuneLedger::encoded_params;
```

## Construction

Produced only by
[`tune_sparse_binary`/`_regression`/`_multiclass`](tune_sparse.md) (and,
identically, [`sweep_sparse_*`](sweep_sparse.md)). A plain value type —
copy or store it freely.

## State

`trials` — `std::vector<SparseTuneTrial>`
: One entry per candidate, in the order `candidates` was given — always
  sequential, no concurrency option (unlike dense
  [`sweep`](../tune/sweep.md)'s `max_parallel`). Each `SparseTuneTrial`
  holds:

  `options` — `LinearFamilyOptions`
  : The exact candidate this trial evaluated.

  `status` — `Status`
  : `ok()` when this trial's own
    [`cross_assess_sparse_*`](cross_assess_sparse.md) call succeeded *and*
    `objective_metric` was found among its resolved metrics. A single bad
    candidate's non-ok `status` here does not affect any other trial — see
    [`tune_sparse`'s own error note](tune_sparse.md#errors).

  `assess` — `SparseCrossReport`
  : The full fold-by-fold result — default-constructed when `status` is
    not ok.

  `objective` — `double`
  : The value read out of `assess.mean_metrics` at `objective_metric`'s
    position. `NaN` when `status` is not ok.

`best_index` — `std::int64_t`
: The 0-based index into `trials` of the best trial — `-1` when no trial
  succeeded with a finite `objective`. Tracked incrementally exactly like
  dense [`TuneReport::best_index`](../tune/TuneReport.md#state): only a
  strictly better later trial replaces the incumbent, so a tie keeps
  whichever trial was found first.

`objective_metric` / `objective_policy`
: Copies of the arguments the producing call was given. `objective_policy`
  is **not** resolved — a call made with the default `Auto` still reports
  `Auto` here; `SparseObjectivePolicy` has the same three values
  (`Auto`/`Maximize`/`Minimize`) and `Auto` follows the same fixed
  per-metric maximize/minimize table as dense
  [`TuneReport::objective_policy`](../tune/TuneReport.md#state).

`objective_rank` — `std::vector<std::int64_t>`
: Parallel to `trials` — 1-based, standard-competition ranking (ties share
  a rank; the next distinct value's rank skips ahead by the tie's size).
  Every trial with `!status.ok()` or a non-finite `objective` shares one
  common worst rank (`count(finite trials) + 1`).

`ledger` — `SparseTuneLedger`
: The same `trials`, restated as parallel `pluss::Series` columns:

  `metrics` — `std::vector<MetricKind>` / `encoded_param_feature_names` —
  `std::vector<std::string>`
  : `encoded_param_feature_names` is always exactly six fixed names, in
    this order: `"linear.objective"`, `"linear.regularization"`,
    `"linear.regularization_strength_slog"`, `"linear.elastic_l1_ratio"`,
    `"linear.robust_transition_slog"`, `"linear.fit_intercept"` — every
    `LinearFamilyOptions` field, whether or not the estimator you tuned
    actually reads it (so, e.g., a `SparseLinearBinaryClassifier` sweep
    still gets a `"linear.elastic_l1_ratio"` column, even though that
    field is never read — see
    [`SparseLinearBinaryClassifier`'s own construction notes](SparseLinearBinaryClassifier.md#construction)
    for exactly which fields it reads).

  `encoded_params` — `pluss::Series`
  : `[trials.size(), 6]` — every candidate's six fields encoded to
    `double`: `objective`/`regularization`/`fit_intercept` as their raw
    enum/bool values; `regularization_strength`/`robust_transition` through
    a signed `log1p` (so `0` encodes to `0` and large magnitudes compress),
    named with the `_slog` suffix above; `elastic_l1_ratio` verbatim.

  `objective` / `objective_rank` / `status_code` / `is_success`
  : `Float64`/`Int64`/`Int64`/`Float64`, each length `trials.size()` — the
    same values as `trials[i].objective`, `objective_rank[i]`,
    `trials[i].status.code`, and `trials[i].status.ok() ? 1.0 : 0.0`.

  `mean_test_metrics` / `std_test_metrics`
  : `[trials.size(), metric_count]` — each trial's
    `assess.mean_metrics`/`assess.std_metrics`, stacked. `NaN`-filled for a
    failed trial or one that ran before any successful trial fixed
    `metric_count`.

  `split_test_metrics` / `fit_time_seconds` / `score_time_seconds`
  : `[trials.size(), max_folds, metric_count]` /
    `[trials.size(), max_folds]` / `[trials.size(), max_folds]` —
    `max_folds` is the largest fold count seen across all trials; a trial
    with fewer folds (or none, on failure) is `NaN`-padded on the right.

## Methods

None — `SparseTuneReport` is a plain data holder, read directly.

## Practical Example

A 4-candidate `regularization_strength` sweep. Three mild-to-moderate
strengths (`1e-4`, `1e-2`, `1.0`) all reach perfect accuracy and tie for
`rank = 1`; the fourth (`100.0`) is strong enough that, combined with this
family's fixed `0.05` learning rate (see
[`SparseLinearBinaryClassifier`'s Construction section](SparseLinearBinaryClassifier.md#construction)),
the per-step update overshoots and diverges — that trial's own `status`
comes back non-ok with a "non-finite objective" message, its `objective`
is `NaN`, and it lands alone in the shared worst-rank bucket (`rank = 4`).
The ledger's encoded-params column is then read back directly.

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
  std::cout << "best_index = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << ": objective=" << report.trials[i].objective
              << " rank=" << report.objective_rank[i] << "\n";
  }
  std::cout << "ledger.encoded_param_feature_names[2] = " << report.ledger.encoded_param_feature_names[2] << "\n";
  std::cout << "ledger.encoded_params = " << report.ledger.encoded_params << "\n";

  return 0;
}
```

Output

```text
best_index = 0
trial 0: objective=1 rank=1
trial 1: objective=1 rank=1
trial 2: objective=1 rank=1
trial 3: objective=nan rank=4
ledger.encoded_param_feature_names[2] = linear.regularization_strength_slog
ledger.encoded_params = pluss::Series(shape=[4,6], strides=[6,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data=[[0, 2, ... , 1],
  [0, 2, ... , 1],
  [0, 2, ... , 1],
  [0, 2, ... , 1]]
```

## Changelog

- **0.1** — `trials` (each a `SparseTuneTrial`), `best_index`,
  `objective_metric`, `objective_policy`, `objective_rank`,
  `ledger` (`SparseTuneLedger`).

## See Also

- [`tune_sparse`](tune_sparse.md) / [`sweep_sparse`](sweep_sparse.md) — the
  producing functions.
- [`SparseCrossReport`](SparseCrossReport.md) — what each trial's own
  `assess` field holds.
- [`TuneReport`](../tune/TuneReport.md) — the dense counterpart, with a
  `stage` field this report has no equivalent for.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
