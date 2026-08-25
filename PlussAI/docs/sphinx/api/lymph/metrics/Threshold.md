# `lymph::metrics::Threshold`

## Summary

`Threshold` is the Lymph threshold-inspection surface for score-based
classification.

Use it when you want to:

- inspect a threshold curve
- choose a threshold by an explicit objective
- move beyond the default decision boundary

## Why Threshold Analysis Matters

Many classifiers produce scores or probabilities before they produce final labels.

The default decision boundary is convenient, but it is not automatically the right
operating point for the business problem.

Threshold analysis exists because the cost of errors is usually asymmetric:

- false positives may be expensive
- false negatives may be expensive
- recall may matter more than precision, or the reverse

`Threshold` is the object you use when you want that tradeoff to be explicit.

## Core Surface

The main operations are:

- inspect the threshold curve with `curve()`
- compute the best threshold under an objective with `best(...)`

That means the object supports both:

- descriptive inspection
- decision-oriented threshold choice

## Signature

```cpp
lymph::metrics::Threshold lymph::metrics::threshold(
    const pluss::Series& scores,
    const pluss::Series& truth);
double lymph::metrics::Threshold::best(lymph::MetricKind metric) const;
```

## Practical Example

This page uses the exact verifier program `lymph_metrics_docverify_demo.cpp`.

### Example 1

This example fits a model, obtains its score vector, constructs a `Threshold`
object, and prints both the best F1 cutoff and the first three thresholds from the
curve.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classic = pluss::ai::classic;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  classic::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = classic::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);
  const Series scores =
      logistic.learn(frame, data_options, train_options).infer_scores(frame, data_options);

  const lymph::metrics::Threshold threshold =
      lymph::metrics::threshold(scores, frame.features("target").as_series());
  std::cout << "== threshold ==\n";
  std::cout << "best_f1=" << threshold.best(lymph::MetricKind::F1) << "\n";
  std::cout << threshold.curve().thresholds.slice(0, 0, 3) << "\n";

  return 0;
}
```

Output

```text
== threshold ==
best_f1=4.36761
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[9.9535291, 8.0915577, 6.2295863]
```

This output shows two things clearly:

- `best(...)` returns one concrete cutoff under the chosen metric objective
- `curve()` exposes the broader sweep so you can inspect threshold behavior before
  committing to that cutoff

## Related Pages

- [CrossValidationReport](CrossValidationReport.md)
- [classification_summary](classification_summary.md)
- [confusion_table](confusion_table.md)
