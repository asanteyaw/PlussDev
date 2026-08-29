# `lymph::prep::FeatureTransformer::learn_map`

## Signature

```cpp
StatusOr<pluss::Series> lymph::prep::FeatureTransformer::learn_map(
    const lymph::FeatureBatch& batch,
    const lymph::options::Train& learn_options = {},
    const lymph::options::Infer& infer_options = {});
```

## What It Does

`learn_map(...)` fits the branch-local transforms and returns the transformed
feature matrix in one pass.

This is the normal exploratory path when you want the composite transformer to
become learned and also want the mapped output immediately.

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

- [learn](learn.md)
- [map](map.md)
- [stage_count](stage_count.md)
