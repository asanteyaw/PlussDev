<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a6b280e93c2c -->

# `lymph::models::builder::Builder::worst`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`worst` returns the lowest-ranked candidate from the last
[`benchmark`](benchmark.md), fitted — the mirror of [`best`](best.md).

## Signature

```cpp
lymph::models::Model lymph::models::builder::Builder::worst() const;
```

## What It Does

`worst()` pulls the bottom-ranked candidate out of the remembered
`CompareResult` and returns it as a fitted `Model`. Everything true of
[`best()`](best.md) is true here: it is a fitted clone rather than the registry
entry, and it throws if no `benchmark` has run.

It exists because the losing model is often the interesting one. Comparing the
best and worst fitted models on the same borderline rows shows *where* the
ranking gap comes from — the two can genuinely disagree on the hard cases even
when they agree everywhere else (Example 2).

## Parameters

None.

## Returns

A fitted `lymph::models::Model` — the candidate that ranked last on the last
benchmark's objective.

## Errors

- Calling `worst()` before any `benchmark` throws (`"benchmark() must be run
  before best() or worst()"`).

## Practical Examples

### Example 1

Benchmark three models, then take the laggard.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

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
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult result = builder.benchmark(x, y, cv);

  // worst() is the mirror of best(): the lowest-ranked candidate from the same
  // comparison, also returned fitted.
  const lymph::models::Model laggard = builder.worst();
  std::cout << "best on the board  = " << result.report.best_name << "\n";
  std::cout << "worst() trained    = " << std::boolalpha << laggard.trained() << "\n";

  return 0;
}
```

Output

```text
best on the board  = logreg
worst() trained    = true
```

### Example 2

Best versus worst on borderline rows. Both are real fitted models; on a row in
the overlap zone they disagree.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

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
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult ignored = builder.benchmark(x, y, cv);
  (void)ignored;

  // Both winners are real fitted models; compare their predictions on the same
  // probe rows.
  const lymph::models::Model top = builder.best();
  const lymph::models::Model bottom = builder.worst();
  // Probe rows in the overlap zone, where the top and bottom models can
  // genuinely disagree.
  const pluss::Series probe_income = pluss::make_series({2.6, 3.0, 3.4, 2.4}, {4}, f64);
  const pluss::Series probe_balance = pluss::make_series({2.4, 2.0, 1.7, 2.6}, {4}, f64);
  const DataFrame probe({{"income", Slate(probe_income)}, {"balance", Slate(probe_balance)}});

  const DataFrame compare({
      {"income", Slate(probe_income)},
      {"balance", Slate(probe_balance)},
      {"best_predicts", Slate(top.infer(probe))},
      {"worst_predicts", Slate(bottom.infer(probe))},
  });
  std::cout << compare;

  return 0;
}
```

Output

```text
=====================================================
     | income  balance  best_predicts  worst_predicts
-----------------------------------------------------
0    | 2.6     2.4      0              0             
1    | 3       2        0              1             
2    | 3.4     1.7      1              1             
3    | 2.4     2.6      0              0             
-----------------------------------------------------
[4 cs x 4 feat]
```

### Example 3

`worst()` before a benchmark throws, exactly like `best()`. Fully explicit
spelling.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::builder::Builder builder = lymph::models::builder::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // Like best(), worst() needs a prior benchmark and throws without one.
  try {
    const lymph::models::Model laggard = builder.worst();
    std::cout << "unexpectedly returned a model\n";
  } catch (const std::exception& err) {
    std::cout << "worst() before benchmark() threw: " << err.what() << "\n";
  }

  return 0;
}
```

Output

```text
worst() before benchmark() threw: lymph::Builder: benchmark() must be run before best() or worst().
```

## See Also

- [best](best.md) — the mirror call and the shared caveats.
- [benchmark](benchmark.md) — produces the ranking.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="report.html">
    <span class="label">Previous</span>
    <span class="title">report</span>
  </a>
</div>
<!-- apiref:nav:end -->
