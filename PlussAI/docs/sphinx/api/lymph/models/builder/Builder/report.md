# `lymph::models::Builder::report`

## Signature

```cpp
pluss::table::DataFrame lymph::models::Builder::report(std::int64_t n = -1) const;
```

## What It Does

`report` materializes the candidate registry into a real table.

This is the main inspection surface when you want to see one row per candidate
instead of one name at a time.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_models_builder_docverify_demo.cpp
:language: cpp
```

Output

```text
== builder ==
size = 3, empty = false, has_tree = true
names = logreg,tree,knn
preview = logreg,tree

===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
2    | knn     classification  false    0          
---------------------------------------------------
[3 cs x 4 feat]
```

## Related Pages

- [names](names.md)
- [benchmark](benchmark.md)
