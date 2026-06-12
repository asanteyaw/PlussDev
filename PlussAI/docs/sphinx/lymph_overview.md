# Lymph Overview

**Lymph** is the convenience layer over NodeJ, with high-level wrappers for:

- workflow-oriented training/inference
- DataFrame-oriented preprocessing and evaluation
- quick one-call builder/trainer helpers
- tuning and cross-validation helpers
- online wrappers

The full method-by-method surface is documented in [Lymph Full API Reference](lymph_full_api.md).

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
