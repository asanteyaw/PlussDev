# `pluss::ai::classic::KernelFamilyOptions`

`KernelFamilyOptions` configures a kernel-method estimator (an SVM-family
classifier/regressor, or a one-class detector): which kernel function
computes similarity between points, which SVM formulation (`form`) is used,
and that formulation's own parameters.

## Fields

### `kernel`

`KernelKind kernel = KernelKind::Rbf`

The kernel function: `Linear` (no transformation — a linear decision
boundary, cheapest), `Rbf` (radial basis function / Gaussian — the default
and usually the strongest general-purpose choice), `Polynomial`, `Sigmoid`.

### `form`

`KernelForm form = KernelForm::C`

The SVM formulation: `C` (the standard C-SVM — penalize margin violations by
`c`), `Nu` (ν-SVM — parameterize by an upper bound on the margin-error
fraction instead, via `nu`), `OneClass` (a one-class boundary around the
training data — for novelty/outlier detection rather than
classification/regression).

### `c`

`double c = 1.0`

Only consulted for `form == C`. The penalty for margin violations. `> 0`.
Larger values fit the training data more tightly (less regularization, more
overfitting risk); smaller values allow a wider margin at the cost of more
training misclassifications.

### `gamma`

`double gamma = 0.0`

The kernel coefficient for `Rbf` / `Polynomial` / `Sigmoid` (ignored for
`Linear`). `0.0` (the default) means "derive it from `1 / n_features`" rather
than a literal zero kernel coefficient. Larger explicit values make the
kernel more locally sensitive (tighter influence per point, more prone to
overfitting); smaller values smooth the decision boundary.

### `epsilon`

`double epsilon = 0.1`

Regression only (kernel SVR). The width of the "no penalty" band around the
predicted value — errors smaller than `epsilon` are not penalized at all.
`>= 0`. Larger values tolerate more slack before penalizing error, producing
a sparser, simpler fit.

### `nu`

`double nu = 0.5`

Only consulted for `form == Nu` or `form == OneClass`. An upper bound on the
fraction of margin errors (and a lower bound on the fraction of support
vectors). `0 < nu <= 1`.

## Guidance

Start with the `Rbf` kernel and the `C` form — it is the most broadly
effective default. Move to `form = OneClass` specifically for novelty/outlier
detection (compare against
[`DetectFamilyOptions`](DetectFamilyOptions.md)'s isolation-forest approach,
which does not need a kernel at all and scales better to large datasets).

## Practical Example

`c` trading margin width for training fit tightness on a classification
problem.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](double c) {
    cls::KernelFamilyOptions opts{};
    opts.kernel = cls::KernelKind::Rbf;
    opts.form = cls::KernelForm::C;
    opts.c = c;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Kernel, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "c = 0.01 train accuracy: " << fit_and_score(0.01) << "\n";
  std::cout << "c = 100  train accuracy: " << fit_and_score(100.0) << "\n";

  return 0;
}
```

Output

```text
c = 0.01 train accuracy: 1
c = 100  train accuracy: 1
```

## Changelog

- **0.1** — `kernel`, `form` (+ `c` / `nu`), `gamma`, `epsilon`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.
- [`DetectFamilyOptions`](DetectFamilyOptions.md) — an alternative,
  kernel-free approach to outlier/novelty detection.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
