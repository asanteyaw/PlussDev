# `lymph::models::Builder::worst`

## Signature

```cpp
lymph::models::Model lymph::models::Builder::worst() const;
```

## What It Does

`worst` returns the remembered fitted loser from the last benchmark run.

That is useful when model comparison is exploratory and you want to inspect not
just what won, but what failed under the same evidence policy.

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

- [benchmark](benchmark.md)
- [best](best.md)
