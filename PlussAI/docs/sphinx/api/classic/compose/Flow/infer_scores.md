<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eb73ad1e07e4 -->

# `pluss::ai::classic::Flow::infer_scores`

`infer_scores` maps a batch through every fitted stage and hands the result
to the attached classifier's [`infer_scores`](../../estimators/ClassifierBase.md)
— the raw per-class score rather than the hard label `infer` returns.
Classification-only: a `Flow` with a regressor or no model rejects the call
outright.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::infer_scores(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;

[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::infer_scores(
    const pluss::table::DataFrame& frame,
    const classic::DataFrameBatchOptions& data_options,
    const classic::InferOptions& options = {}) const;
```

## What It Does

The scores counterpart to [`infer`](infer.md): runs `batch` through the
same fitted-stage pipeline, then hands the transformed result to the
attached classifier's own `infer_scores` instead of `infer` -- raw
per-class decision scores rather than a hard label. Only meaningful for a
`Flow` whose attached model is a classifier; a regressor or no model at
all rejects the call outright.

## Parameters

`batch` — `FeatureBatch`
: Raw features — the same shape [`infer`](infer.md#parameters) takes.

`frame` / `data_options` — `DataFrame` / `DataFrameBatchOptions`
: Builds a `FeatureBatch` via [`feature_batch_from_dataframe`](../../data/feature_batch_from_dataframe.md)
  first, then calls the `FeatureBatch` overload — identical to how
  [`infer`](infer.md#parameters)'s own `DataFrame` overload bridges.

`options` — `InferOptions`, default `{}`
: Forwarded to every stage's `map` and then to the classifier's own
  `infer_scores`.

## Returns

A `StatusOr<Series>` — the classifier's raw scores on the mapped features.

## Errors

- The same two preconditions as [`infer`](infer.md#errors) (`"Flow requires
  a model before infer_scores()."` / `"Flow must be learned before
  infer_scores()."`).
- `Status::invalid_argument`, `"Flow::infer_scores is available for
  classification flows only."` — the attached model is a regressor. Checked
  after the model/trained preconditions, before any stage runs.
- Stage-mapping failures propagate as in [`map`](map.md#errors).

## Practical Examples

### Example 1

A classification `Flow`: `infer_scores` alongside `infer`, showing the raw
score behind the hard label.

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
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({2.1}, {1}, f64).reshape({1, 1});
  std::cout << "infer_scores = " << flow.infer_scores(probe).value() << "\n";
  std::cout << "infer        = " << flow.infer(probe).value() << "\n";

  return 0;
}
```

Output

```text
infer_scores = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[12.443641]
infer        = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

`infer_scores` on a regression `Flow` — rejected, since there is no
per-class score to return.

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
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({2.0}, {1}, f64).reshape({1, 1});
  const cls::StatusOr<pluss::Series> scored = flow.infer_scores(probe);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "message = \"" << scored.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::infer_scores is available for classification flows only."
```

### Example 3

The `DataFrame` overload — named columns resolved into a `FeatureBatch`
internally, then scored exactly as in Example 1.

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

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();

  const DataFrame train_frame({
      {"x", Thread(pluss::make_series({1.0, 1.2, 2.0, 2.2}, {4}, f64))},
      {"label", Thread(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });
  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "label";
  flow.learn(train_frame, data_options);

  const DataFrame probe_frame({{"x", Thread(pluss::make_series({2.1}, {1}, f64))}});
  const cls::StatusOr<pluss::Series> scored = flow.infer_scores(probe_frame, data_options);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "scores = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
ok = true
scores = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[11.36871]
```

## See Also

- [`infer`](infer.md) — the hard-label counterpart.
- [`assess`](assess.md) — metrics that consume these same raw scores
  (`RocAuc`, `Gini`, …) directly.
- [`ClassifierBase::infer_scores`](../../estimators/ClassifierBase.md) — the
  model-side contract this delegates to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer.html">
    <span class="label">Previous</span>
    <span class="title">infer</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
