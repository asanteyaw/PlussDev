# `lymph::models::Builder`

## Summary

`lymph::models::Builder` is the named candidate-model workspace in the
`models::builder` subfamily.

Use it when you want several candidate `Model` objects to live inside one
coordinated object before spotchecking, benchmarking, or search.

The important contract is that `Builder` is not just a container. It remembers
names, exposes tabular reporting, protects against accidental duplicate
registration, and remembers the last benchmark result strongly enough that
`best()` and `worst()` remain meaningful afterward.

## Signature

```cpp
lymph::models::Builder lymph::models::build();
lymph::models::Builder lymph::models::build(
    std::initializer_list<std::pair<std::string, lymph::models::Model>> entries);

lymph::models::Builder& lymph::models::Builder::inject(
    std::string name,
    lymph::models::Model model);

lymph::models::Model lymph::models::Builder::eject(std::string_view name);

bool lymph::models::Builder::has(std::string_view name) const noexcept;

const lymph::models::Model& lymph::models::Builder::get(std::string_view name) const;

std::vector<std::string> lymph::models::Builder::names() const;
std::vector<std::string> lymph::models::Builder::list(std::int64_t n = -1) const;

pluss::table::DataFrame lymph::models::Builder::report(std::int64_t n = -1) const;

lymph::selection::CompareResult lymph::models::Builder::benchmark(
    const pluss::table::DataFrame& x,
    const pluss::Series& y,
    lymph::selection::CrossValOptions cv = {},
    lymph::TrainOptions train_options = {},
    lymph::AssessOptions assess_options = {},
    lymph::MetricKind objective_metric = lymph::MetricKind::Accuracy,
    lymph::TuneObjectivePolicy objective_policy = lymph::TuneObjectivePolicy::Auto,
    lymph::selection::FitOptions fit_options = {}) const;

bool lymph::models::Builder::benchmarked() const noexcept;

lymph::models::Model lymph::models::Builder::best() const;
lymph::models::Model lymph::models::Builder::worst() const;
```

## Core Idea

The object solves three distinct jobs:

1. candidate registration
2. candidate inspection
3. candidate comparison handoff

Registration is the "what lives here?" problem.

Inspection is the "what exactly did I register?" problem.

Comparison handoff is the "now evaluate these without building auxiliary
candidate lists myself" problem.

That is why the surface intentionally includes both lightweight and richer
inspection paths:

- `names()` and `list(n)` for cheap identity inspection
- `report()` for one-row-per-model tabular reporting

## Benchmark Memory Contract

`benchmark(...)` is intentionally non-destructive with respect to the stored
candidate registry.

That matters because experimental comparison and registry ownership are not the
same concern. The comparison produces fitted winners and losers, but the
underlying workspace still holds the original registered models. After a
benchmark:

- `builder.benchmarked()` becomes true
- `builder.best()` and `builder.worst()` become valid
- the stored registry remains available for inspection
- the stored candidate models are not silently overwritten by the fitted
  outputs

The second example below prints that behavior directly.

## Practical Examples

This page uses the exact verifier programs:

- `lymph_models_builder_docverify_demo.cpp`
- `lymph_models_builder_benchmark_docverify_demo.cpp`

### Example 1

This example constructs a real candidate-model workspace, confirms the basic
identity state, shows both cheap listing surfaces, and then prints the richer
report table.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  std::cout << "== builder ==\n";
  std::cout << "size=" << builder.size()
            << " empty=" << std::boolalpha << builder.empty()
            << " has_tree=" << builder.has("tree") << "\n";
  const std::vector<std::string> names = builder.names();
  const std::vector<std::string> preview = builder.list(2);
  std::cout << "names=";
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i > 0) std::cout << ",";
    std::cout << names[i];
  }
  std::cout << "\n";
  std::cout << "preview=";
  for (std::size_t i = 0; i < preview.size(); ++i) {
    if (i > 0) std::cout << ",";
    std::cout << preview[i];
  }
  std::cout << "\n";
  std::cout << builder.report() << "\n";

  return 0;
}
```

Output

```text
== builder ==
size=3 empty=false has_tree=true
names=logreg,tree,knn
preview=logreg,tree
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
2    | knn     classification  false    0          
---------------------------------------------------
[3 cs x 4 feat]
```

This is the normal pre-comparison checkpoint:

- `names()` confirms insertion-order identity
- `list(2)` gives a cheap preview when you do not want the whole table
- `report()` turns the registry into a real table for screen inspection or
  further downstream handling

### Example 2

This example runs a real benchmark over the registered candidates, prints the
ranking board, and then proves that the workspace now remembers the benchmark
result without silently mutating the stored registry entry itself.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"x1", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"x2", Thread(pluss::make_series({1, 1, 2, 2, 8, 8, 9, 9}, {8}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;

  const lymph::selection::CompareResult ranked = builder.benchmark(
      x,
      y,
      cv,
      lymph::TrainOptions{},
      lymph::AssessOptions{},
      lymph::MetricKind::Accuracy,
      lymph::TuneObjectivePolicy::Auto,
      lymph::selection::FitOptions{.fit_top = 2});

  std::cout << "== benchmark ==\n";
  std::cout << ranked.report.board() << "\n";
  std::cout << "best_name=" << ranked.report.best_name
            << " benchmarked=" << std::boolalpha << builder.benchmarked()
            << " best_trained=" << builder.best().trained()
            << " worst_trained=" << builder.worst().trained()
            << " stored_logreg_trained=" << builder.get("logreg").trained()
            << "\n";

  return 0;
}
```

Output

```text
== benchmark ==
========================================
     | name    objective  rank  accuracy
----------------------------------------
0    | logreg  1          1     1       
1    | tree    0.875      3     0.875   
2    | knn     1          1     1       
----------------------------------------
[3 cs x 4 feat]
best_name=logreg benchmarked=true best_trained=true worst_trained=true stored_logreg_trained=false
```

The last line carries the main semantic point:

- `benchmarked=true` means the workspace remembered the result
- `best_trained=true` and `worst_trained=true` mean the remembered winners and
  losers are fitted
- `stored_logreg_trained=false` means the original registered candidate inside
  the workspace was not silently rewritten into the fitted winner

That non-destructive contract is what makes `Builder` safe to reuse across
multiple comparison passes.

## Related Pages

- [models family](../../models.md)
- [builder Introduction](Introduction.md)
- [Model](../Model.md)
