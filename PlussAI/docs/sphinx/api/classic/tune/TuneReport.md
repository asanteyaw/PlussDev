# `pluss::ai::classic::TuneReport`

Every trial [`sweep`](sweep.md) / [`probe`](probe.md) / [`tune`](tune.md)
ran, in order, plus the derived summary that answers "which one won":
`best_index`, a 1-based rank per trial, and a columnar
[`TuneLedger`](TuneLedger.md) restating the same trials as parallel
`pluss::Series`.

## Signature

```cpp
std::vector<classic::TuneTrialResult> classic::TuneReport::trials;
std::int64_t classic::TuneReport::best_index;
classic::MetricKind classic::TuneReport::objective_metric;
classic::TuneObjectivePolicy classic::TuneReport::objective_policy;
std::vector<std::int64_t> classic::TuneReport::objective_rank;
classic::TuneLedger classic::TuneReport::ledger;

// One entry of `trials`:
classic::ModelSpec classic::TuneTrialResult::spec;
classic::Status classic::TuneTrialResult::status;
classic::CrossAssessReport classic::TuneTrialResult::assess;
double classic::TuneTrialResult::objective;
std::int64_t classic::TuneTrialResult::stage;
```

## Construction

Produced only by [`sweep`](sweep.md) / [`probe`](probe.md) /
[`tune`](tune.md). A plain value type — copy or store it freely.

## State

`trials` — `std::vector<TuneTrialResult>`
: One entry per trial, in the order it was evaluated (preserved even when
  trials ran concurrently — see [`sweep`](sweep.md#parameters) /
  [`probe`](probe.md#parameters)'s `max_parallel`). Each `TuneTrialResult`
  holds:

  `spec` — `ModelSpec`
  : The exact candidate this trial evaluated.

  `status` — `Status`
  : `ok()` when the trial completed and produced a finite `objective`.
    A non-ok status means this trial's `cross_assess` call itself failed
    (an invalid `spec`, a fold that could not fit, `batch`/metric
    validation) *or* that `objective_metric` was not found among the
    resolved metrics — either way, `assess` is a default-constructed
    (empty) `CrossAssessReport` and `objective` is `NaN`.

  `assess` — `CrossAssessReport`
  : The full fold-by-fold result this trial's `cross_assess` call
    produced — see [`CrossAssessReport`](../metrics/CrossAssessReport.md).
    Default-constructed when `status` is not ok.

  `objective` — `double`
  : The one value read out of `assess.mean_metrics` at
    `objective_metric`'s position — what this trial was ranked by. `NaN`
    when `status` is not ok.

  `stage` — `std::int64_t`
  : Always `0` for [`sweep`](sweep.md) and [`probe`](probe.md) (and for
    [`tune`](tune.md) routed to either of them — `search =
    None`/`Grid`/`Random`). For `search = SuccessiveHalving`, the 0-based
    elimination round that produced this trial (every candidate is
    evaluated once per round it survives into, so the same `ModelSpec` can
    appear more than once across `trials` with increasing `stage`). For
    `search = Bayesian`, the 0-based position of this trial within that
    search's own sequence — not a round; every Bayesian trial has a
    distinct `stage`, equal to its own index among that search's trials.

`best_index` — `std::int64_t`
: The 0-based index into `trials` of the best trial found — `-1` when no
  trial succeeded with a finite `objective`. Tracked incrementally as each
  trial completes: the first trial with a finite objective becomes the
  incumbent, and only a strictly better later trial (`>` when maximizing,
  `<` when minimizing — never `>=`/`<=`) replaces it, so a tie keeps
  whichever trial was found first.

`objective_metric` — `MetricKind`
: A copy of the `objective_metric` argument the producing call was given.

`objective_policy` — `TuneObjectivePolicy`
: A copy of the `objective_policy` argument as given — **not** resolved.
  A call made with the default `Auto` still reports `objective_policy ==
  Auto` here; read `objective_metric` alongside it and consult the table
  below to know which direction was actually used for ranking.
  `TuneObjectivePolicy` has three values:

  - `Auto` (the default) — the ranking direction is derived from
    `objective_metric` alone, via the fixed table below.
  - `Maximize` — larger `objective` values rank better, regardless of
    which metric it is.
  - `Minimize` — smaller `objective` values rank better, regardless of
    which metric it is.

  `Auto`'s per-metric direction (every `MetricKind` classification and
  regression can request):

  | Prefers **larger** (maximize) | Prefers **smaller** (minimize) |
  | --- | --- |
  | `Accuracy`, `BalancedAccuracy`, `Precision`, `Recall`/`Sensitivity`, `F1`, `RocAuc`, `Specificity`, `GeometricMean`, `BalanceQuality`, `Mcc`, `CohenKappa`, `Gini`, `AveragePrecision`, `R2` | `LogLoss`, `Mse`, `Rmse`, `Mae`, `QuantileLoss`, `Brier` |

`objective_rank` — `std::vector<std::int64_t>`
: Parallel to `trials` — a 1-based rank per trial, `1` best, following the
  same standard-competition convention as most external ranking
  tools (ties share a rank, and the next distinct value's rank skips
  ahead by the tie's size rather than incrementing by one — see
  [`sweep`](sweep.md#example-1) for a worked tie). Every trial with
  `!status.ok()` or a non-finite `objective` shares one common **worst**
  rank (`count(finite trials) + 1`), regardless of how many of them there
  are — a failed trial is never distinguished from another failed trial by
  rank.

`ledger` — `TuneLedger`
: The same `trials`, restated as parallel `pluss::Series` columns instead
  of a `std::vector` of structs — see [`TuneLedger`](TuneLedger.md).

## Methods

None — `TuneReport` is a plain data holder, read directly.

## Practical Example

A 3-candidate sweep with one tied pair and one candidate that fails to
fit — `objective_rank`'s tie handling and shared worst-rank bucket, both
visible in one report.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.9, 2.0, 2.1, 2.2}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions logistic_a{};
  logistic_a.objective = cls::LinearObjective::Logistic;
  cls::LinearFamilyOptions logistic_b{};
  logistic_b.objective = cls::LinearObjective::Logistic;
  logistic_b.regularization_strength = 0.5;
  // Default `objective = LeastSquares` builds fine but fails learn() for a
  // classification task -- see LinearFamilyOptions.md#objective.
  cls::LinearFamilyOptions broken{};

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, logistic_a},
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, logistic_b},
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, broken},
  };

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::TuneReport report =
      cls::sweep(candidates, batch, split, {}, {}, cls::MetricKind::Accuracy).value();
  std::cout << "best_index = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << ": ok=" << std::boolalpha << report.trials[i].status.ok()
              << " objective=" << report.trials[i].objective
              << " rank=" << report.objective_rank[i] << "\n";
  }

  return 0;
}
```

Output

```text
best_index = 0
trial 0: ok=true objective=0.916667 rank=1
trial 1: ok=true objective=0.916667 rank=1
trial 2: ok=false objective=nan rank=3
```

## Changelog

- **0.1** — `trials` (each a `TuneTrialResult`), `best_index`,
  `objective_metric`, `objective_policy`, `objective_rank`, `ledger`.

## See Also

- [`sweep`](sweep.md) / [`probe`](probe.md) / [`tune`](tune.md) — the
  producing functions.
- [`TuneLedger`](TuneLedger.md) — the same trials as `Series` columns.
- [`CrossAssessReport`](../metrics/CrossAssessReport.md) — what each
  trial's own `assess` field holds.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
