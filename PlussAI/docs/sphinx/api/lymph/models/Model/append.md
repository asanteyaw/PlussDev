# `lymph::models::Model::append`

## Signature

```cpp
lymph::models::Model& lymph::models::Model::append(
    lymph::prep::Stage stage,
    lymph::prep::Route route);
```

## What It Does

`append` attaches one explicit stage-route pair to the model.

This is the direct path when you already have the stage object and the route
object in hand and you want to bind them deliberately rather than growing the
plan through `prepare()`.

The important semantic point is that `append` can still remain pending.
`stage_count()` does not have to rise immediately, because route resolution may
need the real input frame before the stage graph can materialize fully.

## Practical Example

This example shows the explicit stage-plus-route assembly style.

```{literalinclude} ../../../../../../examples/NodeJ/lymph/lymph_models_model_append_docverify_demo.cpp
:language: cpp
```

Output

```text
== append_before_learn ==
trained = false, stage_count = 0

== append_after_learn ==
trained = true, stage_count = 2
================
     | predicted
----------------
0    | 0        
1    | 0        
2    | 0        
3    | 1        
4    | 0        
5    | 1        
6    | 0        
7    | 1        
----------------
[8 cs x 1 feat]
```

## Related Pages

- [prepare](prepare.md)
- [learn](learn.md)
- [Notes](Notes.md)
