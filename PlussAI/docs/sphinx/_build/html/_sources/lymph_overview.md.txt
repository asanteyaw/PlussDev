# Lymph Overview

**Lymph** is the convenience layer over NodeJ, with high-level wrappers for:

- workflow-oriented training/inference
- DataFrame-oriented preprocessing and evaluation
- quick one-call builder/trainer helpers
- tuning and cross-validation helpers
- online wrappers

The full method-by-method surface is documented in [Lymph Full API Reference](lymph_full_api.md).

## Cross-Validation Additions

### API Signature

- `cross_validate(model: WorkflowModel|Workflow|ModelSpec, x: DataFrame, y: Series|Thread, cv: CrossValidationOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}) -> CrossValidationReport`

### Notes

`CrossValidationOptions` now forwards `StratifiedCrossVal` and `repeats` into the NodeJ splitter layer. Use it for classification workflows when a single stratified cross-validation pass is too sensitive to one shuffled partition.

### Inputs

- classification workflow or model spec
- feature frame and classification labels
- `CrossValidationOptions` with `plan = StratifiedCrossVal`

### Output

- `CrossValidationReport` with repeated fold metrics and aggregate metric reports

### Options

- `CrossValidationOptions::folds`
- `CrossValidationOptions::repeats`
- `CrossValidationOptions::shuffle`
- `CrossValidationOptions::random_state`
- `CrossValidationOptions::deterministic`

### Examples

```cpp
namespace lym = pluss::ai::lymph;
namespace nje = pluss::ai::nodej;

auto report = lym::cross_validate(
    classifier, x_class, y_class,
    lym::CrossValidationOptions{
        .plan = nje::CVPlanKind::StratifiedCrossVal,
        .folds = 5,
        .repeats = 3,
        .random_state = 31,
        .deterministic = true,
    });
```

### Expected output

- `report.report.folds.size()` is `15` when the labels contain enough samples per class for five folds.

## Classification Metric Summary

### API Signature

- `confusion_table(truth_labels: Series, predicted_labels: Series, options: ConfusionOptions = {}) -> ConfusionTable`
- `classification_summary(truth_labels: Series, predicted_labels: Series, options: ClassReportOptions = {}) -> ClassReport`
- `Workflow::confusion_table(batch: LabeledBatch, infer_options: InferOptions = {}, options: ConfusionOptions = {}) -> ConfusionTable`
- `Workflow::classification_summary(batch: LabeledBatch, infer_options: InferOptions = {}, options: ClassReportOptions = {}) -> ClassReport`

### Notes

Lymph uses the same public names as NodeJ for hard-label classification diagnostics. The free functions evaluate existing truth/prediction vectors, while workflow helpers infer predictions from the current trained model and then return the same summary objects.

### Inputs

- truth and predicted labels, or a trained workflow plus labeled data
- optional label order and normalization or zero-division handling

### Output

- `confusion_table` returns a `ConfusionTable`.
- `classification_summary` returns a `ClassReport`.

### Options

- `ConfusionOptions::normalize`
- `ConfusionOptions::label_order`
- `ClassReportOptions::label_order`
- `ClassReportOptions::zero_division`

### Examples

```cpp
namespace lym = pluss::ai::lymph;

auto table = lym::confusion_table(model, batch);
auto summary = lym::classification_summary(model, batch);
```

```cpp
auto table = workflow.confusion_table(batch);
auto summary = workflow.classification_summary(batch);
```

### Expected output

- `table.counts` contains a square class-by-class count table.
- `summary.weighted_average` contains precision, recall, and F1 aggregates weighted by class support.

## Estimator Factory Additions

### API Signature

- `kernel_factory::nu_svm_regressor(options: KernelFamilyOptions = {}) -> WorkflowModel`
- `kernel_factory::nu_svm_classifier(options: KernelFamilyOptions = {}) -> WorkflowModel`
- `kernel_factory::one_class_svm(options: KernelFamilyOptions = {}) -> WorkflowModel`
- `neighbor_factory::radius_neighbors_regressor(options: NeighborFamilyOptions = {}) -> WorkflowModel`
- `neighbor_factory::radius_neighbors_classifier(options: NeighborFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::bernoulli_nb_regressor(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::bernoulli_nb_classifier(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::multinomial_nb_regressor(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::multinomial_nb_classifier(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::complement_nb_classifier(options: BayesFamilyOptions = {}) -> WorkflowModel`

### Notes

The factories mirror the expanded NodeJ family options while keeping Lymph's one-call workflow style. Radius-neighbor factories set the neighbor query mode to Radius. Nu SVM factories set the kernel form to Nu. `one_class_svm` creates a transform-style workflow model. Bernoulli, Multinomial, and Complement NB factories set the Bayes likelihood explicitly.

Multinomial and Complement NB expect non-negative count-like features. Negative counts fail with an explicit status instead of being clamped. Complement NB is classifier-only.

### Inputs

- `KernelFamilyOptions`
- `NeighborFamilyOptions`
- `BayesFamilyOptions`
- feature or labeled batches for the selected workflow operation

### Output

- `WorkflowModel` objects ready for `learn`, `infer`, `learn_map`, or `map`

### Options

- kernel factories preserve supplied `kernel`, `gamma`, `c`, `epsilon`, and `nu` values while setting the intended form
- radius-neighbor factories preserve supplied `radius`, `radius_fallback_k`, `distance_weighted`, and `standardize`
- Bayes factories preserve supplied `smoothing` while setting the intended likelihood

### Examples

```cpp
namespace lym = pluss::ai::lymph;
namespace nje = pluss::ai::nodej;

auto one_class = lym::kernel_factory::one_class_svm(
    nje::KernelFamilyOptions{
        .kernel = nje::KernelKind::Rbf,
        .gamma = 0.8,
        .nu = 0.5,
    });

auto labels = one_class.learn_map(features);
```

```cpp
auto radius = lym::neighbor_factory::radius_neighbors_classifier(
    nje::NeighborFamilyOptions{
        .radius = 0.75,
        .radius_fallback_k = 3,
        .distance_weighted = true,
    });

auto trained = radius.learn(labeled_batch);
auto predictions = trained.infer(features);
```

```cpp
auto multinomial = lym::bayes_factory::multinomial_nb_classifier(
    nje::BayesFamilyOptions{.smoothing = 1e-6});

auto trained = multinomial.learn(count_batch);
auto predictions = trained.infer(count_features);
```

```cpp
auto complement = lym::bayes_factory::complement_nb_classifier(
    nje::BayesFamilyOptions{.smoothing = 1e-6});

auto trained = complement.learn(count_batch);
auto predictions = trained.infer(count_features);
```

### Expected output

- `one_class.learn_map` returns one inlier/outlier label per input sample.
- `radius.learn(...).infer(...)` uses all neighbors in the configured radius and falls back to nearest neighbors for empty neighborhoods.
- `multinomial.learn(...).infer(...)` returns predictions for valid non-negative counts.
- `complement.learn(...).infer(...)` returns classifier predictions for valid non-negative counts.
