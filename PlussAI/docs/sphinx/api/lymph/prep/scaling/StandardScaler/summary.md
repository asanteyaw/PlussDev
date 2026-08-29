# `lymph::prep::StandardScaler::summary`

## Signature

```cpp
lymph::prep::StandardScalerSummary lymph::prep::StandardScaler::summary() const;
```

## What It Does

`summary()` exposes the fitted feature list and learned state in a structured
summary object.

Use it when you need to inspect what was learned rather than only consume the
transformed output.

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

- [learned](learned.md)
- [Notes](Notes.md)
