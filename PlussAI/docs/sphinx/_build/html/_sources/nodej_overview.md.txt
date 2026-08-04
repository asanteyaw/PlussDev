# NodeJ Overview

**NodeJ** is the explicit classical ML core in **PlussAI**.

Primary public modules:

- types / status / options
- dataset / data
- split
- prep transformers
- model families
- builders
- compose::Flow
- metrics / tune
- online
- io

The full method-by-method surface is documented in [NodeJ Full API Reference](nodej_full_api.md).

## Cross-Validation Additions

### API Signature

- `stratified_crossval_plan(labels: Series, options: SplitOptions = {}) -> StatusOr<SplitPlan>`
- `cross_assess(spec: ModelSpec, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}) -> StatusOr<CrossAssessReport>`

### Notes

`CVPlanKind::StratifiedCrossVal` repeats class-preserving cross-validation plans with shuffled inner stratified partitions and a fresh deterministic seed per repetition. It is classification-only and returns `folds * repeats` fold reports when used through `cross_assess`.

### Inputs

- classification labels for split planning
- `SplitOptions` with `plan = StratifiedCrossVal`

### Output

- a `SplitPlan` or `CrossAssessReport` with repeated stratified folds

### Options

- `SplitOptions::folds`
- `SplitOptions::repeats`
- `SplitOptions::shuffle`
- `SplitOptions::random`

### Examples

```cpp
namespace nje = pluss::ai::nodej;

nje::SplitOptions split{};
split.plan = nje::CVPlanKind::StratifiedCrossVal;
split.folds = 5;
split.repeats = 3;
split.random.seed = 31;
split.random.deterministic = true;

auto plan_or = nje::stratified_crossval_plan(labels, split);
```

### Expected output

- `plan_or.value().folds.size()` is `15` when the labels contain enough samples per class for five folds.

## Balance Core

### API Signature

- `class_mix(labels: Series) -> StatusOr<ClassMix>`
- `resolve_weight_plan(labels: Series, batch_sample_weights: Series, options: TrainOptions) -> StatusOr<WeightPlan>`
- `resolve_training_weights(labels: Series, batch_sample_weights: Series, options: TrainOptions) -> StatusOr<Series>`

### Notes

NodeJ now centralizes class-frequency analysis and training-weight resolution. `class_mix` reports sorted labels, class counts, class proportions, inverse-frequency weights, and imbalance ratio. `resolve_weight_plan` combines batch sample weights, training sample weights, and `ClassWeightMode` into a normalized per-sample weight vector with mean weight one.

This is the shared foundation for wider estimator weight adoption and later data-level balancing modes.

### Inputs

- label vector aligned to observations
- optional batch sample weights
- `TrainOptions` with `sample_weights`, `class_weight`, and `class_weights`

### Output

- `ClassMix` for label distribution diagnostics
- `WeightPlan` for diagnostics plus resolved weights
- `Series` when only the resolved training weights are needed

### Options

- `ClassWeightMode::None`
- `ClassWeightMode::Balanced`
- `ClassWeightMode::Custom`

### Examples

```cpp
namespace nje = pluss::ai::nodej;

nje::TrainOptions train{};
train.class_weight = nje::ClassWeightMode::Balanced;

auto mix = nje::class_mix(labels);
auto weights = nje::resolve_training_weights(labels, batch.sample_weights, train);
```

### Expected output

- `mix.value().counts` contains one count per sorted class label.
- `weights.value()` contains one strictly positive weight per sample, normalized to mean one.

## Classification Metric Summary

### API Signature

- `confusion_table(truth_labels: Series, predicted_labels: Series, options: ConfusionOptions = {}) -> StatusOr<ConfusionTable>`
- `classification_summary(truth_labels: Series, predicted_labels: Series, options: ClassReportOptions = {}) -> StatusOr<ClassReport>`

### Notes

