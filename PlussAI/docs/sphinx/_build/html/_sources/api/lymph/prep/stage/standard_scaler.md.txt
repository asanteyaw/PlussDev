# `lymph::prep::stage::standard_scaler`

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::standard_scaler(
    lymph::prep::StandardScalerOptions options = {});
```

## What It Does

This factory builds an attachable standard-scaling stage.

Use it when the scaling rule should travel as part of a larger prep graph,
usually through `ModelPrep` or `Model::append(...)`, rather than being fitted
as a standalone `prep::StandardScaler` object first.

## Practical Example

```{literalinclude} ../../../../../../examples/NodeJ/lymph/lymph_prep_stage_docverify_demo.cpp
:language: cpp
```

Output

```text
== stage ==
stage_count = 2, trained = true, task_kind = 0

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
```

## Related Pages

- [one_hot_encoder](one_hot_encoder.md)
- [Stage](Stage.md)
- [StandardScaler](../scaling/StandardScaler.md)
