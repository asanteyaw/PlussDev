# `pluss::ai::classic::cross_assess` / `cross_scores` / `cross_score`

Fit and score a [`ModelSpec`](../families/ModelSpec.md) once per fold of a
[`split`](../split.md) plan — the one function in `classic` that owns both
training *and* evaluation, so a model's reliability can be judged across
several train/test partitions rather than trusting one. `cross_assess`
returns the full [`CrossAssessReport`](CrossAssessReport.md); `cross_scores`
and `cross_score` are convenience narrowings to one metric's per-fold values
or across-fold mean, respectively — both call `cross_assess` internally, so
using them costs the same as calling `cross_assess` yourself and reading one
field back.

## Signature

```cpp
classic::StatusOr<classic::CrossAssessReport> classic::cross_assess(
    const classic::ModelSpec& spec,
    const classic::LabeledBatch& batch,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {});

classic::StatusOr<pluss::Series> classic::cross_scores(
    const classic::ModelSpec& spec,
    const classic::LabeledBatch& batch,
    classic::MetricKind metric,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {});

classic::StatusOr<double> classic::cross_score(
    const classic::ModelSpec& spec,
    const classic::LabeledBatch& batch,
    classic::MetricKind metric,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {});
```

## Parameters

`spec`
: Must have `task == Classification` or `Regression` — every other
  `TaskKind` is rejected before any fold runs. A fresh classifier or
  regressor is built from `spec` for **every fold** — folds never share a
  fitted model.

`batch`
: The full dataset — sliced into train/test per fold by
  [`slice_labeled_batch`](../split/slice_batch.md); must pass
  [`validate_labeled`](../data/LabeledBatch.md).

`split` (`SplitOptions`, default `{}`)
: Drives which [`split`](../split.md) function actually runs, via `plan`:
  `Holdout` -> [`holdout_split`](../split/holdout.md) (or
  [`strat_holdout_split`](../split/holdout.md) — see `stratified` below),
  `CrossVal` -> [`crossval_plan`](../split/crossval_plan.md) (or its
  stratified sibling), `StratifiedCrossVal` ->
  [`stratified_crossval_plan`](../split/crossval_plan.md) unconditionally
  (fails if `spec.task != Classification`), `TSCrossVal` ->
  [`ts_crossval_plan`](../split/ts_crossval_plan.md), `GroupCrossVal` ->
  [`group_crossval_plan`](../split/crossval_plan.md) using `split.groups`
  (must be defined and length-matched — this is the one place
  `SplitOptions.groups` is actually read). `split.stratified`, when `true`
  and `spec.task == Classification` and `plan` is `Holdout` or `CrossVal`,
  routes to the stratified variant instead of the plain one.
  `split.reliability_defaults` (`true` by default): for a classification
  task with `plan = Holdout`/`CrossVal` and `stratified` not already
  `true`, silently sets it `true` first — see
  [`SplitOptions::reliability_defaults`](../split/SplitOptions.md#reliability_defaults).
  `split.max_parallel` controls how many folds are fit concurrently — `<= 0`
  (the default) auto-sizes from hardware concurrency; folds still appear in
  `report.folds` in plan order regardless of how many ran concurrently.
  Every other `SplitOptions` field is forwarded to whichever split function
  actually runs.

`train_options` / `assess_options`
: Forwarded to every fold's own `learn` / `assess` call unchanged.
  `assess_options.metrics` empty (the default) falls back to the task's
  default metric set, the same as a plain `assess` call would.

`metric` (`cross_scores` / `cross_score` only)
: The one `MetricKind` to extract. Added to `assess_options.metrics`
  automatically if not already present — you never need to also list it in
  `assess_options.metrics` yourself.

## Returns

`cross_assess` returns a `CrossAssessReport` — see its own page.

`cross_scores` returns a `Float64` `Series`, one value per fold, in plan
order — `metric`'s value from each fold, without needing to know that
metric's position in `assess_options.metrics`.

`cross_score` returns the plain `double` mean of `cross_scores`'s result
across every fold.

## Errors

- `spec.task` is neither `Classification` nor `Regression` —
  `Status::invalid_argument`.
- `batch` fails label-batch validation — that `Status`.
- The resolved split function itself fails (bad `folds`, `GroupCrossVal`
  with `groups` undefined, …) — that `Status`, propagated directly.
- Any fold's `learn` or `assess` fails — that `Status`, with a prefix naming
  the fold index (`1`-based in the message text), fold count, and that
  fold's train/test sample counts, so a failure can be traced to a specific
  fold without re-running anything.
- `assess_options.metrics` (or the resolved default) is empty —
  `Status::invalid_argument`.
- `cross_scores` / `cross_score`: `metric` was somehow not found among the
  resolved metrics — `Status::invalid_argument` (not reachable in ordinary
  use, since `metric` is added automatically).

## Practical Examples

The default holdout plan's single-fold report, 5-fold cross-validation with
`reliability_defaults` silently stratifying a classification task,
`cross_score`/`cross_scores` narrowing the same run to one metric,
`GroupCrossVal` keeping every group's rows in one fold, and `TSCrossVal`
(which requires `shuffle = false`, the same requirement
[`ts_crossval_plan`](../split/ts_crossval_plan.md) itself has).

### Example 1

`SplitOptions{}`'s default `plan = Holdout` produces exactly one fold.

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

  const cls::StatusOr<cls::CrossAssessReport> report = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}}, batch);
  std::cout << "fold count = " << report.value().folds.size() << "\n";
  std::cout << "mean_metrics [Mse, Rmse, R2] = " << report.value().mean_metrics << "\n";

  return 0;
}
```

Output

```text
fold count = 1
mean_metrics [Mse, Rmse, R2] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[9.2064592e-15, 9.5950295e-08, 1]
```

### Example 2

3-fold cross-validation on an imbalanced classification task (9 majority
rows, 3 minority rows) — `reliability_defaults` (on by default) stratifies
automatically, so every fold's *training* set keeps at least one minority
row (with only 3 minority rows total, any more folds than that would force
some fold's training set to have none at all, which the underlying
classifier rejects).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 4.0, 4.1, 4.2}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::CrossAssessReport report = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}, batch, split).value();
  std::cout << "fold count = " << report.folds.size() << "\n";
  std::cout << "mean accuracy = " << report.mean_metrics << "\n";

  return 0;
}
```

