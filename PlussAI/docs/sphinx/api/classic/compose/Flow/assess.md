<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0561ca876681 -->

# `pluss::ai::classic::Flow::assess`

`assess` maps a labeled batch's features through every fitted stage, then
scores the attached model's predictions against the target — the same
`AssessOptions`-driven metric computation as
[`EstimatorBase::assess`](../../estimators/EstimatorBase/assess.md), reachable
straight from raw features instead of a pre-transformed batch.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::assess(
    const classic::LabeledBatch& batch,
    const classic::AssessOptions& options = {}) const;

[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::assess(
    const pluss::table::DataFrame& frame,
    const classic::DataFrameBatchOptions& data_options,
    const classic::AssessOptions& options = {}) const;
```

## What It Does

Runs `batch`'s features through every fitted stage, feeds the transformed
result into the attached model's prediction step, then scores those
predictions against `batch`'s own target -- the same predict-then-score
sequence [`EstimatorBase::assess`](../../estimators/EstimatorBase/assess.md)
performs on a single estimator, reachable here straight from raw features
rather than a pre-transformed batch.

## Parameters

`batch` — `LabeledBatch`
: Raw features plus the target to score against — the same shape `learn`
  took.

`frame` / `data_options` — `DataFrame` / `DataFrameBatchOptions`
: Builds a `LabeledBatch` via [`labeled_batch_from_dataframe`](../../data/labeled_batch_from_dataframe.md)
  first, then calls the `LabeledBatch` overload.

`options` — `AssessOptions`, default `{}`
: Which metrics to compute and how — see
  [`AssessOptions`](../../core/AssessOptions.md). Only `options.layout`
  reaches the stage-mapping step; stage mapping otherwise always runs with
  `missing = KeepNaN` and `unknown_categorical = Ignore`, regardless of what
  `learn`'s own `TrainOptions` used — `assess` does not resurrect the
  training-time missing-value policy.

## Returns

A `StatusOr<Series>` — the requested metric values, aligned to
`options.metrics` (or the task's default set if empty), exactly as
[`EstimatorBase::assess`](../../estimators/EstimatorBase/assess.md) returns.

## Errors

- The same two preconditions as [`infer`](infer.md#errors) (`"Flow requires
  a model before assess()."` / `"Flow must be learned before assess()."`).
- `Status::shape_mismatch` from `validate_labeled` for an undefined or
  mismatched `y` — checked before stage mapping runs.
- Stage-mapping or the model's own `assess` failing propagates that
  `Status` directly — see [`EstimatorBase::assess`](../../estimators/EstimatorBase/assess.md#errors).

## Practical Examples

### Example 1

Scoring a fitted, scaled classification `Flow` on held-out data, with an
explicit metric list.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  cls::LabeledBatch train{};
  train.features.x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  train.y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  flow.learn(train);

  cls::LabeledBatch held_out{};
  held_out.features.x = pluss::make_series({1.1, 2.5}, {2}, f64).reshape({2, 1});
  held_out.y = pluss::make_series({0.0, 1.0}, {2}, f64);

  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Accuracy};
  const cls::StatusOr<pluss::Series> scored = flow.assess(held_out, opts);
  std::cout << "ok    = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "score = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
ok    = true
score = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

`assess` before `learn` — rejected with the same precondition `infer` uses.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  batch.y = pluss::make_series({1.0}, {1}, f64);
  const cls::StatusOr<pluss::Series> scored = flow.assess(batch);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "message = \"" << scored.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow must be learned before assess()."
```

### Example 3

The `DataFrame` overload — named columns resolved into a `LabeledBatch`
internally, then scored exactly as in Example 1.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();

  const DataFrame train_frame({
      {"x", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"y", Slate(pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64))},
  });
  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "y";
  flow.learn(train_frame, data_options);

  const DataFrame held_out({
      {"x", Slate(pluss::make_series({5.0}, {1}, f64))},
      {"y", Slate(pluss::make_series({10.0}, {1}, f64))},
  });
  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Mse};
  const cls::StatusOr<pluss::Series> scored = flow.assess(held_out, data_options, opts);
  std::cout << "ok    = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "score = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
ok    = true
score = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[9.999996e-13]
```

## See Also

- [`infer`](infer.md) — predictions alone, no metric computed.
- [`EstimatorBase::assess`](../../estimators/EstimatorBase/assess.md) — the
  model-side contract this delegates to.
- [`AssessOptions`](../../core/AssessOptions.md) — every configuration field.
- [`metrics`](../../metrics.md) — the individual metric functions `assess`
  composes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append_transform.html">
    <span class="label">Previous</span>
    <span class="title">append_transform</span>
  </a>
  <a class="next-link" href="clear_model.html">
    <span class="label">Next</span>
    <span class="title">clear_model</span>
  </a>
</div>
<!-- apiref:nav:end -->
