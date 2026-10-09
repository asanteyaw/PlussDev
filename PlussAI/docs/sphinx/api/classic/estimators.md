# `pluss::ai::classic::estimators`

`estimators` owns the contract every trainable object in `classic` implements
— `learn`, `infer`/`map`, `assess`, and state persistence — and the three
factory functions that turn a [`ModelSpec`](families/ModelSpec.md) into one.

[`families`](families.md) describes *what* to build; `estimators` is *what you
get back* and *how you drive it*.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`EstimatorBase`](estimators/EstimatorBase.md) | the shared contract: `learn` / `infer` / `assess` / `task_kind` / `trained` / `dump_state` / `load_state` |
| [`ClassifierBase`](estimators/ClassifierBase.md) | an `EstimatorBase` fixed to `TaskKind::Classification`, adding `infer_scores` |
| [`RegressorBase`](estimators/RegressorBase.md) | an `EstimatorBase` fixed to `TaskKind::Regression` |
| [`TransformerBase`](estimators/TransformerBase.md) | the separate `learn` / `map` / `learn_map` contract for stateful feature transforms |
| [`make_classifier`](estimators/make_classifier.md) | `ModelSpec` -> `StatusOr<std::unique_ptr<ClassifierBase>>` |
| [`make_regressor`](estimators/make_regressor.md) | `ModelSpec` -> `StatusOr<std::unique_ptr<RegressorBase>>` |
| [`make_transformer`](estimators/make_transformer.md) | `ModelSpec` -> `StatusOr<std::unique_ptr<TransformerBase>>` |

## How To Call It

Build a logistic classifier from a model spec, fit it, and predict. The program prints how many predictions came back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const cls::LabeledBatch batch = fixture_batch(cfg);
  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  if (!made.ok()) return 1;
  cls::ClassifierBase& model = *made.value();
  model.learn(batch);
  const cls::Series pred = model.infer(batch.features).value();
  std::cout << "predictions = " << pred.numel() << "\n";
  return 0;
}
```

Output

```text
predictions = 8
```

You always reach an `EstimatorBase` through one of these three factories,
holding it by `std::unique_ptr<ClassifierBase>` /
`std::unique_ptr<RegressorBase>` / `std::unique_ptr<TransformerBase>` — never
by value, since every concrete family type is a distinct polymorphic class
behind this shared interface.

## Related Pages

- [`families`](families.md) — `ModelSpec` and the nine `*FamilyOptions`
  structs `make_*` consumes.
- [`core`](core.md) — `TrainOptions` / `InferOptions` / `AssessOptions`, the
  configuration every `learn` / `infer` / `assess` call takes.
- [`data`](data.md) — `FeatureBatch` / `LabeledBatch`, the values these
  methods consume.
- [`compose`](compose.md) — `Flow`, which wraps a `ModelSpec`-built estimator
  inside a routed transform pipeline instead of driving it directly.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

estimators/Introduction
estimators/EstimatorBase
estimators/ClassifierBase
estimators/RegressorBase
estimators/TransformerBase
estimators/make_classifier
estimators/make_regressor
estimators/make_transformer
```
<!-- apiref:toctree:end -->
