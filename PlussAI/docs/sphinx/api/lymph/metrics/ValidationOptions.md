# `lymph::metrics::ValidationOptions`

## Summary

`ValidationOptions` is the public options bag that describes how evidence should be
earned before a model is trusted.

You normally pass it into validation builders such as:

- `lymph::validate::cross(...)`
- `lymph::validate::time(...)`
- `lymph::validate::holdout(...)`

That means its ownership and usage are intentionally split:

- `metrics` owns the reusable public contract object
- `validate` consumes that contract to build executable plans

## Design Role

Do not think of `ValidationOptions` as a miscellaneous config dump.

Its role is precise:

- declare the validation regime
- hold the regime-specific knobs
- keep those knobs stable across repeated runs
- make cross, holdout, grouped, and time validation feel like one public contract

That is why this object lives at the boundary between modeling intent and earned
evidence.

## Field Reference

### `plan`

The underlying validation regime.

Default:

- `CVPlanKind::CrossVal`

In normal usage you should let the builder establish this:

- `validate::cross(...)`
- `validate::time(...)`
- `validate::holdout(...)`

Do not pair a conflicting raw `plan` value with a builder that means something
else.

### `holdout_fraction`

The fraction reserved for the test slice in holdout-style validation.

Default:

- `0.2`

Effects:

- larger values produce a larger test slice and less training data
- smaller values preserve more training data and less held-out evidence

### `folds`

How many folds to use for cross-style plans.

Default:

- `5`

Effects:

- more folds usually give denser evidence
- more folds also increase fit cost
- time plans use fold count to determine how many chronological windows exist

### `repeats`

How many times the fold process is repeated.

Default:

- `1`

Use this when one fold realization is too brittle and you want a more stable
estimate across several realizations.

### `time_gap`

How much empty space to leave between the end of the training block and the start
of the test block in time-aware validation.

Default:

- `0`

Use it when near-neighbor leakage is a credible risk.

### `time_test_size`

The width of each time-aware test window.

Default:

- `0`

If `0`, the downstream planning surface may infer or derive a size. Set it
explicitly when the business horizon matters and should be encoded directly.

### `time_max_train_size`

An optional cap on the training window in time-aware validation.

Default:

- `0`

Use it to move from an expanding-history regime toward a rolling-window regime.

### `stratified`

Whether label balance should be preserved across folds when that is legal.

Default:

- `false`

This matters most for imbalanced classification.

### `groups`

The group labels used for grouped validation.

Default:

- undefined `Series`

Use it when rows sharing an entity boundary must stay together:

- customers
- accounts
- patients
- devices
- sessions

### `reliability_defaults`

Whether lower split-layer reliability defaults should remain active.

Default:

- `true`

Most users should leave this alone unless they intentionally need to override the
lower-level default policy.

### `shuffle`

Whether row order should be randomized before fold construction where that is
legal.

Default:

- `true`

Effects:

- `true` is typical for IID-style cross-validation
- `false` preserves incoming order
- time-aware validation should not shuffle

### `max_parallel`

The fold-level worker cap.

Default:

- `0`

Semantics:

- `0` means auto-size
- `1` forces serial execution
- positive values cap worker count explicitly

This changes runtime, not validation meaning.

### `random_state`

The seed for validation-time randomness where randomness is allowed.

Default:

- `0`

Use a non-zero seed when you want reproducibility that survives repeated runs.

### `deterministic`

Whether randomized behavior should remain reproducible from the supplied seed.

Default:

- `true`

For serious experiments, keep this `true` unless you explicitly want unconstrained
stochastic behavior.

## Signature

```cpp
lymph::ValidationOptions options{};
lymph::ValidationPlan lymph::validate::cross(
    lymph::ValidationOptions options = {});
lymph::ValidationPlan lymph::validate::time(
    lymph::ValidationOptions options = {});
lymph::HoldoutPlan lymph::validate::holdout(...);
```

## Practical Example

This page uses the exact verifier program `lymph_validate_docverify_demo.cpp`.

### Example 1

This example configures one cross-validation contract, turns it into a validation
plan, and prints both the plan-state summary and the resulting score surfaces.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classic = pluss::ai::classic;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
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

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  classic::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = classic::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);

  lymph::ValidationOptions cross_options{};
  cross_options.folds = 4;
  cross_options.repeats = 1;
  cross_options.shuffle = true;
  cross_options.random_state = 20260825ULL;
  cross_options.deterministic = true;
  cross_options.max_parallel = 1;

  const lymph::ValidationPlan cross_plan = lymph::validate::cross(cross_options);
  std::cout << "== validation_options_cross ==\n";
  std::cout << "plan=" << static_cast<int>(cross_plan.options().plan)
            << " folds=" << cross_plan.options().folds
            << " shuffle=" << cross_plan.options().shuffle
            << " seed=" << cross_plan.options().random_state
            << " deterministic=" << cross_plan.options().deterministic
            << " workers=" << cross_plan.options().max_parallel << "\n";
  std::cout << "score(accuracy)="
            << cross_plan.score(
                   logistic,
                   frame,
                   data_options,
                   lymph::MetricKind::Accuracy,
                   train_options,
                   assess_options)
            << "\n";
  std::cout << "scores(f1)="
            << cross_plan.scores(
                   logistic,
                   frame,
                   data_options,
                   lymph::MetricKind::F1,
                   train_options,
                   assess_options)
            << "\n";

  return 0;
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

This shows the real job of `ValidationOptions`:

- it is not inert metadata
- it becomes an active validation contract
- the builder and execution terminals actually honor it

## Related Pages

- [validate](../split.md)
- [Plan](../split/Plan.md)
- [cross](../split/cross.md)
- [time](../split/time.md)
- [holdout](../split/holdout.md)
