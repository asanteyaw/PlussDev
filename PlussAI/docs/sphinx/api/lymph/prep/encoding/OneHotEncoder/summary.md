# `lymph::prep::OneHotEncoder::summary`

## Signature

```cpp
lymph::prep::TransformerSummary lymph::prep::OneHotEncoder::summary() const;
```

## What It Does

`summary()` exposes the encoder’s remembered feature names and learned state.

Use it when you need to inspect what the encoder learned instead of only
consuming the mapped output.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_prep_one_hot_encoder_docverify_demo.cpp
:language: cpp
```

Output

```text
== one_hot_encoder ==
rows = 8, features = 5, summary_features = 1

=======================================================
     | region_0  region_1  region_2  region_3  region_4
-------------------------------------------------------
0    | 0         0         0         1         0
1    | 0         0         1         0         0
2    | 0         0         0         1         0
-------------------------------------------------------
[3 cs x 5 feat]
== one_hot_encoder_inference ==
rows = 2, features = 5

=======================================================
     | region_0  region_1  region_2  region_3  region_4
-------------------------------------------------------
0    | 0         0         0         1         0
1    | 0         0         1         0         0
-------------------------------------------------------
[2 cs x 5 feat]
```

## Related Pages

- [learned](learned.md)
- [Notes](Notes.md)
