# `lymph::models::Builder::eject`

## Signature

```cpp
lymph::models::Model lymph::models::Builder::eject(std::string_view name);
```

## What It Does

`eject` removes one named candidate from the workspace and returns the removed
model.

That combination matters because it lets the registry shrink without destroying
the artifact you removed.

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

- [inject](inject.md)
- [get](get.md)
