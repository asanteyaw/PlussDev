# `pluss::ai::nn::MultiheadAttentionOptions`

Embedding width, head count, dropout, bias, batch-first layout, weight
averaging, and an optional explicit scale.

Passed to [`MultiheadAttention`](MultiheadAttention.md)'s constructor --
or, more commonly, spelled out as separate positional arguments, since
`MultiheadAttention` also has a constructor that takes the most-used
fields individually (everything except `scale`, which the positional form
always leaves at its "derive it from `head_dim`" default of `0.0`). See
[`MultiheadAttention`](MultiheadAttention.md)'s Construction section for
both forms side by side.

## Fields

### `embed_dim`

`std::int64_t embed_dim = 8`
: The width of the query/key/value feature axis this instance is built
  for -- every `q_weight`/`k_weight`/`v_weight`/`out_weight` is allocated
  `[embed_dim, embed_dim]`. `forward`'s `query`/`key`/`value` arguments
  must all have this exact width on their last axis; nothing about this
  layer adapts to a different width after construction. Must be evenly
  divisible by `num_heads` -- see that field.

### `num_heads`

`std::int64_t num_heads = 8`
: How many attention heads `embed_dim` is split into. Each head attends
  independently over a `embed_dim / num_heads`-wide slice (the "head
  dimension"), and the heads' outputs are concatenated back to `embed_dim`
  before the final output projection. Must divide `embed_dim` evenly --
  constructing with a `num_heads` that does not divide `embed_dim` throws
  immediately, before any weight is allocated. More heads means more,
  narrower, independent attention patterns over the same total width; one
  head is ordinary (single) scaled dot-product attention with an
  additional learned pre/post projection.

### `dropout`

`double dropout = 0.0`
: The dropout probability applied to the attention weights themselves
  (after the softmax, before the weighted sum over `value`) -- not to the
  input or output. Must be in `[0, 1]`. Applied only when
  [`training`](#training) is `true` and `dropout > 0.0`; a module in
  evaluation mode never drops attention weights regardless of this value.

### `bias`

`bool bias = true`
: When `true` (the default), a learned `[embed_dim]` bias vector is
  registered for each of the four projections (`q`, `k`, `v`, `out`) and
  added after that projection's matrix multiply. When `false`, none of the
  four bias vectors are registered at all -- every `*_bias()` accessor
  stays an undefined `Series`, and every projection is a pure linear map.

### `batch_first`

`bool batch_first = false`
: Which axis of a **batched** (`[*, *, embed_dim]`, rank 3) `query`/`key`/
  `value` is the sequence-length axis. `false` (the default) is time-major
  -- `[seq_len, batch, embed_dim]`; `true` is `[batch, seq_len,
  embed_dim]`. Has no effect on **unbatched** (rank 2, `[seq_len,
  embed_dim]`) input, which never has a batch axis to place. This is the
  same convention [`RNNOptions::batch_first`](../rnn/RNNOptions.md) and
  every other sequence-consuming layer in this reference uses.

### `average_weights`

`bool average_weights = true`
: What shape the second element of `forward`'s returned
  `tuple<Series, Series>` (the attention weights) has. `true` (the
  default) averages the weights across the head axis, returning
  `[target_len, source_len]` (unbatched) or `[batch, target_len,
  source_len]` (batched) -- one weight matrix per batch element, easy to
  visualize directly. `false` keeps every head's weights separate:
  `[num_heads, target_len, source_len]` (unbatched) or `[batch, num_heads,
  target_len, source_len]` (batched). Purely a post-processing choice --
  the actual attention computation, and the first returned element (the
  output), are identical either way.

### `scale`

`double scale = 0.0`
: An explicit scaling factor applied to the raw `query @ key^T` logits
  before the softmax, replacing the default `1 / sqrt(head_dim)`. The
  default `0.0` is a sentinel, not a literal scale of zero -- `0.0` means
  "derive it from `head_dim` the usual way"; any other value is used
  verbatim. Only worth setting when a specific paper or checkpoint
  specifies a non-standard scale; leave at `0.0` otherwise. The positional
  constructor has no parameter for this field, so reaching a non-default
  `scale` requires the `MultiheadAttentionOptions` form.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform every weight/bias is allocated with. Defaults to
  the engine's default `Config` (Float32 on CPU), the same default every
  other parameterized layer in this reference uses.

### `seed`

`std::uint64_t seed = 0`
: The base seed behind the four projection weights' (and, when `bias` is
  enabled, four bias vectors') random initialization -- each of the eight
  draws its own value from `seed` offset by a distinct constant
  internally, so changing `seed` changes all eight together, reproducibly.

### `training`

`bool training = true`
: Whether this instance starts in training mode (dropout active,
  `Module::is_training() == true`) or evaluation mode. Ordinarily left at
  its default and controlled afterward through
  [`Module::train()`](../module/Module/train.md) /
  [`Module::eval()`](../module/Module/eval.md), the same as every other
  layer; this field only matters at the exact moment of construction,
  before either of those has been called.

## Guidance

`embed_dim`/`num_heads` are the architecture: pick `embed_dim` to match
whatever feeds this layer, and `num_heads` as a divisor of it (8 heads on
a 512-wide embedding, for instance, gives 64-wide heads). Leave `dropout`
at `0.0` for inference-only or deterministic-training use, and set it to
a small positive value (commonly `0.1`) when this layer sits inside a
larger network being trained end to end. Leave `bias`/`cfg`/`scale` at
their defaults unless a specific reason applies; set `seed` explicitly
only when exact reproducibility across runs matters. `batch_first` and
`average_weights` are call-site conveniences -- match `batch_first` to
however the rest of the pipeline lays out its batches, and pick
`average_weights` based on whether the caller wants one interpretable
weight matrix per example or the full per-head detail.

## See Also

- [`MultiheadAttention`](MultiheadAttention.md) -- the only consumer of
  this options struct.
- [`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md) --
  builds its internal `self_attn` from a `MultiheadAttentionOptions`
  derived from its own `d_model`/`nhead`/`dropout`/`bias`.
