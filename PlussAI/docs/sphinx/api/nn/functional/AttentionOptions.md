# `pluss::ai::nn::AttentionOptions`

Dropout, training mode, causal masking, an optional explicit scale, and
the dropout seed.

Passed to [`scaled_attention`](scaled_attention.md)'s fifth argument,
defaulted, and nested inside [`MultiheadOptions`](MultiheadOptions.md)'s
own `attention` field, since [`multihead_attention`](multihead_attention.md)
builds its per-head attention from this same struct internally.

## Fields

### `dropout`

`double dropout = 0.0`
: The dropout probability applied to the softmax attention weights
  (after normalization, before the weighted sum over `value`). Must be
  in `[0, 1]`. `0.0` (the default) applies no dropout regardless of
  `training`.

### `training`

`bool training = true`
: Whether `dropout` is actually applied. `dropout > 0.0` has no effect
  at all when `training == false` -- the same convention
  [`dropout`](dropout.md) itself uses.

### `causal`

`bool causal = false`
: When `true`, blocks every query position from attending to any key
  position after it (standard autoregressive masking), combined with
  (not replacing) whatever `attn_mask` is separately passed to
  [`scaled_attention`](scaled_attention.md).

### `scale`

`double scale = 0.0`
: An explicit scaling factor applied to the raw `query @ key^T` logits
  before the softmax. The default `0.0` is a sentinel meaning "derive it
  from `head_dim` as `1 / sqrt(head_dim)`"; any other value is used
  verbatim.

### `seed`

`std::uint64_t seed = 0`
: The seed behind the dropout mask, when `dropout > 0.0` and `training
  == true`. The same `seed` reproduces the exact same dropped attention
  weights.

## Guidance

Leave every field at its default for ordinary (non-causal, non-dropout,
default-scale) attention. Set `causal=true` for autoregressive
self-attention; set `dropout` to a small positive value only when this
call sits inside a network being trained end to end. `scale` is rarely
touched -- only when a specific paper or checkpoint calls for a
non-standard scale.

## See Also

- [`scaled_attention`](scaled_attention.md) -- the direct consumer of
  this options struct.
- [`MultiheadOptions`](MultiheadOptions.md) -- nests this struct as its
  own `attention` field.
