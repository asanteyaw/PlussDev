# `lymph::prep::OneHotEncoder::learn_map`

## Signature

```cpp
pluss::table::DataFrame lymph::prep::OneHotEncoder::learn_map(
    const pluss::table::DataFrame& frame,
    const lymph::options::Train& learn_options = {},
    const lymph::options::Infer& infer_options = {});

pluss::Series lymph::prep::OneHotEncoder::learn_map(
    const pluss::table::Thread& feature,
    const lymph::options::Train& learn_options = {},
    const lymph::options::Infer& infer_options = {});
```

## What It Does

`learn_map(...)` learns the category layout and immediately emits the encoded
result in one pass.

This is the normal path when you want a fitted encoder and the transformed
output from the same call.

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
- [map](map.md)
- [summary](summary.md)
