# `pluss::ai::classic::tune`

Run [`sweep`](sweep.md) or [`probe`](probe.md) — or one of two strategies
neither implements — under one [`TuneOptions`](TuneOptions.md) budget. Two
overloads mirror the two ways to supply candidates: an explicit list (the
[`sweep`](sweep.md) shape) or a sampler callback (the [`probe`](probe.md)
shape); `tune_options.search` then picks which underlying strategy actually
runs. Reach for `sweep`/`probe` directly when you already know which
strategy you want; reach for `tune` to make the strategy itself a
configuration choice.

## Signature

```cpp
classic::StatusOr<classic::TuneReport> classic::tune(
    /* a range of ModelSpec -- pass a std::vector<ModelSpec> or braced list */ candidates,
    const classic::LabeledBatch& batch,
    const classic::TuneOptions& tune_options = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::TuneObjectivePolicy objective_policy = classic::TuneObjectivePolicy::Auto);

classic::StatusOr<classic::TuneReport> classic::tune(
    const classic::ProbeSpecSampler& sampler,
    const classic::LabeledBatch& batch,
    const classic::TuneOptions& tune_options = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::TuneObjectivePolicy objective_policy = classic::TuneObjectivePolicy::Auto);
```

## What It Does

Reads `tune_options.search` and dispatches to whichever underlying
strategy that value selects -- [`sweep`](sweep.md) for an exhaustive pass
over `candidates`, [`probe`](probe.md) for a sampled search from
`sampler`, or one of the two strategies neither of those directly
implements. Which overload is called (candidate list vs. sampler) fixes
which *shape* of search is possible; `tune_options.search` then fixes
which strategy actually runs within that shape -- see the dispatch tables
below for the exact mapping.

## Parameters

