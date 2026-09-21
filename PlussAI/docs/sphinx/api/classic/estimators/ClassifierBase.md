# `pluss::ai::classic::ClassifierBase`

`ClassifierBase` is [`EstimatorBase`](EstimatorBase.md) fixed to
`TaskKind::Classification`, plus one method neither `EstimatorBase` nor
[`RegressorBase`](RegressorBase.md) has: `infer_scores`, the raw decision
score behind every hard-label prediction. [`make_classifier`](make_classifier.md)
is the only way to obtain one.

## Signature

```cpp
// Everything EstimatorBase declares (learn, infer, assess, task_kind,
// trained, dump_state, load_state) applies unchanged -- see EstimatorBase.
// task_kind() is fixed:
classic::TaskKind classic::ClassifierBase::task_kind() const noexcept;  // always Classification

// The one method this adds:
classic::StatusOr<pluss::Series> classic::ClassifierBase::infer_scores(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;
```

## Construction

Obtained only from [`make_classifier`](make_classifier.md), which requires a
`ModelSpec` with `.task == TaskKind::Classification`. You hold it as
`std::unique_ptr<ClassifierBase>` (or a reference/pointer to that).

## State

Everything [`EstimatorBase`](EstimatorBase.md#state) exposes, unchanged.
`task_kind()` always returns `Classification` regardless of which concrete
family (`Linear`, `Tree`, `Kernel`, …) implements it.

## Methods

Every [`EstimatorBase`](EstimatorBase.md#methods) method — `learn`, `infer`,
`assess`, `task_kind`, `trained`, `dump_state`, `load_state` — applies
unchanged; see that page for each.

`infer_scores(batch, options = {})`
: The raw decision score behind `infer`'s hard-label decision — unbounded,
  not a `[0, 1]` probability, and not directly comparable to
  `InferOptions::positive_threshold` by hand (see
  [`InferOptions`](../core/InferOptions.md#positive_threshold)). Fails the
  same way `infer` does: `Status::precondition` if not yet trained,
  `Status::shape_mismatch` for a feature-count mismatch.

## Practical Example

`infer_scores` alongside `infer` — the raw score and the hard label it
produces, at three points spanning the decision boundary.

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

  cls::FeatureBatch probes{};
  probes.x = pluss::make_series({1.0, 1.8, 2.6}, {3}, f64).reshape({3, 1});

  std::cout << "task_kind() == Classification: " << std::boolalpha
            << (made.value()->task_kind() == cls::TaskKind::Classification) << "\n";
  std::cout << "raw scores  = " << made.value()->infer_scores(probes).value() << "\n";
  std::cout << "hard labels = " << made.value()->infer(probes).value() << "\n";

  return 0;
}
```

Output

```text
task_kind() == Classification: true
raw scores  = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-28.92268, -1.4210855e-14, 28.92268]
hard labels = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 1]
```

## Changelog

- **0.1** — the `EstimatorBase` contract fixed to `Classification`, plus
  `infer_scores`.

## See Also

- [`EstimatorBase`](EstimatorBase.md) — every inherited method, documented in
  full.
- [`RegressorBase`](RegressorBase.md) — the regression counterpart.
- [`make_classifier`](make_classifier.md) — how to obtain one.
- [`InferOptions`](../core/InferOptions.md) — `positive_threshold`, the field
  `infer` (not `infer_scores`) applies.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
