# `lymph::models::Model`

## Summary

`lymph::models::Model` is the main cheap-copy trainable handle in Lymph.

Factories return it, prep stages attach to it, validation plans score it, and
selection surfaces compare it.

## Signature

```cpp
lymph::models::Model lymph::models::linear::logistic_regression(
    lymph::LinearFamilyOptions options = {});

lymph::models::ModelPrep lymph::models::Model::prepare();
lymph::models::Model& lymph::models::Model::learn_(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::TrainOptions& train_options = {});
pluss::Series lymph::models::Model::infer(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options) const;
lymph::MetricReport lymph::models::Model::assess(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::AssessOptions& assess_options = {}) const;
```

## Why This Object Exists

`Model` is the object that prevents everyday modeling work from collapsing into
raw family options and one-off helper code.

You use it when one staged, trainable artifact is enough:

- tabular classification
- tabular regression
- transform-style models
- persisted classical-ML artifacts

Move upward to `workflow::Workflow` when the project needs stronger runtime
orchestration than the trainable artifact alone.

## Related Pages

- [builder Introduction](builder/Introduction.md)
- [Builder](builder/Builder.md)