`candidates` (first overload) / `sampler` (second overload)
: The two candidate-supply shapes — see [`sweep`'s `candidates`](sweep.md#parameters)
  and [`probe`'s `sampler`](probe.md#parameters) for each one's own
  contract; `tune` forwards whichever one you called with unchanged.

`batch`
: The full dataset every trial is scored against — must pass
  [`validate_labeled`](../data/LabeledBatch.md).

`tune_options` (`TuneOptions`, default `{}`)
: The whole search-strategy contract — see [`TuneOptions`](TuneOptions.md)
  for every field. `tune_options.search` drives the dispatch tables below;
  `tune_options.split` is forwarded as `split` to whichever function
  actually runs.

`train_options` / `assess_options` / `objective_metric` / `objective_policy`
: Identical contract to [`sweep`'s](sweep.md#parameters) /
  [`probe`'s](probe.md#parameters) own — including the same
  classification-shaped `objective_metric` default pitfall (see
  [`sweep`'s Example 3](sweep.md#example-3-the-default-objective_metric-pitfall)).

### Dispatch: Candidate-List Overload

| `tune_options.search` | Runs | Notes |
| --- | --- | --- |
| `None` or `Grid` | [`sweep`](sweep.md) over every candidate | exhaustive; `tune_options.trials` is ignored |
| `Random` | [`sweep`](sweep.md) over `tune_options.trials` candidates sampled without replacement from `candidates`, shuffled by `tune_options.random` | requires `trials > 0`; `trials >= candidates.size()` still evaluates every candidate, in shuffled order |
| `SuccessiveHalving` | every candidate, then the better half each round, until one remains | `tune_options.trials` is ignored — round 0 always uses every candidate |
| `Bayesian` | a Gaussian-process-guided search over `tune_options.trials` of `candidates`, one at a time | requires `trials > 0`; capped at `candidates.size()` |

### Dispatch: Sampler Overload

| `tune_options.search` | Runs | Notes |
| --- | --- | --- |
| `None` or `Random` | [`probe`](probe.md) for `tune_options.trials` trials | requires `trials > 0` |
| `Bayesian` | draws a pool of `max(trials + 8, trials * 4)` candidates from `sampler` up front, then the same GP-guided search as the candidate-list overload, over that pool | requires `trials > 0`; a `sampler` failure while building the pool fails the **whole** `tune` call (see [Example 6](#example-6-sampler-pool-failure-is-not-per-trial)) — unlike a per-trial `sampler` failure inside `probe` itself, which only fails that one trial |
| `SuccessiveHalving` | draws exactly `tune_options.trials` candidates from `sampler` up front, then the same round-based elimination as the candidate-list overload | requires `trials > 0`; the same whole-call-failure rule as `Bayesian` applies to the up-front draw |
| `Grid` | rejected outright | `Status::invalid_argument` naming the candidate-list overload; only reachable with `trials > 0` — `trials <= 0` is checked first, for every `search` value including `Grid` |

## Returns

A `StatusOr<TuneReport>` — see [`TuneReport`](TuneReport.md). `trials`'
length and `stage` values depend entirely on which strategy ran, per the
dispatch tables above.

## Errors

Candidate-list overload:

- `candidates` empty — `Status::invalid_argument`, checked before
  `tune_options.search` is even inspected.
- `search == Random` or `search == Bayesian` with `tune_options.trials <=
  0` — `Status::invalid_argument`.
- Everything [`sweep`](sweep.md#errors) itself can fail with, once
  dispatched.

Sampler overload:

- `sampler` empty (default-constructed) — `Status::invalid_argument`.
- `tune_options.trials <= 0` — `Status::invalid_argument`, checked before
  `tune_options.search` is inspected (so it also gates `search == Grid`
  — see [Example 8](#example-8-the-two-sampler-overload-gates-in-order)).
- `search == Grid` (with `trials > 0`) — `Status::invalid_argument`
  naming the candidate-list overload.
- A `sampler` failure while building the up-front candidate pool for
  `Bayesian`/`SuccessiveHalving` — that `Status`, propagated directly for
  the whole call (see [Example 6](#example-6-sampler-pool-failure-is-not-per-trial)).
- Everything [`probe`](probe.md#errors) itself can fail with, once
  dispatched.

## Practical Examples

Every one of the eight dispatch-table rows above, exercised once: the
default search matching `sweep`/`probe` directly, `Random` on both
overloads, `SuccessiveHalving`'s round progression on both overloads,
`Bayesian` on both overloads (including its whole-call sampler-pool
failure), and the sampler overload's `Grid` rejection with its two error
gates.

### Example 1

`search = None` (the default) on the candidate-list overload is exactly
[`sweep`](sweep.md) — same `best_index`, same `objective_rank`, for the
same inputs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
std::vector<pluss::ai::classic::ModelSpec> regularization_candidates() {
  namespace cls = pluss::ai::classic;
  std::vector<cls::ModelSpec> out{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    out.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }
  return out;
}
}  // namespace

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

  cls::TuneOptions tune_options{};
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const std::vector<cls::ModelSpec> candidates = regularization_candidates();
  const cls::TuneReport via_tune = cls::tune(candidates, batch, tune_options).value();
  const cls::TuneReport via_sweep = cls::sweep(candidates, batch, tune_options.split).value();

  std::cout << "best_index matches = " << std::boolalpha << (via_tune.best_index == via_sweep.best_index) << "\n";
  std::cout << "trial count matches = " << (via_tune.trials.size() == via_sweep.trials.size()) << "\n";

  return 0;
}
```

Output

```text
best_index matches = true
trial count matches = true
```

### Example 2

`search = Random` on the candidate-list overload: `trials = 2` of the same
4 candidates, deterministic sampling.

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

  std::vector<cls::ModelSpec> candidates{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    candidates.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::Random;
  tune_options.trials = 2;
  tune_options.random.deterministic = true;
  tune_options.random.seed = 3;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::TuneReport report = cls::tune(candidates, batch, tune_options).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  for (const auto& trial : report.trials) {
    const auto& opts = std::get<cls::LinearFamilyOptions>(trial.spec.options);
    std::cout << "sampled regularization_strength = " << opts.regularization_strength << "\n";
  }

  return 0;
}
```

Output

```text
trial count = 2
sampled regularization_strength = 100
sampled regularization_strength = 1
```

### Example 3

`search = SuccessiveHalving` on the candidate-list overload: 4 candidates
enter round 0, the better 2 survive into round 1, the better 1 survives
into round 2 — 7 trials total, `.stage` recording each round.

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

  std::vector<cls::ModelSpec> candidates{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    candidates.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::SuccessiveHalving;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::TuneReport report = cls::tune(candidates, batch, tune_options).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  for (const auto& trial : report.trials) {
    std::cout << "stage = " << trial.stage << "\n";
  }

  return 0;
}
```

Output

```text
trial count = 7
stage = 0
stage = 0
stage = 0
stage = 0
stage = 1
stage = 1
stage = 2
```

### Example 4

`search = Bayesian` on the candidate-list overload: `trials = 3` of 6
candidates, `.stage` recording each trial's own position in the search
(not a round).

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

  std::vector<cls::ModelSpec> candidates{};
  for (double strength : {1e-4, 1e-3, 1e-2, 1e-1, 1.0, 10.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    candidates.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::Bayesian;
  tune_options.trials = 3;
  tune_options.random.deterministic = true;
  tune_options.random.seed = 11;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::TuneReport report = cls::tune(candidates, batch, tune_options).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  for (const auto& trial : report.trials) {
    std::cout << "stage = " << trial.stage << "\n";
  }
  std::cout << "best_index = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
trial count = 3
stage = 0
stage = 1
stage = 2
best_index = 0
```

### Example 5

The sampler overload, `search` left at its default `None` — identical
dispatch to `Random`, both routing to [`probe`](probe.md).

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

  const cls::ProbeSpecSampler sampler = [](std::int64_t, std::uint64_t seed) -> cls::StatusOr<cls::ModelSpec> {
    const double unit = static_cast<double>(seed % 1000) / 1000.0;
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = 1e-3 + unit * (10.0 - 1e-3);
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::TuneOptions tune_options{};
  tune_options.trials = 3;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::TuneReport report = cls::tune(sampler, batch, tune_options).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  std::cout << "best_index  = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
trial count = 3
best_index  = 0
```

### Example 6: Sampler-Pool Failure Is Not Per-Trial

The sampler overload with `search = Bayesian`: the pool is built by
calling `sampler` up front for `max(trials + 8, trials * 4)` indices, so
`trials = 3` here builds a 12-candidate pool (indices `0..11`). The
sampler refuses one specific index inside that range — the **whole**
`tune` call fails with that exact message, unlike `probe`'s own per-trial
isolation (see [`probe`'s Example 3](probe.md#example-3-partial-sampler-failure)).

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

  const cls::ProbeSpecSampler sampler =
      [](std::int64_t trial_index, std::uint64_t) -> cls::StatusOr<cls::ModelSpec> {
    if (trial_index == 5) {
      return cls::StatusOr<cls::ModelSpec>(
          cls::Status::invalid_argument("sampler: refusing pool index 5."));
    }
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::Bayesian;
  tune_options.trials = 3;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::StatusOr<cls::TuneReport> report = cls::tune(sampler, batch, tune_options);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "sampler: refusing pool index 5."
```

### Example 7

The sampler overload with `search = SuccessiveHalving`: exactly `trials =
4` candidates are drawn up front (no `* 4` pool inflation, unlike
`Bayesian`), then the same round-based elimination as
[Example 3](#example-3) runs over them.

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

  const cls::ProbeSpecSampler sampler = [](std::int64_t trial_index, std::uint64_t) -> cls::StatusOr<cls::ModelSpec> {
    const std::array<double, 4> strengths = {1e-4, 1e-2, 1.0, 100.0};
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strengths[static_cast<std::size_t>(trial_index) % strengths.size()];
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::SuccessiveHalving;
  tune_options.trials = 4;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::TuneReport report = cls::tune(sampler, batch, tune_options).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  for (const auto& trial : report.trials) {
    std::cout << "stage = " << trial.stage << "\n";
  }

  return 0;
}
```

Output

```text
trial count = 7
stage = 0
stage = 0
stage = 0
stage = 0
stage = 1
stage = 1
stage = 2
```

### Example 8: The Two Sampler-Overload Gates, In Order

`search = Grid` on the sampler overload is always rejected, but which
message you get depends on `trials`: `trials <= 0` is checked *before*
`search` is even inspected, so it reports the generic budget error, not
the `Grid`-specific one; only `trials > 0` reaches the `Grid`-specific
rejection.

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

  const cls::ProbeSpecSampler sampler =
      [](std::int64_t, std::uint64_t) -> cls::StatusOr<cls::ModelSpec> {
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  };

  cls::TuneOptions zero_trials{};
  zero_trials.search = cls::SearchKind::Grid;
  zero_trials.trials = 0;
  const cls::StatusOr<cls::TuneReport> zero_report = cls::tune(sampler, batch, zero_trials);
  std::cout << "zero-trials message = \"" << zero_report.status().message << "\"\n";

  cls::TuneOptions with_trials{};
  with_trials.search = cls::SearchKind::Grid;
  with_trials.trials = 3;
  const cls::StatusOr<cls::TuneReport> grid_report = cls::tune(sampler, batch, with_trials);
  std::cout << "grid message = \"" << grid_report.status().message << "\"\n";

  return 0;
}
```

Output

```text
zero-trials message = "tune(sampler,...) requires TuneOptions.trials > 0."
grid message = "tune(sampler,...): SearchKind::Grid requires the candidate-list overload."
```

## Changelog

- **0.1** — two overloads (candidate-list, sampler), dispatched by
  `TuneOptions::search` to `sweep`, `probe`, successive halving, or
  Bayesian search.

## See Also

- [`TuneOptions`](TuneOptions.md) — every field driving the dispatch
  tables above.
- [`sweep`](sweep.md) / [`probe`](probe.md) — the two strategies
  implemented directly; every other `search` value is built on one of
  them.
- [`TuneReport`](TuneReport.md) — the returned type, including what
  `stage` means for `SuccessiveHalving` versus `Bayesian`.
