<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 54690ca46ea0 -->

# `lymph::models::builder::Builder::report`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`report` returns a `DataFrame` describing the workspace: one row per registered
model, with its task kind, whether it is trained, and how many preparation stages
it carries.

## Signature

```cpp
pluss::table::DataFrame lymph::models::builder::Builder::report(
    std::int64_t n = -1) const;
```

## What It Does

`report` is the rich, tabular counterpart of [`names`](names.md) and
[`list`](list.md). Where `names()` gives you strings and `list(n)` gives you a
short preview, `report()` gives you a frame you can `std::cout` (or `.show()`,
or filter) with four columns:

- `name` — the registered key
- `task_kind` — `classification`, `regression`, or `transform`
- `trained` — `true` / `false`
- `stage_count` — the number of fitted preparation stages (0 until a model is
  trained)

Rows are in insertion order. `report(n)` returns only the first `n` — the same
clamping rule as `list(n)`, with `-1` meaning "all". The `trained` column tracks
the live state of each entry, so calling `report()` before and after training an
entry in place shows the change (Example 3).

## Parameters

`std::int64_t n` — default `-1`
: How many rows to include, from the front. `-1` (or `>= size()`) includes all.

## Returns

A `pluss::table::DataFrame` with columns `name`, `task_kind`, `trained`,
`stage_count`; one row per included model. The frame carries the title
`Builder`.

## Errors

None.

## Practical Examples

### Example 1

A full readout of a three-model registry, none trained yet.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // report returns a DataFrame -- one row per held model, with its task kind,
  // whether it is trained, and how many preparation stages it carries. Stream
  // it like any other frame.
  std::cout << builder.report();

  return 0;
}
```

Output

```text
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
2    | knn     classification  false    0          
---------------------------------------------------
[3 cs x 4 feat]
```

### Example 2

`report(n)` is the tabular preview: only the first `n` rows.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
      {"forest", lymph::models::ensemble::random_forest_classifier()},
  });

  // report(n) is the tabular counterpart of list(n): only the first n rows.
  std::cout << builder.report(2);

  return 0;
}
```

Output

```text
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
---------------------------------------------------
[2 cs x 4 feat]
```

### Example 3

The `trained` column follows the live state. Train one entry in place with the
non-`const` [`get`](get.md), and `report()` reflects it.

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
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.4, 3.8, 4.2, 4.6}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  std::cout << "before training:\n" << builder.report() << "\n";

  // Train one entry in place; report's `trained` column tracks it.
  builder.get("logreg").learn_(x, y);

  std::cout << "after training logreg:\n" << builder.report();

  return 0;
}
```

Output

```text
before training:
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
---------------------------------------------------
[2 cs x 4 feat]

after training logreg:
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  true     0          
1    | tree    classification  false    0          
---------------------------------------------------
[2 cs x 4 feat]
```

## See Also

- [names](names.md), [list](list.md) — the lighter inspection surfaces.
- [benchmark](benchmark.md) — its board is a different frame: cross-validated
  scores, not registry status.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="names.html">
    <span class="label">Previous</span>
    <span class="title">names</span>
  </a>
  <a class="next-link" href="worst.html">
    <span class="label">Next</span>
    <span class="title">worst</span>
  </a>
</div>
<!-- apiref:nav:end -->
