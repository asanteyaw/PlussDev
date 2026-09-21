# `classic::estimators` Introduction

## What This Family Owns

`estimators` owns the one interface every trainable object in `classic`
implements, and the three functions that build one from a
[`ModelSpec`](../families/ModelSpec.md):

- [`EstimatorBase`](EstimatorBase.md) — the shared contract:
  `learn(LabeledBatch)` -> `Status`, `infer(FeatureBatch)` ->
  `StatusOr<Series>`, `assess(LabeledBatch)` -> `StatusOr<Series>`, plus
  `task_kind()`, `trained()`, and optional `dump_state()` / `load_state()`.
- [`ClassifierBase`](ClassifierBase.md) / [`RegressorBase`](RegressorBase.md) —
  `EstimatorBase` fixed to one task, so `make_classifier` and `make_regressor`
  can return a type-safe pointer instead of a bare `EstimatorBase`.
  `ClassifierBase` adds one method neither base has: `infer_scores`.
- [`TransformerBase`](TransformerBase.md) — a **separate** interface (it does
  not inherit `EstimatorBase`) for objects that map features to features
  rather than features to a label — `learn` / `map` / `learn_map` instead of
  `learn` / `infer` / `assess`.
- [`make_classifier`](make_classifier.md) / [`make_regressor`](make_regressor.md) /
  [`make_transformer`](make_transformer.md) — the only way to obtain any of
  the above. Each takes a `ModelSpec` and returns a `StatusOr<std::unique_ptr<...>>`.

## Why Three Interfaces Instead Of One

A classifier's natural output (a class label, or a score per class) is not a
regressor's (a continuous value), and neither is a transformer's (a mapped
feature matrix, with no notion of "correct answer" to assess against). Rather
than one interface with methods that only sometimes make sense,
`ClassifierBase` / `RegressorBase` share `EstimatorBase`'s
`learn`/`infer`/`assess` shape (a label is being predicted either way) and
`TransformerBase` is its own shape entirely (nothing here is being predicted
— features are being re-expressed).

## Ownership And Lifetime

`make_classifier` / `make_regressor` / `make_transformer` each return a
`StatusOr<std::unique_ptr<...>>` — you own the estimator through that
`unique_ptr`, and every concrete family (`LinearClassifier`,
`TreeClassifier`, …) is a distinct class behind the shared interface, so
polymorphic access through a base-class pointer is the only way to hold one
generically. There is no value-type `Model` object in `classic` the way
`lymph::models::Model` wraps one — `classic` hands you the polymorphic pointer
directly.

## Reading Order

1. [`EstimatorBase`](EstimatorBase.md) — read this first regardless of which
   concrete family you are using; every method page here applies to every
   classifier and regressor.
2. [`ClassifierBase`](ClassifierBase.md) / [`RegressorBase`](RegressorBase.md) —
   short pages, mostly pointing back at `EstimatorBase`.
3. [`make_classifier`](make_classifier.md) / [`make_regressor`](make_regressor.md) /
   [`make_transformer`](make_transformer.md) — how you actually get one.
4. [`TransformerBase`](TransformerBase.md) — a different shape, read
   separately when you need a transform stage rather than a final model.

## Related Pages

- [`families`](../families.md) — `ModelSpec`, the input every `make_*`
  function takes.
- [`core`](../core.md) — `TrainOptions` / `InferOptions` / `AssessOptions`.
- [`data`](../data.md) — `FeatureBatch` / `LabeledBatch`.
