# `lymph::prep::OneHotEncoder::features`

## Signature

```cpp
lymph::prep::OneHotEncoder& lymph::prep::OneHotEncoder::features(
    std::initializer_list<std::string_view> names);
```

## What It Does

`features(...)` pins the encoder to a named categorical subset.

Use it when the incoming frame carries several columns and you want the encoder
to own the selection contract for the columns it should expand.

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

- [learn_map](learn_map.md)
- [map](map.md)
