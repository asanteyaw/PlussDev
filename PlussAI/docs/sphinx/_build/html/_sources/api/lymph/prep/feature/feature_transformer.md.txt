# `lymph::prep::feature_transformer`

## Signature

```cpp
lymph::prep::FeatureTransformer lymph::prep::feature_transformer(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    std::initializer_list<lymph::prep::Branch> branches,
    lymph::prep::FeatureRemainderPolicy remainder = lymph::prep::FeatureRemainderPolicy::PassThrough);
```

## What It Does

This factory builds a branch-aware composite feature transformer from:

- an ordered feature space
- one or more branch-local preprocessing plans
- one remainder policy

This is the entry point you use when several routed transformations must become
one reusable object.

## Practical Example

```{literalinclude} ../../../../../../examples/NodeJ/lymph/lymph_prep_feature_transformer_docverify_demo.cpp
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

- [FeatureTransformer](FeatureTransformer.md)
- [../route/Route.md](../route/Route.md)
- [../stage/Stage.md](../stage/Stage.md)
