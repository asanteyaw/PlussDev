# `lymph::models::Model::assess`

## Signature

```cpp
lymph::reports::Metric lymph::models::Model::assess(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Assess& assess_options = {}) const;
```

## What It Does

`assess` runs the trained model against a frame and returns a structured report
object rather than a loose metric scalar.

That report object is where Lymph keeps multi-metric work readable. The method
therefore matters most when:

- you want several metrics from one pass
- you want report fields instead of positional tuples
- you want the same preprocessing contract used for training and inference

## Practical Example

This example trains a model, infers, and then assesses with accuracy and F1.

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
- [infer](infer.md)
- [Notes](Notes.md)
