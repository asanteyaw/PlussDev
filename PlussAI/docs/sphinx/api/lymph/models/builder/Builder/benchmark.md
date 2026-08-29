# `lymph::models::Builder::benchmark`

## Signature

```cpp
lymph::selection::CompareResult lymph::models::Builder::benchmark(
    const pluss::table::DataFrame& x,
    const pluss::Series& y,
    lymph::options::CrossVal cv = {},
    lymph::options::Train train_options = {},
    lymph::options::Assess assess_options = {},
    lymph::MetricKind objective_metric = lymph::MetricKind::Accuracy,
    lymph::TuneObjectivePolicy objective_policy = lymph::TuneObjectivePolicy::Auto,
    lymph::options::Fit fit_options = {}) const;
```

## What It Does

`benchmark` runs the registered candidates through one comparison plan and
returns a ranked result object.

This is where the builder stops being a passive registry and becomes a real
multi-model workspace.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_models_builder_benchmark_docverify_demo.cpp
:language: cpp
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

best_name = logreg, benchmarked = true, best_trained = true, worst_trained = true, stored_logreg_trained = false
```

## Related Pages

- [benchmarked](benchmarked.md)
- [best](best.md)
- [worst](worst.md)
