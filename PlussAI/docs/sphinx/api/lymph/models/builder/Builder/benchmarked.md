# `lymph::models::Builder::benchmarked`

## Signature

```cpp
bool lymph::models::Builder::benchmarked() const noexcept;
```

## What It Does

`benchmarked` tells you whether the workspace currently carries a remembered
benchmark result.

That matters because `best()` and `worst()` only become meaningful after a real
comparison pass.

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
- [worst](worst.md)
