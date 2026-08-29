# `lymph::models::Builder::names`

## Signature

```cpp
std::vector<std::string> lymph::models::Builder::names() const;
```

## What It Does

`names` returns the full candidate-name vector in registry order.

It is the cheap identity surface when you want all names but do not need the
heavier one-row-per-model report.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_models_builder_ops_docverify_demo.cpp
:language: cpp
```

Output

```text
== after_inject ==
size = 2, has_tree = true
names = logreg, tree

===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
---------------------------------------------------
[2 cs x 4 feat]
== after_eject ==
size = 1, has_tree = false, ejected_trained = false
preview = logreg
remaining_task_kind = 0
```

## Related Pages

- [list](list.md)
- [report](report.md)
