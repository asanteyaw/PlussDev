# `pluss::ai::classic::InferOptions`

`InferOptions` is the prediction-time counterpart to
[`TrainOptions`](TrainOptions.md) — every `infer` / `infer_scores` / `map`
signature takes one. It repeats the two data-shape guards from `TrainOptions`
(they must match at fit and predict time) and adds the one field that is only
meaningful at prediction time: the binary decision threshold.

## Fields

### `layout`

`ObservationLayout layout = ObservationLayout::Rows`

As [`TrainOptions::layout`](TrainOptions.md#layout) — must match the layout
the estimator was trained with.

### `missing`

`MissingPolicy missing = MissingPolicy::Reject`

As [`TrainOptions::missing`](TrainOptions.md#missing). Independent of the
training-time setting — you can train with `ImputeMean` and predict with
`Reject`, though matching the two is usually what you want so training and
serving see the same missing-value handling.

### `unknown_categorical`

`UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error`

As [`TrainOptions::unknown_categorical`](TrainOptions.md#unknown_categorical).
This is the field that matters most at inference time in practice — training
data rarely has a category the training set itself has never seen, but
production data reliably does eventually.

### `positive_threshold`

`double positive_threshold = 0.5`

Binary classification only. The probability cutoff `infer` (the hard-label
form) uses to turn a positive-class score into `0` or `1`. `infer_scores`
returns a raw, unbounded decision score — not a `[0, 1]` probability, and not
directly comparable to `positive_threshold` by hand (that raw score can print
as anything from a large positive or negative number down to a value that
rounds to zero at the exact boundary; do not assume a 1:1 numeric scale
between the two). `infer` (the hard-label form) is what actually applies
`positive_threshold`, mapping the raw score into probability space
internally before comparing. `infer_scores` ignores `positive_threshold`
entirely — it always returns the same raw score regardless of what it is set
to. Moving `positive_threshold` off `0.5` trades precision for recall (raise
it to demand more confidence before predicting positive; lower it to catch
more positives at the cost of more false ones) without retraining. Feed it the
output of [`best_threshold`](../metrics/threshold_curve.md) rather than
guessing.

## Practical Example

The same fitted classifier, three different `positive_threshold` values — the
hard-label prediction shifts as the threshold crosses the model's own
(near-zero, unbounded) raw score, without touching the model or the raw score
itself.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge
  linear.regularization_strength = 1.0;  // soften the fit so the boundary score isn't saturated
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  // A probe row right at the model's decision boundary (halfway between the
  // classes, at x = 2.75).
  const cls::Series probe = pluss::make_series({2.75}, {1}, f64).reshape({1, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = probe;

  const double score = made.value()->infer_scores(probe_batch).value().item<double>();
  std::cout << "raw score = " << score << "\n";

  for (double threshold : {0.2, 0.5, 0.8}) {
    cls::InferOptions opts{};
    opts.positive_threshold = threshold;
    const double predicted = made.value()->infer(probe_batch, opts).value().item<double>();
    std::cout << "positive_threshold = " << threshold << " -> predicted class = " << predicted << "\n";
  }

  return 0;
}
```

Output

```text
raw score = 2.22045e-16
positive_threshold = 0.2 -> predicted class = 1
positive_threshold = 0.5 -> predicted class = 1
positive_threshold = 0.8 -> predicted class = 0
```

## Changelog

- **0.1** — the shared shape guards plus `positive_threshold`.

## See Also

- [`TrainOptions`](TrainOptions.md) — the fit-time counterpart; `layout` /
  `missing` / `unknown_categorical` should usually match between the two.
- [`AssessOptions`](AssessOptions.md) — carries its own
  `positive_threshold` for metrics that need hard labels.
- [`metrics::threshold_curve`](../metrics/threshold_curve.md) — sweeps every
  achievable threshold and reports the best one for a chosen objective.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
