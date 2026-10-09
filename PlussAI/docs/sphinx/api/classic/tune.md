# `pluss::ai::classic::tune`

`tune` owns choosing between model configurations: given either an explicit
list of [`ModelSpec`](families/ModelSpec.md) candidates or a callback that
generates them, it cross-assesses every one against the same data and ranks
them by a chosen metric — without you building an estimator, timing a loop,
or writing the ranking logic yourself. Every function here is built directly
on [`metrics::cross_assess`](metrics.md): a "trial" is one
`cross_assess` call, and a `tune`-family function differs only in *which*
`ModelSpec`s it evaluates and in what order.

## The Surface

| Function / type | What it does | Reach for it when |
| --- | --- | --- |
| [`sweep`](tune/sweep.md) | cross-assess every candidate in an explicit list, rank them | you already have a finite list of configurations to compare (a hand-picked grid) |
| [`probe`](tune/probe.md) | cross-assess `trials` candidates drawn one at a time from a sampler callback | the space of configurations is generated, not enumerated (random search over a distribution) |
| [`tune`](tune/tune.md) | run `sweep` or `probe` under a [`TuneOptions`](tune/TuneOptions.md) budget — the one entry point that also unlocks successive-halving and Bayesian search | you want to pick a search *strategy* (grid, random, halving, Bayesian) through one shared options struct rather than choosing between `sweep`/`probe` yourself |
| [`TuneReport`](tune/TuneReport.md) | every trial's outcome, the best index, objective ranks, and a columnar ledger | reading the results of any call above |
| [`TuneLedger`](tune/TuneLedger.md) | `TuneReport.trials`, restated as parallel `pluss::Series` columns | feeding trial results into further `Series`-based analysis instead of looping over `std::vector<TuneTrialResult>` by hand |
| [`TuneOptions`](tune/TuneOptions.md) | search kind, trial budget, parallelism, inner split, RNG | driving the `tune(...)` entry point |

## How To Call It

Compare two linear candidates on a held-out split and report the index of the best one. The candidates differ only in regularization strength.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const cls::LabeledBatch batch = fixture_batch(cfg);
  cls::LinearFamilyOptions linear_a{};
  linear_a.objective = cls::LinearObjective::Logistic;
  cls::LinearFamilyOptions linear_b = linear_a;
  linear_b.regularization_strength = 1.0;
  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear_a},
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear_b},
  };
  cls::SplitOptions split_options{};
  split_options.holdout_fraction = 0.25;
  const cls::StatusOr<cls::TuneReport> report =
      cls::sweep(candidates, batch, split_options, {}, {}, cls::MetricKind::Accuracy);
  std::cout << "best candidate index = " << report.value().best_index << "\n";
  return 0;
}
```

Output

```text
best candidate index = 0
```

Every trial's own `cross_assess` failure (a mismatched spec, an objective
metric absent from the resolved metric list, a fold that could not be
fit) is captured on that one trial — `report.trials[i].status` /
`.objective` (`NaN` when it failed) — never propagated as the whole call's
own failure. A `sweep`/`probe`/`tune` call itself only fails for conditions
checked before any trial runs (an empty candidate list, an invalid trial
budget, a batch that fails validation) — see each function's own `##
Errors`.

## Related Pages

- [`metrics`](metrics.md) — `cross_assess` / `CrossAssessReport`, what one
  trial actually runs and returns.
- [`split`](split.md) — `SplitOptions`, forwarded unchanged to every trial's
  `cross_assess` call.
- [`families`](families.md) — `ModelSpec` and the nine `*FamilyOptions`
  structs a candidate is built from.
- [`core`](core.md) — `MetricKind`, the full list `objective_metric` accepts.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

tune/Introduction
tune/sweep
tune/probe
tune/tune
tune/TuneReport
tune/TuneLedger
tune/TuneOptions
```
<!-- apiref:toctree:end -->
