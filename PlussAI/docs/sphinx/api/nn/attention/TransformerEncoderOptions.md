# `pluss::ai::nn::TransformerEncoderOptions`

Per-layer options plus the layer count and whether to apply a final norm.

Passed to [`TransformerEncoder`](TransformerEncoder.md)'s constructor --
or, more commonly, spelled out as separate positional arguments, since
`TransformerEncoder` also has a constructor that takes the most-used
fields individually. Every field here except `num_layers` and
`final_norm` is forwarded, unchanged, into every one of the `num_layers`
internal [`TransformerEncoderLayer`](TransformerEncoderLayer.md)
instances this constructs -- there is no per-layer variation; every
layer in the stack shares the identical configuration (only the random
seed differs per layer, so the layers do not all start with identical
weights).

## Fields

### `d_model`

`std::int64_t d_model = 512`
: Forwarded to every internal layer's own
  [`d_model`](TransformerEncoderLayerOptions.md#d_model), and also the
  width of this class's own final layer-normalization pair, when
  [`final_norm`](#final_norm) is enabled.

### `nhead`

`std::int64_t nhead = 8`
: Forwarded to every internal layer's own
  [`nhead`](TransformerEncoderLayerOptions.md#nhead). Must divide
  `d_model` evenly.

### `num_layers`

`std::int64_t num_layers = 6`
: How many `TransformerEncoderLayer` instances are stacked, each built
  from the rest of these fields. Must be `> 0`. Reachable afterward
  through [`depth()`](TransformerEncoder.md#state), and each individual
  layer through [`layer(index)`](TransformerEncoder.md#state).

### `dim_feedforward`

`std::int64_t dim_feedforward = 2048`
: Forwarded to every internal layer's own
  [`dim_feedforward`](TransformerEncoderLayerOptions.md#dim_feedforward).

### `dropout`

`double dropout = 0.1`
: Forwarded to every internal layer's own
  [`dropout`](TransformerEncoderLayerOptions.md#dropout).

### `activation`

`nn::TransformerActivation activation = nn::TransformerActivation::ReLU`
: Forwarded to every internal layer's own
  [`activation`](TransformerEncoderLayerOptions.md#activation) -- `ReLU`
  or `GELU`.

### `layer_norm_eps`

`double layer_norm_eps = 1e-5`
: Forwarded to every internal layer's own
  [`layer_norm_eps`](TransformerEncoderLayerOptions.md#layer_norm_eps),
  and also used by this class's own final layer-normalization pair, when
  [`final_norm`](#final_norm) is enabled.

### `batch_first`

`bool batch_first = false`
: Forwarded to every internal layer's own
  [`batch_first`](TransformerEncoderLayerOptions.md#batch_first) --
  which axis of a batched `src` is the sequence-length axis.

### `norm_first`

`bool norm_first = false`
: Forwarded to every internal layer's own
  [`norm_first`](TransformerEncoderLayerOptions.md#norm_first) -- pre-
  versus post-norm placement, identical across every layer in the stack.

### `bias`

`bool bias = true`
: Forwarded to every internal layer's own
  [`bias`](TransformerEncoderLayerOptions.md#bias), and also whether this
  class's own final layer-normalization pair registers a bias, when
  [`final_norm`](#final_norm) is enabled.

### `final_norm`

`bool final_norm = true`
: Whether an additional layer normalization runs once, after every
  internal layer has run, on the whole stack's output. When `false`, no
  such normalization exists at all -- this class registers no
  `norm_weight`/`norm_bias` parameters, and `forward`'s output is exactly
  the last internal layer's own output, unchanged.

### `cfg`

`pluss::Config cfg = {}`
: Forwarded to every internal layer's own
  [`cfg`](TransformerEncoderLayerOptions.md#cfg), and also used for this
  class's own final normalization pair.

### `seed`

`std::uint64_t seed = 0`
: The base seed layer `i` derives its own seed from (offset by a distinct
  constant per layer index, so no two layers in the stack start with
  identical weights even though they share every other option), and also
  the seed behind this class's own final normalization pair's
  initialization.

### `training`

`bool training = true`
: Forwarded to every internal layer's own
  [`training`](TransformerEncoderLayerOptions.md#training). Ordinarily
  left at its default and controlled afterward through
  [`Module::train()`](../module/Module/train.md) /
  [`Module::eval()`](../module/Module/eval.md), which propagates to every
  internal layer at once.

## Guidance

There is no field here for per-layer `average_attn_weights` -- every
internal layer's own `self_attn` is always built with
`average_attn_weights=true`, regardless of anything set here. Reaching a
`false` value requires mutating a specific layer's own `self_attn`
options directly after construction (`encoder->layer(i)->self_attn()->options().average_weights
= false;`), not anything reachable from `TransformerEncoderOptions`
itself. Otherwise, treat this exactly like
[`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md)
with two additions: `num_layers` (an architecture depth choice with no
formula -- deeper stacks are more expressive and more expensive) and
`final_norm` (leave at its default `true` for the standard
Transformer-encoder shape; disable only when composing this into a larger
architecture that already applies its own final normalization
downstream).

## See Also

- [`TransformerEncoder`](TransformerEncoder.md) -- the only consumer of
  this options struct.
- [`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md)
  -- the per-layer fields this struct forwards unchanged into every
  internal layer.
