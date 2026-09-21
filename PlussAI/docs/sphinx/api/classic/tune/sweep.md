# `pluss::ai::classic::sweep`

Cross-assess an explicit list of [`ModelSpec`](../families/ModelSpec.md)
candidates against the same data and rank them — the plain grid search
underneath [`tune`](tune.md)'s `search = Grid`/`None`. Reach for it directly
when you already have a finite, hand-picked list of configurations to
compare and do not need [`tune`](tune.md)'s other search strategies.

## Signature

```cpp
classic::StatusOr<classic::TuneReport> classic::sweep(
    /* a range of ModelSpec -- pass a std::vector<ModelSpec> or braced list */ candidates,
    const classic::LabeledBatch& batch,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::TuneObjectivePolicy objective_policy = classic::TuneObjectivePolicy::Auto,
    std::int64_t max_parallel = 0);
```

## Parameters

`candidates`
: A non-owning view of `ModelSpec` values — pass a `std::vector<ModelSpec>`
  or a braced `{spec_a, spec_b, ...}` list directly. Must be non-empty.
  Candidates may mix families and tasks freely (a `Linear` and a `Tree`
  regressor side by side is an ordinary comparison — see
  [`ModelSpec`](../families/ModelSpec.md), the data type every candidate is
  built from); every candidate is cross-assessed independently, once each.

`batch`
: The full dataset every candidate is scored against — must pass
  [`validate_labeled`](../data/LabeledBatch.md).

`split` (`SplitOptions`, default `{}`)
: Forwarded to every candidate's own
  [`cross_assess`](../metrics/cross_assess.md) call unchanged — every
  candidate is scored on **the same split plan**, so the comparison is
  fair. See [`SplitOptions`](../split/SplitOptions.md) for every field.

`train_options` / `assess_options`
: Forwarded to every candidate's `cross_assess` call unchanged.
  `assess_options.metrics` empty (the default) falls back to the task's
  default metric set, per candidate's own task — a mix of classification
  and regression candidates each resolve their own defaults independently.

`objective_metric` (default `MetricKind::Accuracy`)
: Which metric from each candidate's resolved `CrossAssessReport.mean_metrics`
  ranks it. **Classification-shaped by default** — a `sweep` call over
  regression candidates that leaves this at its default fails every trial
  (`Accuracy` is never among a regression task's resolved metrics); always
  pass a regression `MetricKind` (`R2`, `Mse`, `Rmse`, `Mae`,
  `QuantileLoss`) explicitly for regression candidates — see
  [Example 3](#example-3-the-default-objective_metric-pitfall).

`objective_policy` (default `TuneObjectivePolicy::Auto`)
: Ranking direction — see [`TuneReport`](TuneReport.md#state) for the
  full `Auto` per-metric table.

`max_parallel` (default `0`)
: How many candidates are cross-assessed concurrently. `<= 0` auto-sizes
  from hardware concurrency. When more than one worker runs and a
  candidate's own `split.max_parallel` was left at its default (`<= 0`),
  `sweep` bounds that candidate's *inner* fold-level parallelism too
  (roughly `hardware_concurrency / workers`), so evaluating many
  candidates concurrently does not oversubscribe more threads than the
  machine has. Candidate order in `report.trials` always matches
  `candidates`' own order, regardless of how many ran concurrently or
  which one finished first.

## Returns

A `StatusOr<TuneReport>` — one [`TuneTrialResult`](TuneReport.md#state) per
candidate, in `candidates`' own order; every trial's `stage` is `0`. See
[`TuneReport`](TuneReport.md) for `best_index` / `objective_rank` /
`ledger`.

## Errors

- `candidates` empty — `Status::invalid_argument`.
- `batch` fails label-batch validation — that `Status`.
- Everything else — a candidate's own `cross_assess` failure (a mismatched
  spec, a fold that cannot fit, `objective_metric` absent from the
  resolved metrics) is captured on that one trial's `status`/`objective`,
  never propagated as `sweep`'s own failure.

## Practical Examples

A 4-candidate classification sweep with a tied pair (`objective_rank`'s
tie handling), a cross-family regression sweep with an explicit objective
metric, the default-metric pitfall on a regression task, and the
empty-candidate-list error.

### Example 1

Four `Linear` classifiers differing only in `regularization_strength`, two
tied pairs — `objective_rank` shows the "skip a rank" tie convention: two
trials tied for rank `1`, the other two tied for rank `3` (not `2` —
`2` is skipped because two trials already occupy `1`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 1.9, 2.1, 2.3, 2.5, 2.7}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  std::vector<cls::ModelSpec> candidates{};
  for (double strength : {1e-4, 1e-4, 1.0, 1000.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    candidates.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::TuneReport report = cls::sweep(candidates, batch, split).value();
  std::cout << "best_index = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << ": objective=" << report.trials[i].objective
              << " rank=" << report.objective_rank[i] << "\n";
  }

  return 0;
}
```

Output

```text
best_index = 0
trial 0: objective=0.688889 rank=1
trial 1: objective=0.688889 rank=1
trial 2: objective=0.588889 rank=3
trial 3: objective=0.588889 rank=3
```

### Example 2

A `Linear` regressor against a `Tree` regressor on the same data, ranked
by an explicit `R2` — the exact reason `ModelSpec` exists as data: neither
estimator is built until `sweep` decides which one to keep. `max_parallel
= 2` here; ranking is identical regardless of how many candidates run
concurrently, since every candidate sees the same split plan.

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

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}},
  };

  const cls::TuneReport report =
      cls::sweep(candidates, batch, {}, {}, {}, cls::MetricKind::R2,
                cls::TuneObjectivePolicy::Auto, /*max_parallel=*/2).value();
  std::cout << "best_index = " << report.best_index << "\n";
  std::cout << "winning family = " << static_cast<int>(report.trials[report.best_index].spec.family) << "\n";

  return 0;
}
```

Output

```text
best_index = 0
winning family = 0
```

### Example 3: The Default objective_metric Pitfall

The same regression candidates as Example 2, `objective_metric` left at
its classification-shaped default — every trial fails, `best_index`
stays `-1`.

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

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
  };

  // objective_metric left at its default: MetricKind::Accuracy.
  const cls::TuneReport report = cls::sweep(candidates, batch).value();
  std::cout << "best_index = " << report.best_index << "\n";
  std::cout << "trial 0 ok = " << std::boolalpha << report.trials[0].status.ok() << "\n";
  std::cout << "trial 0 message = \"" << report.trials[0].status.message << "\"\n";

  return 0;
}
```

Output

```text
best_index = -1
trial 0 ok = false
trial 0 message = "tune objective metric is not present in cross_assess metrics output."
```

### Example 4

An empty candidate list is rejected before any trial runs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const std::vector<cls::ModelSpec> candidates{};
  const cls::StatusOr<cls::TuneReport> report = cls::sweep(candidates, batch);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "sweep requires at least one candidate ModelSpec."
```

## Changelog

- **0.1** — `sweep`.

## See Also

- [`TuneReport`](TuneReport.md) — the returned type, including the tie /
  worst-rank rules exercised in Example 1.
- [`probe`](probe.md) — the sampler-driven counterpart, for a search space
  too large to enumerate.
- [`tune`](tune.md) — routes `search = Grid`/`None` to this same function.
- [`metrics::cross_assess`](../metrics/cross_assess.md) — what each trial
  runs.
