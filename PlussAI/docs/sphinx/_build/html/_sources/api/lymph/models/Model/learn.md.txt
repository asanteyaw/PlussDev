# `lymph::models::Model::learn`

## Signature

```cpp
lymph::models::Model& lymph::models::Model::learn_(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Train& train_options = {});

lymph::models::Model lymph::models::Model::learn(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Train& train_options = {}) const;
```

## What It Does

This page teaches both training forms because they are one conceptual method:

- `learn_` mutates the left-hand model in place
- `learn` keeps the template untouched and returns a trained copy

The choice is therefore not about different learning algorithms. It is about
ownership style.

Use `learn_` when the left-hand object is supposed to become the trained
artifact.

Use `learn` when the current object is still a template and you want the fitted
result to come back as a separate value.

## Practical Example

This example shows both paths in one program.

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

- [prepare](prepare.md)
- [infer](infer.md)
- [assess](assess.md)
- [Notes](Notes.md)
