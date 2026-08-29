# `lymph::prep::FeatureTransformer::trained`

## Signature

```cpp
bool lymph::prep::FeatureTransformer::trained() const noexcept;
```

## What It Does

`trained()` tells you whether the composite transformer currently carries fitted
branch state.

This is the main guard before `map(...)`.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_prep_feature_transformer_docverify_demo.cpp
:language: cpp
```

Output

```text
== feature_transformer ==
trained = true, stage_count = 2, rows = 8, features = 7

pluss::Series(shape=[8,7], strides=[7,1], dtype=float64, platform=cpu, numel=56, contiguous=true)
data=[[-1.3608276, 0, ... , 0.2],
  [-1.0886621, 0, ... , 0.4],
  ...
 [1.3608276, 1, ... , 2.7]]
```

## Related Pages

- [map](map.md)
- [learn_map](learn_map.md)
