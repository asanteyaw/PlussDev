# `lymph::models::Model::infer`

## Signature

```cpp
pluss::Series lymph::models::Model::infer(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options) const;
```

## What It Does

`infer` runs the trained artifact forward on new rows using the same attached
preparation contract the model learned with.

The key question this method answers is not "how do I get predictions?" in the
abstract. It is "how do I get predictions without re-implementing the feature
conditioning path by hand?"

## Practical Example

This example trains a model and then prints the inferred prediction frame.

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

- [learn](learn.md)
- [assess](assess.md)
- [Notes](Notes.md)
