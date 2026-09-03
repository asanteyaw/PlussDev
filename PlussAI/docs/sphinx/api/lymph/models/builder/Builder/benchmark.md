<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 07d78350740e -->

# `lymph::models::builder::Builder::benchmark`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`benchmark` cross-validates every model in the workspace, ranks them by an
objective metric, and remembers the outcome so [`best`](best.md) and
[`worst`](worst.md) can hand back the winners.

## Signature

```cpp
lymph::selection::CompareResult lymph::models::builder::Builder::benchmark(
    const pluss::table::DataFrame& x,
    const pluss::Series& y,
    lymph::options::CrossVal cv = {},
    lymph::options::Train train_options = {},
    lymph::options::Assess assess_options = {},
    lymph::MetricKind objective_metric = lymph::MetricKind::Accuracy,
    lymph::TuneObjectivePolicy objective_policy = lymph::TuneObjectivePolicy::Auto,
    lymph::selection::FitOptions fit_options = {}) const;

lymph::selection::CompareResult lymph::models::builder::Builder::benchmark(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    lymph::options::CrossVal cv = {}, /* ... same trailing options ... */) const;

lymph::selection::CompareResult lymph::models::builder::Builder::benchmark(
    const lymph::LabeledBatch& batch,
    lymph::options::CrossVal cv = {}, /* ... same trailing options ... */) const;
```

## What It Does

`benchmark` runs `lymph::selection::compare(...)` over the whole registry: every
held model is **cloned**, each clone is cross-validated on the data you supply,
and the fold scores are aggregated into an objective value per candidate. The
result is a `CompareResult` whose `report.board()` is a `DataFrame` — one row per
candidate with its name, objective, rank, and each requested metric's mean.

Two properties matter:

- **Non-destructive.** The comparison runs on clones, so the `Builder` still
  holds every model it started with, untrained, after `benchmark` returns. Check
  with [`get`](get.md).
- **Remembered.** The last `CompareResult` is stored on the `Builder`.
  [`benchmarked()`](benchmarked.md) then returns `true`, and [`best()`](best.md) /
  [`worst()`](worst.md) return the top and bottom fitted candidates without you
  passing anything back in.

The three overloads take data the same three ways `Model::learn` does: features
plus a label `Series`, one `frame` plus a `DFBatch` naming the label, or a
`LabeledBatch` (the path for GPU-resident data, since a `LabeledBatch` is raw
`Series` with no DataFrame CPU detour).

## Parameters

`const pluss::table::DataFrame& x, const pluss::Series& y` (or `frame` + `data_options`, or `batch`)
: The data every candidate is cross-validated on.

`lymph::options::CrossVal cv` — default `{}`
: The cross-validation policy. `cv.folds` sets the fold count (Example 1 uses
  `3`); the rest of the fields — shuffling, stratification, repeats — follow the
  `CrossValOptions` defaults.

`lymph::options::Train train_options` — default `{}`
: Training options applied to every candidate for the duration of the benchmark.
  A per-candidate override is possible through `selection::Candidate` if one
  model needs different settings.

`lymph::options::Assess assess_options` — default `{}`
: Which metrics to compute for each fold. Every metric named here appears as a
  column on the board.

`lymph::MetricKind objective_metric` — default `MetricKind::Accuracy`
: The single metric the ranking is based on (Example 3 ranks by `F1`). It must be
  among the metrics that get computed.

`lymph::TuneObjectivePolicy objective_policy` — default `Auto`
: Whether a higher or lower objective is better. `Auto` picks the right direction
  for the metric (higher for accuracy, lower for error metrics).

`lymph::selection::FitOptions fit_options` — default `{}`
: Lower-level fitting controls passed straight through to `selection::compare`.

## Returns

A `lymph::selection::CompareResult`. Read `result.report.best_name` for the
winning candidate's name and `result.report.board()` for the full table.
`result.best()` / `result.worst()` return the fitted top and bottom models — the
same models [`best()`](best.md) / [`worst()`](worst.md) return from the `Builder`.

## Errors

- Too few rows for the requested fold count fails inside cross-validation.
- An `objective_metric` that is not among the computed metrics is rejected.
- An empty registry has nothing to compare.

## Practical Examples

### Example 1

Benchmark three classifiers with 3-fold cross-validation and read the board.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Twenty rows with a diagonal boundary and a few points in the overlap, so
  // the candidates score differently.
  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // benchmark cross-validates every held model through selection::compare and
  // remembers the outcome. The registry's own copies are untouched -- the
  // comparison runs on clones.
  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult result = builder.benchmark(x, y, cv);

  std::cout << "best candidate = " << result.report.best_name << "\n\n";
  std::cout << result.report.board();

  return 0;
}
```

Output

```text
best candidate = logreg

===============================================
     | name    objective     rank  accuracy    
-----------------------------------------------
0    | logreg  0.9027777778  1     0.9027777778
1    | tree    0.8611111111  3     0.8611111111
2    | knn     0.8888888889  2     0.8888888889
-----------------------------------------------
[3 cs x 4 feat]
```

### Example 2

The `(frame, DFBatch)` overload — one frame carrying features and the label
column, exactly as `Model::learn` accepts it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series defaulted = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);

  // One frame holds features and label; a DFBatch names them. benchmark has
  // the same (frame, DFBatch) overload learn and infer do.
  const DataFrame frame({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"defaulted", Thread(defaulted)},
  });
  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "defaulted";

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult result = builder.benchmark(frame, batch, cv);

  std::cout << "best candidate = " << result.report.best_name << "\n\n";
  std::cout << result.report.board();

  return 0;
}
```

Output

```text
best candidate = logreg

===============================================
     | name    objective     rank  accuracy    
-----------------------------------------------
0    | logreg  0.9027777778  1     0.9027777778
1    | tree    0.8611111111  2     0.8611111111
-----------------------------------------------
[2 cs x 4 feat]
```

### Example 3

Rank by `F1` instead of the default accuracy, on 6 folds. The objective column
now reports F1, and the ranking can shift as a result.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // The objective metric decides which candidate is "best". Rank by F1 rather
  // than the default accuracy, and score each fold on more folds.
  lymph::options::CrossVal cv{};
  cv.folds = 6;
  const lymph::selection::CompareResult result =
      builder.benchmark(x, y, cv, /*train_options=*/{}, /*assess_options=*/{},
                        lymph::MetricKind::F1);

  std::cout << "objective  = F1\n";
  std::cout << "best       = " << result.report.best_name << "\n\n";
  std::cout << result.report.board();

  return 0;
}
```

Output

```text
objective  = F1
best       = logreg

===============================================
     | name    objective     rank  f1          
-----------------------------------------------
0    | logreg  0.9111111111  1     0.9111111111
1    | tree    0.7777777778  2     0.7777777778
-----------------------------------------------
[2 cs x 4 feat]
```

## See Also

- [benchmarked](benchmarked.md) — whether an outcome is currently remembered.
- [best](best.md), [worst](worst.md) — the fitted winners from the last run.
- [get](get.md) — confirm the registry entries are untouched afterward.
- [Introduction](Introduction.md) — the non-destructive benchmark-memory model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="benchmarked.html">
    <span class="label">Next</span>
    <span class="title">benchmarked</span>
  </a>
</div>
<!-- apiref:nav:end -->
