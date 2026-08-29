# `lymph::models::Builder::get`

## Signature

```cpp
const lymph::models::Model& lymph::models::Builder::get(std::string_view name) const;
```

## What It Does

`get` reads one stored candidate by name without removing it from the
workspace.

Use it when one downstream step needs the registered template itself, not the
remembered winner from `best()`.

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
- [eject](eject.md)
