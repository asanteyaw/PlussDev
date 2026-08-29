# `lymph::models::Model::prepare`

## Signature

```cpp
lymph::models::ModelPrep lymph::models::Model::prepare() noexcept;
```

## What It Does

`prepare` opens the model-bound preparation planner.

This is the fluent path for attaching scaling, encoding, routing, and related
feature work directly to the future model artifact. In normal use it is the
cleaner public surface than repeated `append(...)` calls, especially when the
project is still being shaped interactively.

## Practical Example

This example shows `prepare()` growing a numeric-plus-categorical plan before
any training happens.

```{literalinclude} ../../../../../../examples/NodeJ/lymph/lymph_models_model_docverify_demo.cpp
:language: cpp
```

Output

```text
== initial_model ==
trained = false, stage_count = 0, task_kind = 0

== after_prepare ==
trained = false, stage_count = 0

== learned_model ==
trained = true, stage_count = 2
================
     | predicted
----------------
0    | 0        
1    | 0        
2    | 0        
3    | 0        
4    | 1        
5    | 1        
6    | 1        
7    | 1        
----------------
[8 cs x 1 feat]
accuracy = 1, f1 = 1

== learned_copy ==
original_trained = false, original_stage_count = 0, copy_trained = true, copy_stage_count = 2
```

## Related Pages

- [append](append.md)
- [learn](learn.md)
- [Notes](Notes.md)
