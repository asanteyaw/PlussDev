# `lymph::prep::stage::one_hot_encoder`

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::one_hot_encoder(
    lymph::prep::OneHotEncoderOptions options = {});
```

## What It Does

This factory builds an attachable one-hot-encoding stage.

Use it when categorical expansion should become part of a model-bound or
branch-bound preprocessing graph instead of being learned as a standalone
encoder object first.

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

- [standard_scaler](standard_scaler.md)
- [Stage](Stage.md)
- [OneHotEncoder](../encoding/OneHotEncoder.md)
