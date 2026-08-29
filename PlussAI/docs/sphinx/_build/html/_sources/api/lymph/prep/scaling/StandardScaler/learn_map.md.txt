# `lymph::prep::StandardScaler::learn_map`

## Signature

```cpp
pluss::table::DataFrame lymph::prep::StandardScaler::learn_map(
    const pluss::table::DataFrame& frame,
    const lymph::options::Train& learn_options = {},
    const lymph::options::Infer& infer_options = {});
```

## What It Does

`learn_map(...)` is the one-pass fit-and-transform surface.

This is the normal path when you want the fitted scaler to immediately return a
transformed frame and still retain its learned state for later reuse.

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
- [summary](summary.md)