NodeJ exposes hard-label classification diagnostics through Pluss-native metric names. `confusion_table` returns the label order and count table, with optional truth-row, prediction-column, or global normalization. `classification_summary` derives per-class support, precision, recall, F1, accuracy, and micro/macro/weighted averages from the same label stream.

### Inputs

- truth labels aligned to observations
- predicted labels aligned to the same observations
- optional label order and normalization or zero-division handling

### Output

- `confusion_table` returns `StatusOr<ConfusionTable>`.
- `classification_summary` returns `StatusOr<ClassReport>`.

### Options

- `ConfusionOptions::normalize`
- `ConfusionOptions::label_order`
- `ClassReportOptions::label_order`
- `ClassReportOptions::zero_division`

### Examples

```cpp
namespace nje = pluss::ai::nodej;

auto table = nje::confusion_table(truth, pred, {0.0, 1.0}, nje::ConfusionNormalize::ByTruth);
auto summary = nje::classification_summary(truth, pred, {0.0, 1.0});
```

### Expected output

- `table.value().counts` is a 2 by 2 table when both label streams contain binary class labels.
- `summary.value().macro_average` contains precision, recall, and F1 aggregates.

## Estimator Family Additions

### API Signature

- `ModelSpec(task: Classification|Regression, family: Kernel, options: KernelFamilyOptions) -> ModelSpec`
- `ModelSpec(task: Transform, family: Kernel, options: KernelFamilyOptions{.form = OneClass}) -> ModelSpec`
- `ModelSpec(task: Classification|Regression, family: Bayes, options: BayesFamilyOptions) -> ModelSpec`

### Notes

Kernel family options now cover C-style SVM, Nu-style SVM, and one-class kernel transform workflows. One-class flows use the transform lifecycle and can be saved/restored with fitted transform state.

Bayes family options now cover Gaussian, Bernoulli, Multinomial, and Complement likelihoods for classification. Multinomial and Complement modes treat features as count-like inputs and reject negative counts during learn/infer. Multinomial Bayes regression also rejects negative labels. Complement Bayes is classifier-only.

### Inputs

- `KernelFamilyOptions` with `form = C`, `Nu`, or `OneClass`
- `BayesFamilyOptions` with `likelihood = Gaussian`, `Bernoulli`, `Multinomial`, or `Complement`

### Output

- trained classifiers/regressors for supervised specs
- mapped inlier/outlier labels for one-class transform specs
- explicit `InvalidArgument` status for invalid multinomial/complement count inputs
- explicit `NotImplemented` status for complement Bayes regression

### Options

- `KernelFamilyOptions::nu` controls Nu and one-class behavior.
- `BayesFamilyOptions::smoothing` applies to all Bayes likelihoods.

### Examples

```cpp
namespace nje = pluss::ai::nodej;

nje::ModelSpec one_class{
  .task = nje::TaskKind::Transform,
  .family = nje::ModelFamily::Kernel,
  .options = nje::KernelFamilyOptions{
      .kernel = nje::KernelKind::Rbf,
      .form = nje::KernelForm::OneClass,
      .gamma = 0.8,
      .nu = 0.5,
  },
};
```

```cpp
nje::ModelSpec multinomial{
  .task = nje::TaskKind::Classification,
  .family = nje::ModelFamily::Bayes,
  .options = nje::BayesFamilyOptions{
      .likelihood = nje::BayesLikelihood::Multinomial,
      .smoothing = 1e-6,
  },
};
```

```cpp
nje::ModelSpec complement{
  .task = nje::TaskKind::Classification,
  .family = nje::ModelFamily::Bayes,
  .options = nje::BayesFamilyOptions{
      .likelihood = nje::BayesLikelihood::Complement,
      .smoothing = 1e-6,
  },
};
```

### Expected output

- one-class kernel specs produce transform flows.
- multinomial Bayes specs accept non-negative counts and reject negative counts.
- complement Bayes specs accept non-negative counts for classification and reject regression specs.
