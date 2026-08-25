# `lymph::validate::Plan`

## Summary

`Plan` is the reusable validation planner in Lymph.

It separates two concerns that users too often mix together:

- what validation policy you want
- what model or workflow you want to test under that policy

That separation is one of the main reasons Lymph scales better than ad-hoc
experiment scripts.

## Signature

```cpp
lymph::ValidationPlan lymph::validate::cross(
    lymph::ValidationOptions options = {});
lymph::ValidationPlan lymph::validate::time(
    lymph::ValidationOptions options = {});
lymph::ValidationPlan lymph::validate::holdout(
    lymph::ValidationOptions options = {});

const lymph::ValidationOptions& lymph::ValidationPlan::options() const;

lymph::ValidationPlan& lymph::ValidationPlan::folds(std::int64_t value);
lymph::ValidationPlan& lymph::ValidationPlan::repeat(std::int64_t value);
lymph::ValidationPlan& lymph::ValidationPlan::shuffle(bool value = true);
lymph::ValidationPlan& lymph::ValidationPlan::seed(std::uint64_t value);
lymph::ValidationPlan& lymph::ValidationPlan::workers(std::int64_t value);
lymph::ValidationPlan& lymph::ValidationPlan::gap(std::int64_t value);
lymph::ValidationPlan& lymph::ValidationPlan::horizon(std::int64_t value);
lymph::ValidationPlan& lymph::ValidationPlan::lookback(std::int64_t value);

lymph::metrics::CrossValidationReport lymph::ValidationPlan::run(...) const;
std::vector<lymph::TimeSeriesSplit> lymph::ValidationPlan::splits(
    const pluss::table::DataFrame& frame,
    std::string_view time_feature) const;
lymph::TimeSeriesSplit lymph::ValidationPlan::last(
    const pluss::table::DataFrame& frame,
    std::string_view time_feature) const;
pluss::Series lymph::ValidationPlan::scores(...) const;
double lymph::ValidationPlan::score(...) const;
```

## Core Idea

`Plan` is a validation object, not a model object.

That means you can define the validation policy once and then run several compatible
modeling objects under the same contract. That is exactly the right pattern when
you care about fairness, auditability, and reproducibility.

## Construction

You normally do not instantiate `Plan` directly in user code.

You enter through a builder:

- `lymph::validate::cross(...)`
- `lymph::validate::stratified(...)`
- `lymph::validate::grouped(...)`
- `lymph::validate::time(...)`
- `lymph::validate::holdout(...)`

Those builders communicate intent much better than manually setting a raw plan kind.

## Mutator Reference

### `folds(value)`

Sets the fold count.

Constraint:

- `value` must be greater than `1`

### `repeat(value)`

Sets how many times fold evaluation is repeated.

Constraint:

- `value` must be positive

### `shuffle(value = true)`

Controls whether the plan may shuffle rows when that plan kind allows it.

Chronological plans reject shuffling because it would violate the time contract.

### `seed(value)`

Seeds randomized fold construction while preserving deterministic intent.

### `workers(value)`

Caps fold-parallel workers.

This is a runtime control, not a statistical one.

### `gap(value)`

Sets the train/test temporal buffer for time-aware plans.

Constraint:

- `value` cannot be negative

### `horizon(value)`

Sets the time-window test size.

Constraint:

- `value` cannot be negative

### `lookback(value)`

Caps the time-aware training window.

Constraint:

- `value` cannot be negative

## Execution Surfaces

The strength of `Plan` is that it can execute against several Lymph-level objects
without forcing you to redesign the validation code each time.

### `run(...)`

`run(...)` is the full-report terminal.

It returns a `lymph::metrics::CrossValidationReport`, which is what you want when
you need:

- fold-level detail
- metric means
- metric standard deviations
- a board you can inspect or export

### `scores(...)`

`scores(...)` is the fold-series terminal.

Use it when you want the per-fold values for one metric and plan to inspect or
chart them directly.

### `score(...)`

`score(...)` is the scalar terminal.

Use it when you want one objective number quickly.

### `splits(frame, time_feature)` and `last(frame, time_feature)`

These are the time-specific helpers.

Use them when you want the actual split artifacts rather than only model-evaluation
results.

## Practical Examples

These examples show the three most important ways to think about `Plan`:

1. one reusable policy object with scalar and fold terminals
2. one full report terminal
3. one chronological split helper

