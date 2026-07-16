# Lymph Troubleshooting

This page lists common runtime failures and fast fixes.

## Workflow Construction Errors

### API Signature

- `Workflow{ .model = ..., .stages = {...} }`
- `Workflow::learn_(...) -> Workflow&`

### Notes

A `Workflow` must have `.model` set before `learn_`, `infer`, `assess`, or `save`.

### Inputs

- workflow object

### Output

- valid executable workflow

### Options

- optional `.stages`

### Examples

```cpp
lym::Workflow wf{};               // invalid for execution
wf.learn_(x_train, y_train);      // throws

lym::Workflow ok{
    .model = lym::linear_factory::linear_regression({}),
    .stages = {lym::workflow::stage::standard_scaler()},
};
ok.learn_(x_train, y_train);      // valid
```

### Expected output

- first call throws runtime error indicating missing model.
- second call trains normally.

## No Series Features Found

### API Signature

- `prep::<Transformer>::learn(frame: DataFrame, ...) -> Transformer&`
- `Workflow::learn_(x: DataFrame, y: Series|Thread, ...) -> Workflow&`

### Notes

Lymph preprocessing and model entrypoints expect Series-backed features.

### Inputs

- DataFrame with non-Series-only columns

### Output

- training/transform after feature selection or casting

### Options

- `Transformer::features({...})` to pin columns

### Examples

```cpp
auto imp = lym::prep::SimpleImputer({});
imp.features({"temp", "humidity"}).learn(train_df);
```

### Expected output

- transformer learns only selected numeric columns and proceeds without "no Series features" errors.

## Calling map() Before learn()

### API Signature

- `Transformer::map(frame: DataFrame, options: InferOptions = {}) -> DataFrame`
- `Transformer::learn(...) -> Transformer&`
- `Transformer::learn_map(...) -> DataFrame`

### Notes

All preprocessors require a learned state before `map`.

### Inputs

- fresh preprocessor instance

### Output

- transformed DataFrame after learn phase

### Options

- use `learn_map` when you want one call

### Examples

```cpp
auto scaler = lym::prep::StandardScaler({});
// scaler.map(train_df);  // throws

const DataFrame train_s = scaler.learn_map(train_df);
const DataFrame test_s = scaler.map(test_df);
```

### Expected output

- `learn_map` initializes state and returns transformed frame.
- subsequent `map` calls succeed.

## Stratified CrossVal On Regression

### API Signature

- `cross_validate(..., cv: CrossValidationOptions, ...) -> CrossValidationReport`

### Notes

`StratifiedCrossVal` is classification-only, including when `repeats > 1`.

### Inputs

- regression task + stratified CV request

### Output

- use `CrossVal` for regression

### Options

- `CrossValidationOptions.plan`

### Examples

```cpp
auto cv = lym::CrossValidationOptions{
    .plan = pluss::ai::nodej::CVPlanKind::CrossVal,
    .folds = 5,
};

auto report = lym::cross_validate(workflow, x_train, y_train, cv);
```

### Expected output

- CV runs successfully for regression workflow.

## Objective Metric Missing During Tuning

### API Signature

- `selection::tune(..., assess_options: AssessOptions = {}, objective_metric: MetricKind = ..., ...) -> TuneResult`

### Notes

If objective metric is not in assess metrics, tune injects it automatically. If you post-process raw reports manually, keep objective included.

### Inputs

- candidate workflows and objective metric

### Output

- ranked trials

### Options

- `objective_metric`
- `objective_policy`

### Examples

```cpp
auto tuned = lym::selection::tune(
    std::move(candidates),
    x_train,
    y_train,
    {},
    {},
    pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Rmse}},
    pluss::ai::nodej::MetricKind::Rmse,
    pluss::ai::nodej::TuneObjectivePolicy::Minimize);
```

### Expected output

- `tuned.report.trials` populated and `tuned.report.best_index >= 0`.

## Restore Parity Drift

### API Signature

- `Workflow::save(...) -> void`
- `Workflow::load(path: string) -> Workflow`
- `max_abs_diff(lhs: Series, rhs: Series) -> double`

### Notes

Prediction drift after restore usually comes from non-deterministic training setup.

### Inputs

- original and restored predictions

### Output

- numeric parity check

### Options

- set deterministic random options in train/split/CV

### Examples

```cpp
const auto train_opts = pluss::ai::nodej::TrainOptions{
    .random = {.seed = 101, .deterministic = true},
};

wf.learn_(x_train, y_train, train_opts);
wf.save("model.bin", x_train, y_train, train_opts);
auto loaded = lym::Workflow::load("model.bin");

double delta = lym::max_abs_diff(wf.infer(x_test), loaded.infer(x_test));
```

### Expected output

- `delta` should be near zero.

## compare_series Shape Mismatch

### API Signature

- `compare_series(lhs: Series, rhs: Series) -> SeriesCompareReport`

### Notes

`compare_series` requires same flattened element count.

### Inputs

- two prediction series

### Output

- abs-error diagnostics

### Options

- use matching inference subsets

### Examples

```cpp
const Series p1 = wf.infer(x_test_a);
const Series p2 = wf.infer(x_test_b);
// Ensure test_a and test_b have same number of rows before compare_series.
```

### Expected output

- with matching shapes, returns `abs_error`, `max_abs_diff`, `mae`.

## Unknown Category Failures

### API Signature

- `TrainOptions.unknown_categorical`
- `InferOptions.unknown_categorical`
- `prep::OneHotEncoder(options: OneHotEncoderOptions = {})`
- `prep::FrequencyEncoder(options: FrequencyEncoderOptions = {})`
- `prep::OrdinalEncoder(options: OrdinalEncoderOptions = {})`
- `prep::LabelEncoder(options: LabelEncoderOptions = {})`

### Notes

Unknown category behavior is configurable both at preprocessing and model infer/train options.

### Inputs

- categorical values unseen during training

### Output

- controlled fallback behavior instead of hard failure

### Options

- `UnknownCategoryPolicy::Error | MissingBucket | Ignore`
- `OneHotEncoderOptions.include_unknown_bucket`
- `OrdinalEncoderOptions.include_unknown_bucket`
- `LabelEncoderOptions.include_unknown_bucket`

### Examples

```cpp
auto enc = lym::prep::OneHotEncoder({
    .include_unknown_bucket = true,
});
enc.features({"ocean_proximity"});

const DataFrame x_train_e = enc.learn_map(x_train);
const DataFrame x_test_e = enc.map(x_test);
```

### Expected output

- unseen categories map into unknown bucket instead of throwing.

## Performance Checklist

- Prefer `Workflow` with staged preprocessing over repeated ad-hoc transforms.
- Reuse fitted preprocessors for test/inference frames.
- Use deterministic seeds for reproducible benchmark comparisons.
- Use `cross_validate` for robust performance estimates before holdout reporting.
- Use typed options directly instead of custom wrappers for full control.
