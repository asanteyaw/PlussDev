<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f6a8c0bb2398 -->

# `lymph::models::builder::Builder::best`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`best` returns the top-ranked candidate from the last [`benchmark`](benchmark.md),
as a fitted `Model` ready to use.

## Signature

```cpp
lymph::models::Model lymph::models::builder::Builder::best() const;
```

## What It Does

After a `benchmark`, the `Builder` remembers a `CompareResult`. `best()` pulls the
highest-ranked candidate out of it and returns it **fitted** — trained during the
comparison, ready for [`infer`](../../Model/infer.md), [`assess`](../../Model/assess.md),
or [`save`](../../Model/save.md).

Two things to keep straight:

- `best()` returns a **fitted clone**, not the registry entry. The model
  registered under that name is still the untrained template you injected;
  [`get(name)`](get.md) still returns it untrained (Example 3). `best()` and
  `get()` answer different questions — "the trained winner" versus "the entry I
  registered".
- `best()` **throws** if no `benchmark` has run. Guard it with
  [`benchmarked()`](benchmarked.md) when a winner might be requested too early
  (Example 2).

The winning candidate's *name* is available without `best()` — it is
`result.report.best_name` on the value `benchmark` returned, and it heads the
board.

## Parameters

None.

## Returns

A fitted `lymph::models::Model` — the candidate that ranked first on the last
benchmark's objective.

## Errors

- Calling `best()` before any `benchmark` throws (`"benchmark() must be run
  before best() or worst()"`).

## Practical Examples

### Example 1

Benchmark, then take the winner and predict with it.

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
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult result = builder.benchmark(x, y, cv);

  // best() returns the winning candidate as a fitted Model, ready to infer.
  const lymph::models::Model winner = builder.best();
  std::cout << "best candidate  = " << result.report.best_name << "\n";
  std::cout << "winner trained  = " << std::boolalpha << winner.trained() << "\n\n";

  const pluss::Series probe_income = pluss::make_series({1.5, 3.0, 4.5}, {3}, f64);
  const pluss::Series probe_balance = pluss::make_series({0.6, 1.4, 2.4}, {3}, f64);
  const DataFrame probe({{"income", Thread(probe_income)}, {"balance", Thread(probe_balance)}});
  const DataFrame table({
      {"income", Thread(probe_income)},
      {"balance", Thread(probe_balance)},
      {"winner_predicts", Thread(winner.infer(probe))},
  });
  std::cout << table;

  return 0;
}
```

Output

```text
best candidate  = logreg
winner trained  = true

=======================================
     | income  balance  winner_predicts
---------------------------------------
0    | 1.5     0.6      0              
1    | 3       1.4      0              
2    | 4.5     2.4      1              
---------------------------------------
[3 cs x 3 feat]
```

### Example 2

`best()` before a benchmark throws.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // Asking for the best before running a benchmark is an error: there is no
  // remembered comparison to draw a winner from.
  try {
    const lymph::models::Model winner = builder.best();
    std::cout << "unexpectedly returned a winner\n";
  } catch (const std::exception& err) {
    std::cout << "best() before benchmark() threw: " << err.what() << "\n";
  }

  return 0;
}
```

Output

```text
best() before benchmark() threw: lymph::Builder: benchmark() must be run before best() or worst().
```

### Example 3

`best()` is a fitted clone; the registry entries stay untrained.

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

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult ignored = builder.benchmark(x, y, cv);
  (void)ignored;

  // best() hands back a freshly fitted clone. The registry's own entry under
  // that name is untouched -- still the untrained template you injected.
  const lymph::models::Model winner = builder.best();
  std::cout << "best()          trained = " << std::boolalpha << winner.trained() << "\n";
  std::cout << "get(\"logreg\")   trained = " << builder.get("logreg").trained() << "\n";
  std::cout << "get(\"tree\")     trained = " << builder.get("tree").trained() << "\n";

  return 0;
}
```

Output

```text
best()          trained = true
get("logreg")   trained = false
get("tree")     trained = false
```

## See Also

- [benchmark](benchmark.md) — produces the ranking `best` draws from.
- [worst](worst.md) — the mirror call.
- [get](get.md) — the registered entry, not the fitted winner.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="benchmarked.html">
    <span class="label">Previous</span>
    <span class="title">benchmarked</span>
  </a>
  <a class="next-link" href="eject.html">
    <span class="label">Next</span>
    <span class="title">eject</span>
  </a>
</div>
<!-- apiref:nav:end -->
