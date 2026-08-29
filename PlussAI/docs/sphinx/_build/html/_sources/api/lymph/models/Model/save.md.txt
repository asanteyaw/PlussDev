# `lymph::models::Model::save`

## Signature

```cpp
void lymph::models::Model::save(const std::string& path) const;
```

## What It Does

`save` persists the trained model artifact so it can be restored later through
`load`.

The method matters when model building and model serving are no longer the same
process lifetime. It is therefore the persistence boundary for the trained
artifact itself, not for a broader deployment workflow.

## Practical Example

```{literalinclude} ../../../../../../examples/NodeJ/lymph/lymph_models_model_io_docverify_demo.cpp
:language: cpp
```

Output

```text
== save_load ==
original_trained = true, loaded_trained = true
original_stage_count = 2, loaded_stage_count = 2

===============
     | original
---------------
0    | 0       
1    | 0       
2    | 0       
3    | 0       
4    | 1       
5    | 1       
6    | 1       
7    | 1       
---------------
[8 cs x 1 feat]
===============
     | reloaded
---------------
0    | 0       
1    | 0       
2    | 0       
3    | 0       
4    | 1       
5    | 1       
6    | 1       
7    | 1       
---------------
[8 cs x 1 feat]
roundtrip_match = true
```

## Related Pages

- [load](load.md)
- [Notes](Notes.md)
