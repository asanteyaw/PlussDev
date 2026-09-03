# `prep::feature` Introduction

## What This Subfamily Owns

`prep::feature` owns branch-structured preprocessing: the
[`prep::FeatureTransformer`](FeatureTransformer.md) object, the
[`prep::feature_transformer(...)`](feature_transformer.md) factory that builds
one, and the [`prep::branch`](branch.md) surface (`Branch`, `compose`, and the
one-stage builders) for describing each branch.

## Branch, Not Chain

A single [`prep::Stage`](../stage_family.md) attached with `Model::append` is one
transform on one route, and several of them chained run in sequence over an
evolving frame. A `FeatureTransformer` is different: each **branch** takes its
own slice of the *original* feature space and runs its own pipeline, and the
branch outputs are concatenated — left to right, in branch order — into one
matrix. Columns claimed by no branch are the **remainder**, kept or dropped by
the [remainder policy](feature_transformer.md#the-remainder-policy).

- **branch A** → `{median_income}` → impute, then scale
- **branch B** → `{ocean_proximity}` → one-hot
- **remainder** → everything else → pass through (or drop)

That whole contract is learned once and replayed as a unit.

## The Lifecycle In One Line

```cpp
lymph::prep::FeatureTransformer ft = lymph::prep::feature_transformer(frame, { /* branches */ }, remainder);
model.feature_transformer(std::move(ft));
```

`feature_transformer(...)` assembles the object; the model fits it in `learn`
and replays it in `infer`. Driven directly, it is
`ft.learn_map(feature_batch)` — see the
[`FeatureTransformer` examples](FeatureTransformer.md#practical-examples).

## Related Pages

- [`prep::FeatureTransformer`](FeatureTransformer.md) — the object.
- [`prep::feature_transformer`](feature_transformer.md) — the factory.
- [`prep::branch`](branch.md) — how a branch is described.
- [`prep::stage`](../stage_family.md) — the stage factories a branch composes.