Output

```text
fold count = 3
mean accuracy = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 3

`cross_score` narrows the same kind of run to a single number, without
building an `AssessOptions` first.

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

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 5;

  const cls::Series per_fold = cls::cross_scores(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, cls::MetricKind::R2, split).value();
  const double mean_r2 = cls::cross_score(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, cls::MetricKind::R2, split).value();
  std::cout << "per-fold R2 = " << per_fold << "\n";
  std::cout << "mean R2     = " << mean_r2 << "\n";

  return 0;
}
```

Output

```text
per-fold R2 = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 1, 1, 1, 1]
mean R2     = 1
```

### Example 4

`split.plan = GroupCrossVal`: 4 groups of 2 rows each (repeated
measurements), `split.groups` naming which group each row belongs to —
every group's rows land in the same fold, together, whether that fold uses
them for training or testing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.05, 2.0, 2.05, 3.0, 3.05, 4.0, 4.05}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series(
      {2.0, 2.1, 4.0, 4.1, 6.0, 6.1, 8.0, 8.1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::GroupCrossVal;
  split.groups = pluss::make_series({0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0}, {8}, f64);
  split.folds = 2;

  const cls::CrossAssessReport report = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, split).value();
  std::cout << "fold count = " << report.folds.size() << "\n";
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << report.folds[i].test_indices << "\n";
  }

  return 0;
}
```

Output

```text
fold count = 2
fold 0 test_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[4, 5, 6, 7]
fold 1 test_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 3]
```

### Example 5

`split.plan = TSCrossVal` requires `split.shuffle = false` — the same
requirement [`ts_crossval_plan`](../split/ts_crossval_plan.md) itself has,
since a time-ordered split's whole point is chronological order. Leaving
`shuffle` at its default `true` fails every fold identically.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 22.0, 24.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::TSCrossVal;
  split.folds = 3;
  split.shuffle = false;

  const cls::CrossAssessReport report = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, split).value();
  std::cout << "fold count = " << report.folds.size() << "\n";
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << report.folds[i].test_indices << "\n";
  }

  cls::SplitOptions shuffled = split;
  shuffled.shuffle = true;
  const cls::StatusOr<cls::CrossAssessReport> rejected = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, shuffled);
  std::cout << "shuffle=true ok = " << std::boolalpha << rejected.ok() << "\n";

  return 0;
}
```

Output

```text
fold count = 3
fold 0 test_indices = pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 4, 5]
fold 1 test_indices = pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[6, 7, 8]
fold 2 test_indices = pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[9, 10, 11]
shuffle=true ok = false
```

## Changelog

- **0.1** — `cross_assess`, `cross_scores`, `cross_score`.

## See Also

- [`CrossAssessReport`](CrossAssessReport.md) — the full returned report.
- [`SplitOptions`](../split/SplitOptions.md) — every field this dispatches
  on.
- [`EstimatorBase::learn`](../estimators/EstimatorBase/learn.md) /
  [`assess`](../estimators/EstimatorBase/assess.md) — what runs once per
  fold.
- [`tune`](../tune.md) — hyperparameter search built on the same
  cross-validated scoring.
