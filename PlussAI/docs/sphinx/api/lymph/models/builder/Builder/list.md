# `lymph::models::Builder::list`

## Signature

```cpp
std::vector<std::string> lymph::models::Builder::list(std::int64_t n = -1) const;
```

## What It Does

`list` returns a preview-sized slice of the candidate names.

Use it when you want a short human-readable subset rather than the full name
vector or the full tabular report.

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

- [names](names.md)
- [report](report.md)
