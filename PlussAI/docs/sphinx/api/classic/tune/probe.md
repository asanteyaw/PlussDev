# `pluss::ai::classic::probe`

Cross-assess `trials` [`ModelSpec`](../families/ModelSpec.md) candidates
drawn one at a time from a sampler callback and rank them — the sampled
search underneath [`tune`](tune.md)'s `search = None`/`Random` (sampler
overload). Reach for it directly when the space of configurations is
generated rather than enumerated (drawing a hyperparameter from a
distribution each trial) instead of chosen from a fixed list — see
[`sweep`](sweep.md) for that shape instead.

## Signature

```cpp
using classic::ProbeSpecSampler =
    std::function<classic::StatusOr<classic::ModelSpec>(std::int64_t trial_index, std::uint64_t seed)>;

classic::StatusOr<classic::TuneReport> classic::probe(
    const classic::ProbeSpecSampler& sampler,
    std::int64_t trials,
    const classic::LabeledBatch& batch,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::TuneObjectivePolicy objective_policy = classic::TuneObjectivePolicy::Auto,
    classic::RandomState random = {},
    std::int64_t max_parallel = 0);
```

## Parameters

`sampler`
: Called once per trial to build that trial's candidate:
  `trial_index` is the 0-based trial number in `[0, trials)`; `seed` is a
  distinct `std::uint64_t` derived for that trial (see `random` below) —
  use it to seed your own draw from a distribution so the whole `probe`
  call is reproducible end to end when `random.deterministic` is `true`.
  Returning a non-ok `StatusOr<ModelSpec>` fails only that one trial
  (captured on `report.trials[trial_index]`), the same as a `cross_assess`
  failure — it never aborts the surrounding `probe` call. Must be a valid
  (non-empty) callable.

`trials`
: How many candidates to draw and evaluate. Must be `> 0`.

`batch` / `split` / `train_options` / `assess_options` / `objective_metric` /
`objective_policy`
: Identical contract to [`sweep`](sweep.md#parameters)'s own — every trial
  is cross-assessed against the same `batch`/`split`, and the same
  classification-shaped `objective_metric` default pitfall applies (see
  [`sweep`'s Example 3](sweep.md#example-3-the-default-objective_metric-pitfall)).

`random` (`RandomState`, default `{}`)
: Derives each trial's `seed`: a base value is composed once from `random`
  (see [`RandomState`](../core/RandomState.md) for its own
  seeded/non-deterministic contract), then trial `i`'s `seed` is a fixed,
  well-mixed function of `base + i` — distinct per trial. `random{}`'s
  default `deterministic = true` makes the whole `seed` sequence (and, for
  a `sampler` that is itself a pure function of `seed`, the whole trial
  sequence) identical across repeated `probe` calls with the same
  `random.seed` and `trials` (see
  [Example 2](#example-2-deterministic-reproduction)). `random.deterministic
  = false` mixes in wall-clock time and system entropy instead, so even
  the sequence of `seed` values differs between runs.

`max_parallel` (default `0`)
: Identical contract to [`sweep`'s own](sweep.md#parameters) — how many
  trials run concurrently, with the same nested fold-parallelism bounding.

## Returns

A `StatusOr<TuneReport>` — one [`TuneTrialResult`](TuneReport.md#state) per
requested trial, in trial-index order; every trial's `stage` is `0`. See
[`TuneReport`](TuneReport.md) for `best_index` / `objective_rank` /
`ledger`.

## Errors

- `sampler` is empty (default-constructed) — `Status::invalid_argument`.
- `trials <= 0` — `Status::invalid_argument`.
- `batch` fails label-batch validation — that `Status`.
- Everything else — a per-trial `sampler` failure or `cross_assess`
  failure is captured on that one trial, never propagated as `probe`'s own
  failure (see [Example 3](#example-3-partial-sampler-failure)).

## Practical Examples

A sampler drawing `regularization_strength` from its own `seed`,
deterministic reproduction across two separate calls, a sampler that fails
on some trials but not others, and the `trials <= 0` error.

### Example 1

`sampler` maps each trial's `seed` to a `regularization_strength` in
`[1e-3, 10.0]` via a fixed formula — `trial_index` is unused here, but is
available for samplers whose distribution should change over the run
(for instance, narrowing a search range on later trials).

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
    const double unit = static_cast<double>(seed % 1000) / 1000.0;  // [0, 1)
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = 1e-3 + unit * (10.0 - 1e-3);
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::RandomState random{};
  random.deterministic = true;
  random.seed = 7;

  const cls::TuneReport report = cls::probe(sampler, 5, batch, split, {}, {},
                                            cls::MetricKind::Accuracy,
                                            cls::TuneObjectivePolicy::Auto, random).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  std::cout << "best_index  = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
trial count = 5
best_index  = 0
```

### Example 2: Deterministic Reproduction

The same call, run twice with `random.deterministic = true` and the same
`random.seed` — every trial's `objective` and `objective_rank` matches
exactly across both calls.

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

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::RandomState random{};
  random.deterministic = true;
  random.seed = 7;

  const cls::TuneReport first = cls::probe(sampler, 4, batch, split, {}, {},
                                           cls::MetricKind::Accuracy,
                                           cls::TuneObjectivePolicy::Auto, random).value();
  const cls::TuneReport second = cls::probe(sampler, 4, batch, split, {}, {},
                                            cls::MetricKind::Accuracy,
                                            cls::TuneObjectivePolicy::Auto, random).value();

  bool identical = first.best_index == second.best_index;
  for (std::size_t i = 0; identical && i < first.trials.size(); ++i) {
    identical = identical && first.trials[i].objective == second.trials[i].objective
             && first.objective_rank[i] == second.objective_rank[i];
  }
  std::cout << "identical across both calls = " << std::boolalpha << identical << "\n";

  return 0;
}
```

Output

```text
identical across both calls = true
```

### Example 3: Partial Sampler Failure

`sampler` itself rejects every other trial (a stand-in for a sampler that
refuses to build a candidate for some drawn parameter combination) — those
trials fail individually, `probe` as a whole still succeeds, and
`best_index` still resolves from the trials that worked.

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
    if (trial_index % 2 != 0) {
      return cls::StatusOr<cls::ModelSpec>(
          cls::Status::invalid_argument("sampler: refusing an odd trial index."));
    }
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::TuneReport report = cls::probe(sampler, 4, batch, split).value();
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << " ok = " << std::boolalpha << report.trials[i].status.ok() << "\n";
  }
  std::cout << "best_index = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
trial 0 ok = true
trial 1 ok = false
trial 2 ok = true
trial 3 ok = false
best_index = 0
```

### Example 4

`trials <= 0` is rejected before the sampler is ever called.

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

  const cls::StatusOr<cls::TuneReport> report = cls::probe(sampler, 0, batch);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "probe requires trials > 0."
```

## Changelog

- **0.1** — `probe`, `ProbeSpecSampler`.

## See Also

- [`TuneReport`](TuneReport.md) — the returned type.
- [`sweep`](sweep.md) — the fixed-list counterpart.
- [`tune`](tune.md) — routes `search = None`/`Random` (sampler overload)
  to this same function.
- [`RandomState`](../core/RandomState.md) — the seed/determinism contract
  `random` follows.
