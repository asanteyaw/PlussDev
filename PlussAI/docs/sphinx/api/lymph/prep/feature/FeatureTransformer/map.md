# `lymph::prep::FeatureTransformer::map`

## Signature

```cpp
StatusOr<pluss::Series> lymph::prep::FeatureTransformer::map(
    const lymph::FeatureBatch& batch,
    const lymph::options::Infer& options = {}) const;
```

## What It Does

`map(...)` applies the previously learned branch-local contract to a later
feature batch.

This is where the composite object proves its value: the same full routed plan
can be reused without reconstructing or refitting the branches.

## Related Pages

- [learn](learn.md)
- [learn_map](learn_map.md)
- [trained](trained.md)
