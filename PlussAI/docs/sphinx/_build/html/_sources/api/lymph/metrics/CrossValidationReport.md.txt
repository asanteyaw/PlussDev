# `lymph::metrics::CrossValidationReport`

## Summary

`CrossValidationReport` is the structured output of validation execution in Lymph.

Use it when you want fold-level evaluation plus aggregate metric summaries in one
object.

## What The Report Contains

The report carries several layers of evidence:

- fold-level trial information underneath
- metric-wise mean values
- metric-wise standard deviations
- a board materialization surface for inspection or export

That combination matters because a useful validation surface should answer both:

- how good is the model on average?
- how stable is that goodness across folds?

Too many APIs only answer the first question.

## `board()`

`board()` materializes the report as a `DataFrame`.

The board is usually what users want to inspect first because it exposes one row
per requested metric with columns such as:

- metric name
- fold mean
- fold standard deviation

Use it when you want:

- terminal inspection
- a saved report artifact
- a board you can join into experiment tracking

## Signature

```cpp
lymph::metrics::CrossValidationReport lymph::ValidationPlan::run(...) const;
pluss::table::DataFrame lymph::metrics::CrossValidationReport::board() const;
```

## Practical Example

This page uses the exact verifier program `lymph_validate_docverify_demo.cpp`.

### Example 1

This example constructs a validation plan, runs it against one model, and prints
the resulting cross-validation report board.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classic = pluss::ai::classic;
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
      {"when", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  classic::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = classic::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);

  lymph::ValidationOptions cross_options{};
  cross_options.folds = 4;
  cross_options.repeats = 1;
  cross_options.shuffle = true;
  cross_options.random_state = 20260825ULL;
  cross_options.deterministic = true;
  cross_options.max_parallel = 1;

  const lymph::ValidationPlan cross_plan = lymph::validate::cross(cross_options);
  const lymph::metrics::CrossValidationReport cross_report =
      cross_plan.run(logistic, frame, data_options, train_options, assess_options);

  std::cout << "== cross_validation_report ==\n";
  std::cout << cross_report.board() << "\n";

  return 0;
}
```

Output

```text
== cross_validation_report ==
============================
     | metric    mean  stdev
----------------------------
0    | accuracy  1     0
1    | f1        1     0
----------------------------
[2 cs x 3 feat]
```

This is the normal report flow:

1. define the validation contract
2. run the plan
3. keep the structured report
4. materialize the board when you need a readable summary

## Related Pages

- [ValidationOptions](ValidationOptions.md)
- [validate](../split.md)
- [confusion_table](confusion_table.md)
- [classification_summary](classification_summary.md)
