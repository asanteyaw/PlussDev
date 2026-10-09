# `pluss::ai::classic::families`

`families` owns the estimator taxonomy: [`ModelSpec`](families/ModelSpec.md) —
the `{task, family, options}` triple that *describes* an estimator without
building one — and the nine `*FamilyOptions` structs, one per estimator
family, that fill its `options` slot.

Nothing here trains or predicts. [`estimators`](estimators.md) is where a
`ModelSpec` becomes something you can actually call `learn` / `infer` on.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`ModelSpec`](families/ModelSpec.md) | task + family + options, the thing every `make_*` / `sweep` / `tune` call consumes |
| [`LinearFamilyOptions`](families/LinearFamilyOptions.md) | linear regression / logistic / hinge / robust regression |
| [`TreeFamilyOptions`](families/TreeFamilyOptions.md) | a single decision tree |
| [`EnsembleFamilyOptions`](families/EnsembleFamilyOptions.md) | bagging, random forest, gradient boosting, AdaBoost |
| [`NeighborFamilyOptions`](families/NeighborFamilyOptions.md) | k-nearest-neighbors |
| [`KernelFamilyOptions`](families/KernelFamilyOptions.md) | SVM-family kernel methods |
| [`BayesFamilyOptions`](families/BayesFamilyOptions.md) | naive Bayes |
| [`ClusterFamilyOptions`](families/ClusterFamilyOptions.md) | k-means clustering |
| [`DecomposeFamilyOptions`](families/DecomposeFamilyOptions.md) | PCA |
| [`DetectFamilyOptions`](families/DetectFamilyOptions.md) | isolation-forest outlier detection |

## How To Call It

Build a linear classifier from a model spec with a small regularization strength. The program prints whether the classifier was constructed.

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
  cls::LinearFamilyOptions options{};
  options.regularization_strength = 1e-3;
  cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, options};
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> classifier = cls::make_classifier(spec);
  std::cout << "built = " << (classifier.ok() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
built = true
```

`.family` and the alternative held in `.options` must agree — see
[`ModelSpec`](families/ModelSpec.md#errors) for what happens when they do
not.

## Related Pages

- [`estimators`](estimators.md) — `make_classifier` / `make_regressor` /
  `make_transformer`, `ClassifierBase` / `RegressorBase` / `TransformerBase`.
- [`tune`](tune.md) — ranking a list of `ModelSpec` candidates by
  cross-validated score.
- [`compose`](compose.md) — a `ModelSpec` as the final model inside a `Flow`
  pipeline.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

families/Introduction
families/ModelSpec
families/LinearFamilyOptions
families/TreeFamilyOptions
families/EnsembleFamilyOptions
families/NeighborFamilyOptions
families/KernelFamilyOptions
families/BayesFamilyOptions
families/ClusterFamilyOptions
families/DecomposeFamilyOptions
families/DetectFamilyOptions
```
<!-- apiref:toctree:end -->
