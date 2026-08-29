# `lymph::models::Builder::has`

## Signature

```cpp
bool lymph::models::Builder::has(std::string_view name) const noexcept;
```

## What It Does

`has` answers the cheap identity question: "is this named candidate present in
the workspace right now?"

That is the quick guard before `get`, `eject`, or any comparison pass that
expects a particular candidate name to exist.

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

- [get](get.md)
- [inject](inject.md)
