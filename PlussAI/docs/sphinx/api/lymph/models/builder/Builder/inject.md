# `lymph::models::Builder::inject`

## Signature

```cpp
lymph::models::Builder& lymph::models::Builder::inject(
    std::string name,
    lymph::models::Model model);
```

## What It Does

`inject` inserts one named candidate into the workspace.

This is the method you use when the candidate set grows incrementally and an
initializer-list construction would be too rigid.

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

- [has](has.md)
- [report](report.md)
