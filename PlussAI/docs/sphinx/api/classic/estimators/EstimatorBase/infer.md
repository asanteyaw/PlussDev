<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f630b95cd9c8 -->
# `pluss::ai::classic::EstimatorBase::infer`

`infer` predicts from a trained estimator: hard class labels for a
classifier, continuous values for a regressor. For a classifier's raw
decision scores instead of hard labels, use
[`ClassifierBase::infer_scores`](../ClassifierBase.md).

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::EstimatorBase::infer(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;
```

## What It Does

Runs `batch` through this already-trained estimator's fitted parameters
and produces one prediction per row, aligned to `batch`'s own row order --
hard class labels for a classifier, continuous values for a regressor.
Requires `learn` to have already succeeded; `batch` needs no label column
at all, since `infer` never reads one.

## Parameters

`batch` — `FeatureBatch`
: The features to predict from. See [`FeatureBatch`](../../data/FeatureBatch.md).
  No label is needed — `infer` never reads one.

`options` — `InferOptions`, default `{}`
: Prediction-time configuration — chiefly `positive_threshold` for binary
  classification. See [`InferOptions`](../../core/InferOptions.md).

## Returns

A `StatusOr<Series>` — one predicted value per sample, aligned to `batch`'s
row order. For a binary classifier, `0`/`1` (or the two label values training
used); for a regressor, the predicted continuous value.

## Errors

- `Status::precondition` — the estimator is not yet trained (`learn` has not
  succeeded).
- `Status::shape_mismatch` — `batch`'s feature count does not match what the
  estimator was trained on.

## Practical Examples

### Example 1

Predicting hard labels from a trained classifier.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  const cls::StatusOr<pluss::Series> pred = made.value()->infer(batch.features);
  std::cout << "ok = " << std::boolalpha << pred.ok() << "\n";
  std::cout << "predictions = " << pred.value() << "\n";

  return 0;
}
```

Output

```text
ok = true
predictions = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 2

Predicting continuous values from a trained regressor, on data the model
never saw during training.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  made.value()->learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0, 6.0}, {2}, f64).reshape({2, 1});
  const cls::StatusOr<pluss::Series> pred = made.value()->infer(probe);
  std::cout << "predictions for x = [5, 6] = " << pred.value() << "\n";

  return 0;
}
```

Output

```text
predictions for x = [5, 6] = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[9.999999, 11.999999]
```

### Example 3

`infer` on an untrained estimator, and on a feature count the estimator was
not trained with.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  const cls::StatusOr<pluss::Series> untrained = made.value()->infer(probe);
  std::cout << "untrained: ok = " << std::boolalpha << untrained.ok() << ", message = \""
            << untrained.status().message << "\"\n";

  const cls::Series x = pluss::make_series({1.0, 2.0, 1.0, 2.0}, {4}, f64).reshape({2, 2});
  const cls::Series y = pluss::make_series({3.0, 5.0}, {2}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  made.value()->learn(batch);  // trained on 2 features

  cls::FeatureBatch wrong_shape{};
  wrong_shape.x = probe.x;  // only 1 feature
  const cls::StatusOr<pluss::Series> mismatched = made.value()->infer(wrong_shape);
  std::cout << "wrong feature count: ok = " << mismatched.ok() << ", message = \""
            << mismatched.status().message << "\"\n";

  return 0;
}
```

Output

```text
untrained: ok = false, message = "Estimator must be learned before infer()."
wrong feature count: ok = false, message = "Feature count mismatch between learn and inference batch."
```

## See Also

- [`ClassifierBase::infer_scores`](../ClassifierBase.md) — raw decision
  scores instead of hard labels.
- [`learn`](learn.md) — must succeed before `infer` can.
- [`InferOptions`](../../core/InferOptions.md) — every field, including
  `positive_threshold`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dump_state.html">
    <span class="label">Previous</span>
    <span class="title">dump_state</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
