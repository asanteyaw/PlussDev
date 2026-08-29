# `lymph::models::Model::load`

## Signature

```cpp
lymph::models::Model lymph::models::Model::load(const std::string& path);
```

## What It Does

`load` restores a persisted model artifact from storage and returns a ready
`Model` value.

The important question here is not just "can it deserialize?" but "does the
loaded artifact still behave like the trained object I saved?" The roundtrip
example below checks that directly.

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

- [save](save.md)
- [Notes](Notes.md)
