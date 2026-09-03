# Lymph `prep::feature`

`feature` is the `prep` subfamily for **branch-structured preprocessing** — one
[`FeatureTransformer`](feature/FeatureTransformer.md) object that runs several
independent, column-routed stage pipelines in parallel and concatenates their
outputs into one feature matrix.

Where a [`prep::Stage`](stage_family.md) is *one* transform on *one* route, and a
standalone [`scaler`](scaling.md) or [`encoder`](encoding.md) is *one* transform
you hold, a `FeatureTransformer` is the whole preprocessing contract: "scale
these columns, one-hot those, impute-then-scale that group, drop the rest" —
learned once, replayed as a unit.

## The pieces

| Entity | Role |
| --- | --- |
| [`prep::branch::Branch`](feature/branch.md) | one route + one ordered list of stages. Built with [`branch::compose`](feature/branch.md) or a one-stage builder like `branch::standard_scaler` |
| [`prep::feature_transformer(...)`](feature/feature_transformer.md) | the factory — takes the ordered feature space, a list of branches, and a [remainder policy](feature/feature_transformer.md#the-remainder-policy) |
| [`prep::FeatureTransformer`](feature/FeatureTransformer.md) | the fitted composite object — `learn` / `map` / `learn_map` / `trained` / `stage_count`, move-only |

## Shape of use

```cpp
lymph::prep::FeatureTransformer ft = lymph::prep::feature_transformer(
    frame,
    {
        lymph::prep::branch::compose(
            { lymph::prep::stage::simple_imputer({.strategy = /*Mean*/}),
              lymph::prep::stage::standard_scaler() },
            {"median_income"}),
        lymph::prep::branch::one_hot_encoder({}, {"ocean_proximity"}),
    },
    lymph::prep::FeatureRemainderPolicy::PassThrough);

model.feature_transformer(std::move(ft));   // hand it to a model, or drive it directly
```

The common home for a `FeatureTransformer` is a model, via
[`Model::feature_transformer`](../models/Model/append.md) — the model fits and
replays it as the first step of `learn` / `infer`. It can also be driven
directly on a `FeatureBatch` ([`FeatureTransformer` examples](feature/FeatureTransformer.md#practical-examples)).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

feature/Introduction
feature/FeatureTransformer
feature/feature_transformer
feature/branch
```
<!-- apiref:toctree:end -->
