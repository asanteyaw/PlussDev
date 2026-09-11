# `pluss::ai::classic::BayesFamilyOptions`

`BayesFamilyOptions` configures a naive Bayes estimator: `likelihood` picks
which distribution each feature is assumed to follow given the class, and
`smoothing` guards against zero-probability estimates.

## Fields

### `likelihood`

`BayesLikelihood likelihood = BayesLikelihood::Gaussian`

The per-feature likelihood model: `Gaussian` (each feature normally
distributed within a class — the default, appropriate for continuous
features), `Bernoulli` (binary/indicator features), `Multinomial` (count
features — word counts, for instance), `Complement` (a Multinomial variant
that estimates from the complement of each class, generally more robust to
imbalanced classes).

### `use_gaussian`

`bool use_gaussian = true`

A compatibility shim, not an independent choice: when `likelihood` is left at
its default `Gaussian`, this flag's `false` value maps to a discrete
(non-Gaussian) mode instead. Prefer setting `likelihood` explicitly to one of
`Bernoulli` / `Multinomial` / `Complement` rather than relying on this flag.

### `smoothing`

`double smoothing = 1e-9`

Additive (Laplace/Lidstone-style) smoothing for `Bernoulli` / `Multinomial` /
`Complement`, and a variance floor for `Gaussian`. `>= 0`. Guards against a
feature value that never co-occurred with some class in training producing a
hard zero probability (which would make the whole posterior zero regardless
of other evidence). Raise it when your training data is small enough that
zero co-occurrence is common but not necessarily meaningful.

## Guidance

Match `likelihood` to your feature type first — this choice matters far more
than tuning `smoothing`. `smoothing`'s default is deliberately tiny (`1e-9`,
mostly a numerical-stability floor); raise it meaningfully (`0.1`-`1.0`) only
once you have observed zero-probability issues on genuinely sparse data.

## Practical Example

`likelihood` on count-shaped data: `Multinomial` fits the shape correctly,
`Gaussian` does not.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Word-count-style features: small non-negative integers.
  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 4.0, 1.0, 0.0, 5.0, 1.0, 4.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](cls::BayesLikelihood likelihood) {
    cls::BayesFamilyOptions opts{};
    opts.likelihood = likelihood;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Bayes, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "Gaussian train accuracy:     " << fit_and_score(cls::BayesLikelihood::Gaussian) << "\n";
  std::cout << "Multinomial train accuracy:  " << fit_and_score(cls::BayesLikelihood::Multinomial) << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_BAYESFAMILYOPTIONS_1
```

## Changelog

- **0.1** — `likelihood`, `use_gaussian` (compatibility shim), `smoothing`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
