# `lymph::validate::cross`

## Summary

`validate::cross(...)` creates a reusable cross-validation plan for model-oriented
evaluation.

Use it when you want:

- ordinary k-fold validation
- repeated cross-validation
- a reusable validation contract that can be applied to several models or
  workflows

## What It Means

`validate::cross(...)` is the normal IID-style evidence builder in Lymph.

Its job is not to return one score immediately. Its job is to define the fold
policy once so that you can then:

- score one model quickly
- inspect fold-to-fold stability
- materialize a validation report
- reuse the same policy across several candidate models

That separation is important because it keeps the validation contract stable while
you swap modeling objects.

## High-Value Options

The main toggles you will usually care about are:

- `folds`
- `repeats`
- `shuffle`
- `random_state`
- `deterministic`
- `max_parallel`

### `folds`

Controls how many train/test rotations are used.

Higher fold counts usually mean:

- more evidence
- more fit cost

### `repeats`

Repeats the fold process for a more stable estimate.

Use it when one fold realization feels too noisy and you are willing to pay the
extra runtime.

### `shuffle`

Usually `true` for ordinary cross-validation.

If row order carries meaning, `shuffle` becomes a semantic decision rather than a
mere convenience toggle.

### `random_state` and `deterministic`

These together control reproducibility.

For serious experiment tracking, reproducible validation is usually the right
default.

### `max_parallel`

Caps fold-level worker concurrency.

This changes runtime behavior, not the statistical meaning of the validation
contract.

## Signature

```cpp
lymph::ValidationPlan lymph::validate::cross(
    lymph::ValidationOptions options = {});
lymph::metrics::CrossValidationReport lymph::ValidationPlan::run(...) const;
pluss::Series lymph::ValidationPlan::scores(...) const;
double lymph::ValidationPlan::score(...) const;
```

## Practical Examples

The examples below show the normal progression:

1. define the fold policy
2. use scalar and fold-score terminals
3. materialize the full report when you need a board

### Example 1

This example builds a standard cross-validation plan, then uses the scalar
`score(...)` terminal for one metric and the fold-level `scores(...)` terminal for
another.

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

The key lesson is not the perfect score on this toy data. The key lesson is the
shape of the evidence:

- `score(...)` gives one objective number
- `scores(...)` gives the full fold series
- both come from the same plan object

### Example 2

This follow-up example stays with the same `frame`, `data_options`, `train_options`,
`assess_options`, `logistic`, and `plan`, but moves from quick terminals to the
full report object.

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

This is the point where `cross(...)` stops being “just a splitter” and becomes a
real validation surface. The same fold policy can feed:

- one scalar objective
- one fold-score series
- one report board

## When Not To Use It

Do not use `cross(...)` when:

- time order matters
- chronological leakage is possible
- entity grouping must be preserved explicitly

In those cases, use a more appropriate validation builder such as [time](time.md)
or a grouped plan.

## Related Pages

- [ValidationOptions](../metrics/ValidationOptions.md)
- [Plan](Plan.md)
