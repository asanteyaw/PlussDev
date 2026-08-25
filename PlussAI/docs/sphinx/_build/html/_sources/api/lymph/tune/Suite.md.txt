# `lymph::spotcheck::Suite`

## Summary

`Suite` is the main spotcheck comparison object in Lymph.

Use it when you want to compare named candidate models across training strategies
without writing the orchestration manually each time.

## What Spotchecking Means Here

Spotchecking is not the same thing as full hyperparameter search.

Its job is broader and earlier:

- take several candidate model families
- try them under one or more training strategies
- learn which broad directions are worth further investment

This is why `Suite` usually comes before `Search`, not after it.

## Why Suite Exists

Teams often want to answer questions like:

- does balanced weighting help this whole candidate set?
- does a reuse or balancing strategy change the ranking materially?
- which model family is strong enough to deserve tuning?

Without `Suite`, those questions usually turn into repetitive comparison code and
unstable naming.

`Suite` fixes that by crossing:

- named candidate models from a `Builder`
- named training strategies

into one ranked comparison board.

## Signature

```cpp
lymph::Suite lymph::spotcheck::suite(const lymph::Builder& builder);
lymph::Suite& lymph::Suite::strategy(
    std::string name,
    lymph::TrainOptions options = {});
lymph::spotcheck::Result lymph::Suite::run(...) const;
```

## Practical Example

This page uses the exact verifier program `lymph_tune_docverify_demo.cpp`.

### Example 1

This example builds a two-model shortlist, attaches two strategies, runs the
comparison, and prints the resulting ranked board.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
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

  lymph::Builder builder = lymph::builder::build({
      {"logreg", lymph::model::logistic_regression()},
      {"tree", lymph::model::decision_tree_classifier()},
  });

  lymph::Suite suite = lymph::spotcheck::suite(builder);
  suite.strategy("plain", lymph::TrainOptions{});
  suite.strategy("weighted", lymph::TrainOptions{
      .class_weight = lymph::ClassWeightMode::Balanced,
  });

  lymph::ValidationOptions cv{};
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 20260825ULL;
  cv.deterministic = true;

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::spotcheck::Result suite_result =
      suite.run(
          frame,
          data_options,
          cv,
          assess_options,
          lymph::MetricKind::F1,
          lymph::TuneObjectivePolicy::Auto,
          {.fit_top = 1});
  std::cout << "== suite_compare_report ==\n";
  std::cout << "best=" << suite_result.report.best_name << "\n";
  std::cout << suite_result.report.board() << "\n";

  return 0;
}
```

Output

```text
== suite_compare_report ==
best=plain__logreg
========================================================
     | name              objective  rank  accuracy  f1
--------------------------------------------------------
0    | plain__logreg     1          1     1         1
1    | plain__tree       1          1     1         1
2    | weighted__logreg  1          1     1         1
3    | weighted__tree    1          1     1         1
--------------------------------------------------------
[4 cs x 5 feat]
```

This is what `Suite` earns its keep for:

- strategy names become part of candidate identity
- ranking happens on the post-strategy candidate, not only the raw builder entry
- the result is already in the form most users want to inspect or export

## Related Pages

- [CompareReport](CompareReport.md)
- [Search](Search.md)
- [Builder](../model/Builder.md)
