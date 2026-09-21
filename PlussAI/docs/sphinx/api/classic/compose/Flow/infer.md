<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2e5f50357315 -->

# `pluss::ai::classic::Flow::infer`

`infer` maps a batch through every fitted transform stage, then hands the
result to the attached model's own `infer` — a prediction (a class label, or
a continuous value) exactly as [`EstimatorBase::infer`](../../estimators/EstimatorBase/infer.md)
would return, except the caller supplies raw features and never touches the
intermediate transformed representation.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::infer(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;

[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::infer(
    const pluss::table::DataFrame& frame,
    const classic::DataFrameBatchOptions& data_options,
    const classic::InferOptions& options = {}) const;
```

## Parameters

`batch` — `FeatureBatch`
: Raw features — the same shape `learn`'s `LabeledBatch.features` had, before
  any stage ran.

`frame` / `data_options` — `DataFrame` / `DataFrameBatchOptions`
: Builds a `FeatureBatch` via [`feature_batch_from_dataframe`](../../data/feature_batch_from_dataframe.md)
  first, then calls the `FeatureBatch` overload.

`options` — `InferOptions`, default `{}`
: Forwarded to every stage's `map` and then to the model's own `infer`.

## Returns

A `StatusOr<Series>` — the model's prediction on the mapped features. Requires
**both** a model attached and a successful prior `learn` — see Errors.

## Errors

- `Status::precondition`, `"Flow requires a model before infer()."` — no
  model is attached (`has_model() == false`); a transform-only `Flow` has
  nothing to predict with, use [`map`](map.md) instead.
- `Status::precondition`, `"Flow must be learned before infer()."` —
  a model is attached but `learn` has not yet succeeded.
- Either stage-mapping or model-inference failures propagate their own
  `Status` — see [`map`](map.md#errors) and
  [`EstimatorBase::infer`](../../estimators/EstimatorBase/infer.md#errors).

## Practical Examples

The two failure preconditions in isolation, a successful predict call
through a scaled pipeline, and the `DataFrame` overload predicting from a
named column instead of a raw `FeatureBatch`.

### Example 1

`infer` on a `Flow` with no model attached — rejected before any stage runs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // no model
  flow.add_standard_scaler();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  const cls::StatusOr<pluss::Series> predicted = flow.infer(batch);
  std::cout << "ok = " << std::boolalpha << predicted.ok() << "\n";
  std::cout << "message = \"" << predicted.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow requires a model before infer()."
```

### Example 2

A `Flow` with a model attached, but never `learn`ed.

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

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  const cls::StatusOr<pluss::Series> predicted = flow.infer(batch);
  std::cout << "ok = " << std::boolalpha << predicted.ok() << "\n";
  std::cout << "message = \"" << predicted.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow must be learned before infer()."
```

### Example 3

A fit `Flow` — scale, then predict — `infer` on raw (unscaled) features.

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
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({50.0}, {1}, f64).reshape({1, 1});  // raw, unscaled
  const cls::StatusOr<pluss::Series> predicted = flow.infer(probe);
  std::cout << "ok        = " << std::boolalpha << predicted.ok() << "\n";
  std::cout << "predicted = " << predicted.value().item<double>() << "\n";

  return 0;
}
```

Output

```text
ok        = true
predicted = 5
```

### Example 4

The `DataFrame` overload — the same fitted `Flow` as Example 3, predicting
from a named-column `DataFrame` row instead of a raw `FeatureBatch`. Same
prediction, since [`feature_batch_from_dataframe`](../../data/feature_batch_from_dataframe.md)
builds the identical `FeatureBatch` internally before the `FeatureBatch`
overload runs.

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

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  const DataFrame frame({
      {"reading", Thread(pluss::make_series({50.0}, {1}, f64))},
  });
  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"reading"};

  const cls::StatusOr<pluss::Series> predicted = flow.infer(frame, data_options);
  std::cout << "ok        = " << std::boolalpha << predicted.ok() << "\n";
  std::cout << "predicted = " << predicted.value().item<double>() << "\n";

  return 0;
}
```

Output

```text
ok        = true
predicted = 5
```

## See Also

- [`infer_scores`](infer_scores.md) — raw scores instead of hard predictions,
  for classification.
- [`assess`](assess.md) — `infer` plus metrics against a known target.
- [`map`](map.md) — the stage-only half of this call.
- [`EstimatorBase::infer`](../../estimators/EstimatorBase/infer.md) — the
  model-side contract this delegates to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="graph_info.html">
    <span class="label">Previous</span>
    <span class="title">graph_info</span>
  </a>
  <a class="next-link" href="infer_scores.html">
    <span class="label">Next</span>
    <span class="title">infer_scores</span>
  </a>
</div>
<!-- apiref:nav:end -->
