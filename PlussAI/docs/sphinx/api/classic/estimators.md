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

```cpp
namespace cls = pluss::ai::classic;

cls::LinearFamilyOptions linear{};
linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge

cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
    cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
if (!made.ok()) { /* made.status() */ }

cls::ClassifierBase& model = *made.value();
model.learn(batch);                       // Status
cls::Series pred = model.infer(x).value(); // StatusOr<Series>
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
