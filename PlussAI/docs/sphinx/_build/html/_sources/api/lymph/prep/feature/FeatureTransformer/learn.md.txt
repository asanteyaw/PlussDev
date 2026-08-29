# `lymph::prep::FeatureTransformer::learn`

## Signature

```cpp
Status lymph::prep::FeatureTransformer::learn(
    const lymph::FeatureBatch& batch,
    const lymph::options::Train& options = {});
```

## What It Does

`learn(...)` fits every branch-local transform in the feature transformer
without immediately emitting the mapped output.

Use it when you want to learn once and reuse the same composite transformer
across later batches.

## Related Pages

- [learn_map](learn_map.md)
- [map](map.md)
- [trained](trained.md)
