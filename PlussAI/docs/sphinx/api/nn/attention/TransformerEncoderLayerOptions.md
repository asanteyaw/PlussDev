# `pluss::ai::nn::TransformerEncoderLayerOptions`

Model width, head count, feedforward width, dropout, activation, norm
placement and epsilon, bias.

Passed to [`TransformerEncoderLayer`](TransformerEncoderLayer.md)'s
constructor -- or, more commonly, spelled out as separate positional
arguments, since `TransformerEncoderLayer` also has a constructor that
takes the most-used fields individually. The positional form has no
parameters for `layer_norm_eps` or `average_attn_weights` -- both stay
fixed at `1e-5` and `true` respectively unless the
`TransformerEncoderLayerOptions` form is used directly.

## Fields

### `d_model`

`std::int64_t d_model = 512`
: The width of the block's input and output feature axis. Every internal
  sub-module is sized from this: the internal
  [`self_attn`](TransformerEncoderLayer.md#state)'s `embed_dim`, and
  `linear1`'s `in_features`/`linear2`'s `out_features`. Must be evenly
  divisible by [`nhead`](#nhead).

### `nhead`

`std::int64_t nhead = 8`
: How many attention heads the internal `self_attn` splits `d_model`
  into -- forwarded directly as `self_attn`'s
  [`num_heads`](MultiheadAttentionOptions.md#num_heads). Must divide
  `d_model` evenly.

### `dim_feedforward`

`std::int64_t dim_feedforward = 2048`
: The width of the hidden layer inside the block's feedforward
  sub-network -- `linear1` maps `d_model -> dim_feedforward`, an
  activation is applied, then `linear2` maps `dim_feedforward ->
  d_model`. Wider feedforward layers add representational capacity (and
  compute cost) without changing `d_model`, the width every other part of
  a `Transformer`-family stack has to agree on.

### `dropout`

`double dropout = 0.1`
: The dropout probability applied in three places inside `forward`: on
  the internal `self_attn`'s attention weights (forwarded as its own
  [`dropout`](MultiheadAttentionOptions.md#dropout)), and twice more
  inside the feedforward sub-network (after the activation, and again
  after `linear2`). Must be in `[0, 1]`. Applied only when
  [`training`](#training) is `true`.

### `activation`

`nn::TransformerActivation activation = nn::TransformerActivation::ReLU`
: The elementwise nonlinearity applied between `linear1` and `linear2`
  inside the feedforward sub-network. Two values: `ReLU` (the default) or
  `GELU`. This is the one architecture choice with no numeric field --
  pick whichever a given paper or checkpoint specifies; `GELU` is the more
  common choice in recent transformer designs, `ReLU` the cheaper,
  older-style default.

### `layer_norm_eps`

`double layer_norm_eps = 1e-5`
: The epsilon added to the variance inside every layer-normalization step
  this block performs (two per block: one after/before the attention
  sub-block, one after/before the feedforward sub-block, depending on
  [`norm_first`](#norm_first)), preventing division by a near-zero
  variance. Must be `> 0`. Left at its default in virtually every case;
  only reachable through the `TransformerEncoderLayerOptions` form, since
  the positional constructor has no parameter for it.

### `batch_first`

`bool batch_first = false`
: Which axis of a batched (rank-3) `src` is the sequence-length axis --
  `false` (the default) is time-major (`[seq_len, batch, d_model]`),
  `true` is `[batch, seq_len, d_model]`. Forwarded to the internal
  `self_attn`'s own [`batch_first`](MultiheadAttentionOptions.md#batch_first).
  The same convention every sequence-consuming layer in this reference
  uses.

### `norm_first`

`bool norm_first = false`
: Whether layer normalization runs before ("pre-norm", `true`) or after
  ("post-norm", `false`, the default) each sub-block. Post-norm is the
  original Transformer paper's placement; pre-norm is the more common
  choice in later, deeper designs, since it tends to make very deep
  stacks easier to train (gradients reach the input sub-blocks without
  passing through a normalization step first). Changing this changes
  `forward`'s actual arithmetic, not just bookkeeping -- see
  [`TransformerEncoderLayer::forward`](TransformerEncoderLayer.md#methods).

### `bias`

`bool bias = true`
: Whether the internal `self_attn`, `linear1`, `linear2`, and the two
  layer-normalization steps each register a learned bias/shift parameter.
  When `false`, none of them do -- every sub-module's own bias-related
  accessors become undefined `Series` values, matching what `bias=false`
  means for [`MultiheadAttention`](MultiheadAttentionOptions.md#bias) and
  [`Fusion`](../fusion/FusionOptions.md#bias) individually.

### `average_attn_weights`

`bool average_attn_weights = true`
: Forwarded directly to the internal `self_attn`'s own
  [`average_weights`](MultiheadAttentionOptions.md#average_weights) --
  whether the attention weights `self_attn` computes internally are
  merged across heads. `forward` on this block never returns those
  weights at all (only the transformed `src`), so this field only matters
  when inspecting `self_attn`'s own weights directly through
  [`self_attn()`](TransformerEncoderLayer.md#state). Only reachable
  through the `TransformerEncoderLayerOptions` form.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform every internal sub-module's parameters are
  allocated with. Defaults to the engine's default `Config` (Float32 on
  CPU), the same default every other parameterized layer in this
  reference uses.

### `seed`

`std::uint64_t seed = 0`
: The base seed this block's internal `self_attn`/`linear1`/`linear2`
  each derive their own, distinctly-offset seed from -- so changing
  `seed` reproducibly changes every internal weight together.

### `training`

`bool training = true`
: Whether this instance (and every internal sub-module) starts in
  training mode. Ordinarily left at its default and controlled afterward
  through [`Module::train()`](../module/Module/train.md) /
  [`Module::eval()`](../module/Module/eval.md).

## Guidance

`d_model`/`nhead` are the architecture backbone -- pick `d_model` to
match the rest of the stack, `nhead` as a divisor of it. `dim_feedforward`
is conventionally `4x` `d_model` in the original Transformer paper (e.g.
`512`/`2048`), though this is a convention, not a requirement. Leave
`dropout` at its default `0.1` for training a real network end to end;
drop it to `0.0` for deterministic inference or debugging. `norm_first`
is worth setting explicitly for deep stacks (prefer `true` past a modest
depth) but otherwise safe to leave at its post-norm default. Leave
`activation`/`bias`/`layer_norm_eps`/`cfg` at their defaults unless a
specific architecture calls for a change; set `seed` explicitly only when
exact reproducibility across runs matters.

## See Also

- [`TransformerEncoderLayer`](TransformerEncoderLayer.md) -- the only
  consumer of this options struct.
- [`MultiheadAttentionOptions`](MultiheadAttentionOptions.md) -- several
  fields here (`nhead`, `dropout`, `bias`, `batch_first`,
  `average_attn_weights`) are forwarded directly into one of these.
- [`TransformerEncoderOptions`](TransformerEncoderOptions.md) -- carries
  the same fields (plus `num_layers`/`final_norm`) to build a whole stack
  of these blocks at once.
