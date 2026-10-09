# `pluss::ai::classic::core`

`core` is the vocabulary every other `classic` family assumes you already know:
how a call reports failure, and the small option bags that configure a fit, a
prediction, or an assessment no matter which estimator family is doing the
work.

Nothing here trains a model or touches data. It is the plumbing everything
else is built from.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Status`](core/Status.md) | the result every fallible call returns instead of throwing — a code plus a message |
| [`StatusOr<T>`](core/StatusOr.md) | a `Status` paired with a `T` that is meaningful only when the status is ok |
| [`TrainOptions`](core/TrainOptions.md) | per-fit configuration: missing-value policy, class weighting, calibration, RNG, row balancing |
| [`InferOptions`](core/InferOptions.md) | prediction-time configuration, chiefly the binary decision threshold |
| [`AssessOptions`](core/AssessOptions.md) | which metrics `assess(...)` computes |
| [`RandomState`](core/RandomState.md) | the seed + determinism pair every stochastic routine takes |

## How To Call It

`Status` and `StatusOr<T>` are not something you construct in ordinary code —
you receive them. Every fallible `classic` function or method returns one of
the two:

Fit a flow and check the returned status, then build a cross-validation plan for eight samples. The program prints whether each call succeeded.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <iostream>
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
  cls::LinearFamilyOptions linear_options{};
  linear_options.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear_options}).move_value();
  cls::Status s = flow.learn(batch, cls::TrainOptions{});
  std::cout << "learn ok = " << (s.ok() ? "true" : "false") << "\n";
  cls::StatusOr<cls::SplitPlan> plan = cls::crossval_plan(8, cls::SplitOptions{});
  std::cout << "plan ok = " << (plan.ok() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
learn ok = true
plan ok = true
```

`TrainOptions`, `InferOptions`, `AssessOptions`, and `RandomState` are plain
aggregates you build yourself and pass by value:

```cpp
cls::TrainOptions train{};
train.max_iterations = 250;
train.class_weight = cls::ClassWeightMode::Balanced;

classifier->learn(batch, train);
```

## Related Pages

- [`data`](data.md) — the batch types `learn` / `infer` / `assess` all take.
- [`estimators`](estimators.md) — the `learn` / `infer` / `assess` contract
  that consumes `TrainOptions` / `InferOptions` / `AssessOptions`.
- [`metrics`](metrics.md) — `AssessOptions::metrics` names entries from the
  same `MetricKind` enum this family's pages reference.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

core/Introduction
core/Status
core/StatusOr
core/TrainOptions
core/InferOptions
core/AssessOptions
core/RandomState
```
<!-- apiref:toctree:end -->
