# `pluss::ai::nn::TransformerDecoderLayerOptions`

Model width, head count, feedforward width, dropout, activation, norm
placement and epsilon, bias.

Passed to [`TransformerDecoderLayer`](TransformerDecoderLayer.md)'s
constructor -- or, more commonly, spelled out as separate positional
arguments, since `TransformerDecoderLayer` also has a constructor that
takes the most-used fields individually. Field-for-field identical to
[`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md);
the difference between the two lies entirely in what
`TransformerDecoderLayer::forward` does with them (an additional
cross-attention sub-block, and a third layer normalization to match), not
in the options themselves.

## Fields

### `d_model`

`std::int64_t d_model = 512`
: The width of the block's input, output, and `memory` feature axis.
  Sizes every internal sub-module: both the internal `self_attn` and
  `cross_attn`'s `embed_dim`, and `linear1`'s `in_features`/`linear2`'s
  `out_features`. Must be evenly divisible by [`nhead`](#nhead).

### `nhead`

`std::int64_t nhead = 8`
: How many attention heads both internal attention sub-modules
  (`self_attn` and `cross_attn`) split `d_model` into -- forwarded to
  each's own [`num_heads`](MultiheadAttentionOptions.md#num_heads). Must
  divide `d_model` evenly.

### `dim_feedforward`

`std::int64_t dim_feedforward = 2048`
: The width of the hidden layer inside the block's feedforward
  sub-network, identical in role to
  [`TransformerEncoderLayerOptions::dim_feedforward`](TransformerEncoderLayerOptions.md#dim_feedforward).

### `dropout`

`double dropout = 0.1`
: The dropout probability applied in four places inside `forward`: on
  both internal attention sub-modules' own attention weights (forwarded
  as each's own `dropout`), and twice more inside the feedforward
  sub-network. Must be in `[0, 1]`.

### `activation`

`nn::TransformerActivation activation = nn::TransformerActivation::ReLU`
: The elementwise nonlinearity applied between `linear1` and `linear2`.
  `ReLU` (the default) or `GELU` -- identical role to
  [`TransformerEncoderLayerOptions::activation`](TransformerEncoderLayerOptions.md#activation).

### `layer_norm_eps`

`double layer_norm_eps = 1e-5`
: The epsilon added to the variance inside every layer-normalization step
  this block performs -- three per block here (one per sub-block: self-
  attention, cross-attention, feedforward), one more than
  `TransformerEncoderLayerOptions`'s two. Must be `> 0`. Only reachable
  through the `TransformerDecoderLayerOptions` form.

### `batch_first`

`bool batch_first = false`
: Which axis of batched `tgt`/`memory` input is the sequence-length
  axis -- forwarded to both internal attention sub-modules' own
  `batch_first`.

### `norm_first`

`bool norm_first = false`
: Whether layer normalization runs before ("pre-norm") or after
  ("post-norm", the default) each of the three sub-blocks:

  - **post-norm**: `x = norm1(x + self_attn(x)); x = norm2(x +
    cross_attn(x, memory)); x = norm3(x + ff(x))`.
  - **pre-norm**: `x = x + self_attn(norm1(x)); x = x + cross_attn(norm2(x),
    memory); x = x + ff(norm3(x))`.

  Changes the actual arithmetic, not just bookkeeping -- see
  [`TransformerDecoderLayer::forward`](TransformerDecoderLayer.md#methods).

### `bias`

`bool bias = true`
: Whether `self_attn`, `cross_attn`, `linear1`, `linear2`, and all three
  layer-normalization steps each register a learned bias/shift
  parameter. When `false`, none of them do.

### `average_attn_weights`

`bool average_attn_weights = true`
: Forwarded to both internal attention sub-modules' own
  [`average_weights`](MultiheadAttentionOptions.md#average_weights).
  `forward` on this block never returns either sub-module's weights
  directly -- only matters when inspecting `self_attn()`/`cross_attn()`
  directly. Only reachable through the `TransformerDecoderLayerOptions`
  form.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform every internal sub-module's parameters are
  allocated with.

### `seed`

`std::uint64_t seed = 0`
: The base seed this block's internal `self_attn`/`cross_attn`/
  `linear1`/`linear2` each derive their own, distinctly-offset seed from.

### `training`

`bool training = true`
: Whether this instance (and every internal sub-module) starts in
  training mode. Ordinarily left at its default and controlled afterward
  through [`Module::train()`](../module/Module/train.md) /
  [`Module::eval()`](../module/Module/eval.md).

## Guidance

Identical guidance to
[`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md#guidance):
pick `d_model`/`nhead` to match the surrounding stack, `dim_feedforward`
conventionally around `4x` `d_model`, `dropout` at `0.1` for real
training, `norm_first=true` worth considering for deep stacks. The one
decoder-specific consideration is that `d_model` here must also match
whatever [`TransformerEncoder`](TransformerEncoder.md) or other source
produces the `memory` this block's `cross_attn` reads -- `cross_attn`'s
`key`/`value` come from `memory`, not `tgt`, so a `d_model` mismatch
between an encoder stack and a decoder stack built independently
surfaces as a `MultiheadAttention` shape error at `forward` time, not at
construction time.

## See Also

- [`TransformerDecoderLayer`](TransformerDecoderLayer.md) -- the only
  consumer of this options struct.
- [`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md)
  -- the encoder-side counterpart, field-for-field identical.
- [`MultiheadAttentionOptions`](MultiheadAttentionOptions.md) -- several
  fields here are forwarded directly into both internal attention
  sub-modules.
- [`TransformerDecoderOptions`](TransformerDecoderOptions.md) -- carries
  the same fields (plus `num_layers`/`final_norm`) to build a whole stack
  of these blocks at once.
