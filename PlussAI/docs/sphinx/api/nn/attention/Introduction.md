# `nn::attention` Introduction

## What This Category Owns

`attention` owns the attention-based layers, built in two layers:

- [`MultiheadAttention`](MultiheadAttention.md) -- the primitive: learned
  multi-head scaled dot-product attention over query/key/value inputs.
- The Transformer-family blocks/stacks built on top of it --
  [`TransformerEncoderLayer`](TransformerEncoderLayer.md)/
  [`TransformerEncoder`](TransformerEncoder.md) (self-attention +
  feedforward, and a stack of that), and
  [`TransformerDecoderLayer`](TransformerDecoderLayer.md)/
  [`TransformerDecoder`](TransformerDecoder.md) (self-attention +
  cross-attention + feedforward, and a stack of that), composed together
  into [`Transformer`](Transformer.md), the full encoder-decoder model.

Every attention computation in this family -- inside `MultiheadAttention`
directly, or inside a Transformer-family block's own `self_attn`/
`cross_attn` -- always computes and returns its attention weights
alongside its output; there is no toggle to skip that computation.
[`MultiheadAttentionOptions::average_weights`](MultiheadAttentionOptions.md#average_weights)
only changes whether those weights are merged across heads, never
whether they exist.

## Same Shape Conventions Throughout

- `MultiheadAttention` accepts rank 2 (unbatched, `[seq_len, embed_dim]`)
  or rank 3 (batched, laid out per `batch_first`) query/key/value input.
  Every Transformer-family block/stack only ever handles the batched
  (rank 3) case in practice, since a real model's `src`/`tgt` is
  virtually always batched, but the underlying `MultiheadAttention`
  sub-modules they build still support the unbatched form directly.
- `batch_first` means the same thing everywhere in this family it means
  in [`rnn`](../rnn.md): `false` (the default) is time-major
  (`[seq_len, batch, *]`), `true` is `[batch, seq_len, *]`.
- Every Transformer-family block/stack preserves its input's shape
  exactly -- `d_model` in, `d_model` out, at every level from a single
  `TransformerEncoderLayer` up through the full `Transformer`.

## Why Attention Instead Of Recurrence

[`rnn`](../rnn.md)'s layers carry a hidden state forward one timestep at
a time -- position `t`'s output can only depend on positions `0..t`
indirectly, through whatever the hidden state managed to summarize by
the time it got there. Attention instead lets every position look
directly at every other position in a single computation, with no
summarization bottleneck and no inherent notion of "before" or "after"
at all -- order has to be supplied separately (typically as a positional
encoding added to the input before it ever reaches these layers, which
is not something this family provides). The tradeoff is cost: computing
every position's relationship to every other position is quadratic in
sequence length, where a recurrent layer's cost is linear.

## Building Masks For Autoregressive Decoding

Two free functions in this family build the boolean masks a
`tgt_mask`/`attn_mask`/`src_mask` argument expects, as an alternative to
the `causal`/`is_causal`/`tgt_is_causal` boolean toggle every `forward`
in this family also accepts directly:

- `generate_square_subsequent_mask(size)` -- an explicit `[size, size]`
  boolean mask, `true` at `[i, j]` exactly where `j > i` (blocking every
  position from attending to any position after it). Passing this as
  `attn_mask` to [`MultiheadAttention::forward`](MultiheadAttention.md#methods)
  produces bit-for-bit the same output as `causal=true` -- it is the same
  mask, built once and reusable, rather than recomputed by every call.
- `generate_causal_mask(target_len, source_len)` -- the non-square
  generalization, `[target_len, source_len]`, for when the query and key
  sequences have different lengths (the shape
  [`TransformerDecoderLayer`](TransformerDecoderLayer.md)'s cross-attention
  needs is never square in general, though cross-attention is not
  ordinarily masked this way in practice -- this generalization exists
  for self-attention over target sequences whose length differs from a
  reused mask's original size).

Both also have an overload taking a `const Series& like` in place of an
explicit `Config`, inferring the mask's platform from `like`'s own
platform.

## Choosing Among The Pieces

1. Reach for [`MultiheadAttention`](MultiheadAttention.md) directly only
   when building something that is not itself an ordinary Transformer
   block -- most uses go through one of the blocks below instead.
2. Reach for [`TransformerEncoderLayer`](TransformerEncoderLayer.md) /
   [`TransformerEncoder`](TransformerEncoder.md) for an encoder-only
   architecture (classification, representation learning, anything that
   does not need to *generate* a sequence).
3. Reach for [`Transformer`](Transformer.md) directly for a full
   sequence-to-sequence architecture (translation, summarization,
   anything that transforms one sequence into a genuinely different
   one) -- it composes an encoder and a decoder stack for you, rather
   than requiring the two to be wired together by hand.
4. Reach for [`TransformerDecoderLayer`](TransformerDecoderLayer.md) /
   [`TransformerDecoder`](TransformerDecoder.md) directly only when a
   decoder-only architecture is genuinely wanted without a paired
   encoder (a `cross_attn` with no independent encoder-provided `memory`
   still needs *some* `memory` argument -- typically the same sequence
   used as `tgt`, an architecture choice this family supports but does
   not build for you).

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract every type here implements, and how layers compose into a
  `Schedule`.
- [`rnn`](../rnn.md) -- the sequence-at-a-time alternative.
- [`fusion`](../fusion.md) -- every feedforward sub-network in this
  family is built from [`Fusion`](../fusion/Fusion.md) layers directly.
