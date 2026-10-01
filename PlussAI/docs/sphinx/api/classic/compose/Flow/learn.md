<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 931661c60724 -->

# `pluss::ai::classic::Flow::learn`

`learn` fits every transform stage and, if one is attached, the final model —
in that order, on the same call. It is the one call that turns an assembled
`Flow` (stages added, a model attached or not) into a usable one:
`trained()` becomes `true` only once every stage and the model (if any) has
fit successfully.

## Signature

```cpp
classic::Status classic::Flow::learn(
    const classic::LabeledBatch& batch,
    const classic::TrainOptions& options = {});

classic::Status classic::Flow::learn(
    const pluss::table::DataFrame& frame,
    const classic::DataFrameBatchOptions& data_options,
    const classic::TrainOptions& options = {});
```

## What It Does

Fits every transform stage, in order, on `batch`'s features, feeding each
stage's output forward into the next; once every stage has fit, fits the
attached model (if any) on whatever the final stage produced. This single
call is what turns an assembled `Flow` -- stages added, a model attached
or not -- into one where `trained()` is `true` and
[`infer`](infer.md)/[`assess`](assess.md) can run.

## Parameters

`batch` — `LabeledBatch`
: The features and target to fit — stages first, then the model, all on this
  same data. See [`LabeledBatch`](../../data/LabeledBatch.md).

`frame` / `data_options` — `DataFrame` / `DataFrameBatchOptions`
: The `DataFrame` overload builds a `LabeledBatch` via
  [`labeled_batch_from_dataframe`](../../data/labeled_batch_from_dataframe.md)
  first, then calls the `LabeledBatch` overload — identical behavior from
  that point on.

`options` — `TrainOptions`, default `{}`
: Forwarded to every stage's `learn`/`learn_supervised` call and to the
  model's `learn` call. `options.missing` / `options.unknown_categorical`
  also seed the `InferOptions` used internally to `map` each stage's output
  forward to the next stage (see [`TrainOptions`](../../core/TrainOptions.md)).

## Returns

A `Status`. `.ok()` on success, and `trained()` is `true` immediately
afterward. A `Flow` with no model attached still succeeds here — its stages
fit, `trained()` becomes `true`, and it is usable as a transform-only
pipeline via [`map`](map.md) (not `infer`, which requires a model — see
[`infer`](infer.md)).

## Errors

- A stage or the model's own `learn`/`learn_supervised` failing propagates
  that `Status` directly (e.g. a routed stage's feature count not matching
  its route, or a classifier's family-specific validation — see
  [`EstimatorBase::learn`](../../estimators/EstimatorBase/learn.md#errors)
  for the model-side failure modes).
- A stage that overrides `needs_target()` to `true` (a supervised prep stage,
  such as a target/WOE encoder) but is fit through a plain
  [`learn_map`](learn_map.md)/[`map`](map.md)-only path with no target
  attached fails with an explicit message directing you to attach it via
  `Flow::learn`'s `LabeledBatch` instead.
- `Status::shape_mismatch` from `validate_labeled` for an undefined `y` or a
  `y` length that does not match the sample count — checked before any stage
  runs.
- Every call re-fits every stage and the model from scratch: calling `learn`
  again discards the previous fit entirely, it does not resume or average.

## Practical Examples

A transform-and-model pipeline fit in one call, a transform-only `Flow` (no
model — `learn` only fits its stages), and the `DataFrame` overload.

### Example 1

A `Flow` with one scaling stage and a classifier — `learn` fits the scaler
first, then the classifier on the scaler's output, all from one call.

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

  const cls::Series x = pluss::make_series(
      {10.0, 12.0, 14.0, 16.0, 100.0, 102.0, 104.0, 106.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  std::cout << "trained() before learn = " << std::boolalpha << flow.trained() << "\n";
  const cls::Status learned = flow.learn(batch);
  std::cout << "learn ok = " << learned.ok() << "\n";
  std::cout << "trained() after learn  = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
trained() before learn = false
learn ok = true
trained() after learn  = true
```

### Example 2

A transform-only `Flow` — no model attached, built from an empty `Flow{}`
rather than `make_flow`. `learn` here only fits the one stage;
[`has_model()`](../Flow.md) stays `false` throughout, but `trained()` still
becomes `true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // empty -- no model
  flow.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4}, f64);  // ignored -- no model to fit

  const cls::Status learned = flow.learn(batch);
  std::cout << "learn ok    = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "trained()   = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
learn ok    = true
has_model() = false
trained()   = true
```

### Example 3

The `DataFrame` overload — named columns resolved into a `LabeledBatch`
internally, then fit exactly as in Example 1.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Thread;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.5, 2.0, 2.5}, {4}, f64))},
      {"label", Thread(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });

  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income"};
  data_options.label_feature = "label";

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();

  const cls::Status learned = flow.learn(frame, data_options);
  std::cout << "learn ok  = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "trained() = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
learn ok  = true
trained() = true
```

## See Also

- [`map`](map.md) / [`learn_map`](learn_map.md) — stage-only fitting for a
  transform-only `Flow`, without a model.
- [`infer`](infer.md) / [`infer_scores`](infer_scores.md) /
  [`assess`](assess.md) — what you can call once `learn` succeeds.
- [`EstimatorBase::learn`](../../estimators/EstimatorBase/learn.md) — the
  model-side contract this delegates to.
- [`TrainOptions`](../../core/TrainOptions.md) — every configuration field.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer_scores.html">
    <span class="label">Previous</span>
    <span class="title">infer_scores</span>
  </a>
  <a class="next-link" href="learn_map.html">
    <span class="label">Next</span>
    <span class="title">learn_map</span>
  </a>
</div>
<!-- apiref:nav:end -->
