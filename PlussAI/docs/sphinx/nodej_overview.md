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