### Example 1

This example defines one reusable cross-validation plan and uses both `score(...)`
and `scores(...)` without restating the fold policy.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}))},
      {"late", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
      {"target", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  lymph::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = lymph::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);

  lymph::ValidationOptions opts{};
  opts.folds = 4;
  opts.repeats = 1;
  opts.shuffle = true;
  opts.random_state = 20260825ULL;
  opts.deterministic = true;
  opts.max_parallel = 1;

  const lymph::ValidationPlan plan = lymph::validate::cross(opts);

  std::cout << "== validation_options_cross ==\n";
  std::cout << "plan=" << static_cast<int>(plan.options().plan)
            << " folds=" << plan.options().folds
            << " shuffle=" << plan.options().shuffle
            << " seed=" << plan.options().random_state
            << " deterministic=" << plan.options().deterministic
            << " workers=" << plan.options().max_parallel << "\n";
  std::cout << "score(accuracy)="
            << plan.score(logistic, frame, data_options, lymph::MetricKind::Accuracy,
                          train_options, assess_options)
            << "\n";
  std::cout << "scores(f1)="
            << plan.scores(logistic, frame, data_options, lymph::MetricKind::F1,
                           train_options, assess_options)
            << "\n";
}
```

Output

```text
== validation_options_cross ==
plan=1 folds=4 shuffle=1 seed=20260825 deterministic=1 workers=1
score(accuracy)=1
scores(f1)=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 1]
```

That is the real value of `Plan`: one policy object, several compatible terminals,
and no need to restate fold semantics every time you evaluate another candidate.

### Example 2

This follow-up example keeps the same `plan`, `logistic`, `frame`, `data_options`,
`train_options`, and `assess_options`, but switches to the full-report terminal.

```cpp
const lymph::metrics::CrossValidationReport report =
    plan.run(logistic, frame, data_options, train_options, assess_options);

std::cout << "== cross_validation_report ==\n";
std::cout << report.board() << "\n";
```

Output

```text
== cross_validation_report ==
============================
     | metric    mean  stdev
----------------------------
0    | accuracy  1     0
1    | f1        1     0
----------------------------
[2 cs x 3 feat]
```

That is the point where `Plan` becomes a reporting surface rather than only a
scoring surface.

### Example 3

This third example switches to a time-aware plan and uses `last(...)` to extract
the final chronological block directly.

```cpp
const DataFrame timed_frame({
    {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}))},
    {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}))},
    {"late", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
    {"target", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
    {"when", Thread(pluss::make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-02T00:00:00Z",
        "2026-01-03T00:00:00Z",
        "2026-01-04T00:00:00Z",
        "2026-01-05T00:00:00Z",
        "2026-01-06T00:00:00Z",
        "2026-01-07T00:00:00Z",
        "2026-01-08T00:00:00Z",
    }))},
});

lymph::ValidationOptions time_opts{};
time_opts.folds = 3;
time_opts.shuffle = false;
time_opts.time_gap = 0;
time_opts.time_test_size = 2;
time_opts.time_max_train_size = 4;

const lymph::ValidationPlan time_plan =
    lymph::validate::time(time_opts);
const lymph::prep::TimeSeriesSplit final_block = time_plan.last(timed_frame, "when");

std::cout << "== time_plan_last ==\n";
std::cout << "train_rows=" << final_block.train_set.ncases()
          << " test_rows=" << final_block.test_set.ncases() << "\n";
std::cout << "train_last_when=" << final_block.train_set.features("when").tail(1)
          << " test_first_when=" << final_block.test_set.features("when").head(1) << "\n";
```

Output

```text
== time_plan_last ==
train_rows=4 test_rows=2
train_last_when=5  2026-01-06
Name: when, ptype: Timeline
 test_first_when=6  2026-01-07
Name: when, ptype: Timeline
```

This is the concrete proof that `last(...)` is not just “some final fold.” It is
the most recent chronological block, with train rows ending before the first test
row.

## Time-Aware Guardrails

The plan surface enforces important safety rules:

- time validation cannot shuffle
- time-aware evaluations require an explicit time feature
- workflow time settings and payload time settings must remain compatible

These are not cosmetic checks. They exist to prevent silent leakage and semantic
misalignment.

## Related Pages

- [ValidationOptions](../metrics/ValidationOptions.md)
- [cross](cross.md)
- [time](time.md)
- [holdout](holdout.md)
