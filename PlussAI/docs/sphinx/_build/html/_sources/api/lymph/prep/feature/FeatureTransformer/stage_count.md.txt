# `lymph::prep::FeatureTransformer::stage_count`

## Signature

```cpp
std::int64_t lymph::prep::FeatureTransformer::stage_count() const noexcept;
```

## What It Does

`stage_count()` reports how many branch-local stage pipelines the transformer
currently carries.

This is a fast sanity check that the composite object actually contains the
number of routed branches you expect.

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

- [trained](trained.md)
- [Notes](Notes.md)
