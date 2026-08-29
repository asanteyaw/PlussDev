# `lymph::prep::OneHotEncoder::map`

## Signature

```cpp
pluss::table::DataFrame lymph::prep::OneHotEncoder::map(
    const pluss::table::DataFrame& frame,
    const lymph::options::Infer& options = {}) const;

pluss::Series lymph::prep::OneHotEncoder::map(
    const pluss::table::Thread& feature,
    const lymph::options::Infer& options = {}) const;
```

## What It Does

`map(...)` applies the previously learned indicator layout to new data.

This is where `include_unknown_bucket` starts to matter in practice: new data
may contain category values that were not seen during learning.

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

- [learn](learn.md)
- [learn_map](learn_map.md)
- [Notes](Notes.md)
