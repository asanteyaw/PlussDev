# `lymph::models::Builder::best`

## Signature

```cpp
lymph::models::Model lymph::models::Builder::best() const;
```

## What It Does

`best` returns the remembered fitted winner from the last benchmark run.

This method is useful precisely because the registry itself remains
non-destructive. The stored candidates are not overwritten, but the best fitted
artifact is still retrievable as a separate value.

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
- [worst](worst.md)
