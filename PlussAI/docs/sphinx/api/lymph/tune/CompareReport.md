# `lymph::selection::CompareReport`

## Summary

`CompareReport` is the ranked evidence board produced after comparing several
named candidates under one objective.

Use it when you need ranked model outcomes rather than just one winning score.

## What The Report Represents

`CompareReport` does not only tell you the winner. It preserves the ranking
landscape around that winner.

That matters because model selection should be inspectable, not mystical.

A useful comparison report keeps:

- the candidate names
- the objective values
- the ranks
- the requested metric summaries

So the report answers both:

- who won?
- how did everyone else place?

## `board()`

`board()` materializes the comparison result as a `DataFrame`.

This is one of the most practically useful surfaces because it gives you a table
with rows for each candidate and columns for:

- candidate name
- objective value
- rank
- requested metrics

That board is what you inspect, export, or attach to experiment notes.

## Signature

```cpp
pluss::table::DataFrame lymph::CompareReport::board() const;
```

## Practical Example

This page uses the exact verifier program `lymph_tune_docverify_demo.cpp`.

### Example 1

This example produces a `CompareReport` through a spotcheck suite and prints the
ranking board directly.

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
  std::cout << suite_result.report.board() << "\n";

  return 0;
}
```

Output

```text
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

That board is the point of the type. It keeps the selection process auditable
after the winner has been identified.

## Related Pages

- [Suite](Suite.md)
- [Search](Search.md)
- [TuneReport](TuneReport.md)
