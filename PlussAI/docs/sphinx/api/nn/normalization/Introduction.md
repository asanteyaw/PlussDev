# `nn::normalization` Introduction

## What This Category Owns

`normalization` owns layers that rescale a `Series`' statistics -- as
opposed to [`activation`](../activation.md), which reshapes values
pointwise without regard to the distribution they came from:

- [`LayerNorm`](LayerNorm.md) -- normalizes each row to zero mean / unit
  variance over its last dimension, then applies a learned per-feature
  scale (`weight`) and shift (`bias`). The only type in this category so
  far.

Other normalization schemes common elsewhere (batch-level statistics
across a minibatch, per-group statistics across a channel subset) are not
present in this engine yet -- there is exactly one `Series::layer_norm`
primitive underneath, and `LayerNorm` is its sole `Module` wrapper.

## Why Normalize At All

Deep stacks of linear-then-activation layers are sensitive to the scale of
their intermediate values: too large and activations saturate or gradients
explode, too small and gradients vanish. `LayerNorm` fixes each row's scale
independently of every other row (unlike batch-level schemes, it needs no
running statistics and behaves identically in training and evaluation
mode), then hands back the expressiveness a fixed `0`-mean/`1`-variance
constraint would otherwise remove via a learned affine transform on top.

## Parameters: Two, Not Zero Or One

Every type in [`activation`](../activation.md) has zero learnable
parameters except [`PReLU`](../activation/PReLU.md), which has one
(`weight`). `LayerNorm` has two -- `weight` and `bias` -- both shaped
`[normalized_size]` and both trained alongside the rest of the model, the
same way [`Fusion`](../fusion.md)'s `weight`/`bias` are.

## Choosing Where To Place It

There is no universally correct placement, but a reasonable default:

1. Place `LayerNorm` right after a [`Fusion`](../fusion.md) (or other
   linear) stage and before its activation -- normalizing the linear
   output's scale before the nonlinearity is the conventional ordering.
2. Set `normalized_size` to that linear stage's output width; a mismatch
   throws at `forward` time rather than silently misbehaving.
3. Leave `eps` at its default `1e-5` unless a row of exactly constant
   values is a realistic input for your data -- that is the only case it
   meaningfully affects.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract `LayerNorm` implements, and how layers compose into a
  `Schedule`.
- [`activation`](../activation.md) -- the elementwise nonlinearities
  `LayerNorm` is typically placed alongside.
- [`fusion`](../fusion.md) -- the linear layers `LayerNorm` most often
  follows.
