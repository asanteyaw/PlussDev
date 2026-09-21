<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 74690e29ddfc -->
# `pluss::ai::classic::EstimatorBase::assess`

`assess` runs inference and scores it against known labels in one call —
`infer` (or `infer_scores`, for whichever metrics need them) plus the
matching [`metrics`](../../metrics.md) computation, folded together so you
never thread a prediction `Series` through by hand.

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::EstimatorBase::assess(
    const classic::LabeledBatch& batch,
    const classic::AssessOptions& options = {}) const;
```

## Parameters

`batch` — `LabeledBatch`
: The features and ground-truth labels to score against. See
  [`LabeledBatch`](../../data/LabeledBatch.md).

`options` — `AssessOptions`, default `{}`
: Which metrics to compute (`options.metrics`), plus the threshold and
  weighting fields — see [`AssessOptions`](../../core/AssessOptions.md).
  An empty `metrics` list falls back to the task's default set.

## Returns

A `StatusOr<Series>` of scalar metric values, one per entry of
`options.metrics` (or the task default), in that order.

## Errors

- `Status::precondition` — the estimator is not yet trained.
- `Status::invalid_argument` — a requested metric does not apply to the
  estimator's task (a regression metric on a classifier, or vice versa).

## Practical Examples

### Example 1

The task-default metric set — an empty `AssessOptions{}`.

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

  const cls::StatusOr<pluss::Series> scored = made.value()->assess(batch);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "default metrics = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
ok = true
default metrics = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.9999992e-13, 4.4721351e-07, 1]
```

### Example 2

An explicit classification metric list.

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

  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Accuracy, cls::MetricKind::Mcc};
  const cls::StatusOr<pluss::Series> scored = made.value()->assess(batch, opts);
  std::cout << "accuracy, mcc = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
accuracy, mcc = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

### Example 3

`assess` on an untrained estimator.

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

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  batch.y = pluss::make_series({1.0}, {1}, f64);

  const cls::StatusOr<pluss::Series> scored = made.value()->assess(batch);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "message = \"" << scored.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Estimator must be learned before assess()."
```

## See Also

- [`infer`](infer.md) — what `assess` runs internally before scoring.
- [`AssessOptions`](../../core/AssessOptions.md) — every field.
- [`metrics`](../../metrics.md) — the compute layer `assess` delegates to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="dump_state.html">
    <span class="label">Next</span>
    <span class="title">dump_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
