# `nn::fusion` Introduction

## What This Category Owns

`fusion` owns the fully connected (dense/linear) layer:

- [`Fusion`](Fusion.md) -- a learned affine map, `y = x @ weight^T + bias`.
  Accepts input of any rank; only the trailing axis is transformed, every
  leading (batch/sequence) axis is preserved untouched. The only type in
  this category, and the layer every worked `Schedule` example elsewhere
  in this reference already relies on.

There is exactly one dense-layer shape in this engine -- no separate
bilinear, low-rank, or grouped variant. `Fusion` covers the whole "learned
linear map" surface this reference exposes.

## Why This Is The Workhorse Layer

Every other layer family in this reference either reshapes values pointwise
([`activation`](../activation.md)), rescales a batch's statistics
([`normalization`](../normalization.md)), or looks values up by index
([`embedding`](../embedding.md)) -- none of them can mix information
*across* features. `Fusion` is the layer that actually combines a row's
features into a new representation: every output feature is a learned
weighted sum of every input feature (plus an optional bias). A network
with no `Fusion` (or comparable linear) stage at all cannot learn any
cross-feature interaction, no matter how many activation or normalization
stages it has.

## Parameters: Two, Like `LayerNorm`

`Fusion` has two learnable parameters -- `weight` and, unless
`bias=false`, `bias` -- the same count as
[`LayerNorm`](../normalization/LayerNorm.md), though `weight`'s shape
(`[out_features, in_features]`, potentially very large) is a different
order of magnitude from `LayerNorm`'s per-feature scale.

## Choosing `out_features` And `bias`

There is no formula that picks `out_features` automatically -- it is a
genuine architecture choice, typically shrinking toward a final
prediction's width or expanding to give a hidden representation more
capacity. A reasonable default path:

1. Leave `bias=true` (the default) for most layers -- the additive shift
   costs one extra `[out_features]` parameter and is rarely worth
   disabling.
2. Reach for `bias=false` specifically when the layer is immediately
   followed by a stage that re-centers its input anyway (a normalization
   layer), where the bias term would be redundant.
3. Match `in_features` to whatever actually feeds the layer -- the
   previous stage's `out_features`, [`Embedding`](../embedding/Embedding.md)'s
   `hidden_size`, or the raw feature count for a first layer -- a
   mismatch throws at `forward` time rather than silently misbehaving.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract `Fusion` implements, and how layers compose into a `Schedule`.
- [`activation`](../activation.md) -- the elementwise nonlinearities
  conventionally placed right after a `Fusion` stage.
- [`normalization`](../normalization.md) -- the other family whose only
  member also has two learnable parameters.
