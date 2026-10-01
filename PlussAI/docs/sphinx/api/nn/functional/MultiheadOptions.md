# `pluss::ai::nn::MultiheadOptions`

Head count, batch-first layout, attention-weight averaging, and the
wrapped `AttentionOptions`.

Passed to [`multihead_attention`](multihead_attention.md)'s fifth
argument, defaulted.

## Fields

### `num_heads`

`std::int64_t num_heads = 8`
: How many attention heads `query`/`key`/`value`'s last axis
  (`embed_dim`) is split into. Must divide `embed_dim` evenly.

### `batch_first`

`bool batch_first = false`
: Which axis of a batched (rank-3) `query`/`key`/`value` is the
  sequence-length axis. Has no effect on unbatched (rank-2) input.

### `average_weights`

`bool average_weights = true`
: Whether the returned attention weights are merged across heads
  (`true`, the default -- `[target_len, source_len]` or `[batch,
  target_len, source_len]`) or kept separate (`false` --
  `[num_heads, target_len, source_len]` or `[batch, num_heads,
  target_len, source_len]`).

### `attention`

`nn::AttentionOptions attention = {}`
: The [`AttentionOptions`](AttentionOptions.md) forwarded to the
  underlying per-head [`scaled_attention`](scaled_attention.md) call --
  `dropout`, `training`, `causal`, `scale`, `seed`.

## Guidance

Field-for-field identical in role to
[`MultiheadAttentionOptions`](../attention/MultiheadAttentionOptions.md)
(minus `embed_dim`/`bias`/`cfg`/`seed`, which have no meaning here since
there is no `Module` to construct or own parameters) -- see that page's
own guidance for `num_heads`/`batch_first`/`average_weights`. Set
`attention.causal`/`attention.dropout` the same way
[`AttentionOptions`](AttentionOptions.md#guidance) itself recommends.

## See Also

- [`multihead_attention`](multihead_attention.md) -- the only consumer
  of this options struct.
- [`AttentionOptions`](AttentionOptions.md) -- the nested `attention`
  field.
- [`MultiheadAttentionOptions`](../attention/MultiheadAttentionOptions.md)
  -- the stateful `Module` wrapper's analogous options.
