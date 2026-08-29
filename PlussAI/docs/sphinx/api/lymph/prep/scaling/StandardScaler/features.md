# `lymph::prep::StandardScaler::features`

## Signature

```cpp
lymph::prep::StandardScaler& lymph::prep::StandardScaler::features(
    std::initializer_list<std::string_view> names);
```

## What It Does

`features(...)` pins the scaler to a named feature subset.

Use it when the surrounding frame contains more columns than the scaler should
touch, and you want that intent to live on the transformer itself instead of in
every later `learn(...)` or `map(...)` call.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_prep_standard_scaler_docverify_demo.cpp
:language: cpp
```

Output

```text
== standard_scaler ==
rows = 8, features = 3, summary_features = 3

=======================================
     | income        balance       late
---------------------------------------
0    | -1.360827635  -1.300637865  -1
1    | -1.088662108  -1.081583067  -1
---------------------------------------
[2 cs x 3 feat]
```

## Related Pages

- [learn](learn.md)
- [map](map.md)
