# `pluss::ai::nn::TransformerDecoderOptions`

Per-layer options plus the layer count and whether to apply a final norm.

Passed to [`TransformerDecoder`](TransformerDecoder.md)'s constructor --
or, more commonly, spelled out as separate positional arguments, since
`TransformerDecoder` also has a constructor that takes the most-used
fields individually. Every field here except `num_layers` and
`final_norm` is forwarded, unchanged, into every one of the `num_layers`
internal [`TransformerDecoderLayer`](TransformerDecoderLayer.md)
instances this constructs -- field-for-field identical in role to
[`TransformerEncoderOptions`](TransformerEncoderOptions.md), just
building decoder blocks instead of encoder blocks.

## Fields

### `d_model`

`std::int64_t d_model = 512`
: Forwarded to every internal layer's own
  [`d_model`](TransformerDecoderLayerOptions.md#d_model), and also the
  width of this class's own final layer-normalization pair, when
  [`final_norm`](#final_norm) is enabled. Must also match whatever
  `memory` this stack's `forward` is eventually called with.

### `nhead`

`std::int64_t nhead = 8`
: Forwarded to every internal layer's own
  [`nhead`](TransformerDecoderLayerOptions.md#nhead). Must divide
  `d_model` evenly.

### `num_layers`

`std::int64_t num_layers = 6`
: How many `TransformerDecoderLayer` instances are stacked. Must be `>
  0`. Reachable afterward through
  [`depth()`](TransformerDecoder.md#state), and each individual layer
  through [`layer(index)`](TransformerDecoder.md#state).

### `dim_feedforward`

`std::int64_t dim_feedforward = 2048`
: Forwarded to every internal layer's own
  [`dim_feedforward`](TransformerDecoderLayerOptions.md#dim_feedforward).

### `dropout`

`double dropout = 0.1`
: Forwarded to every internal layer's own
  [`dropout`](TransformerDecoderLayerOptions.md#dropout).

### `activation`

`nn::TransformerActivation activation = nn::TransformerActivation::ReLU`
: Forwarded to every internal layer's own
  [`activation`](TransformerDecoderLayerOptions.md#activation).

### `layer_norm_eps`

`double layer_norm_eps = 1e-5`
: Forwarded to every internal layer's own `layer_norm_eps`, and also used
  by this class's own final layer-normalization pair, when `final_norm`
  is enabled.

### `batch_first`

`bool batch_first = false`
: Forwarded to every internal layer's own
  [`batch_first`](TransformerDecoderLayerOptions.md#batch_first).

### `norm_first`

`bool norm_first = false`
: Forwarded to every internal layer's own
  [`norm_first`](TransformerDecoderLayerOptions.md#norm_first) -- pre-
  versus post-norm placement, identical across every layer in the stack.

### `bias`

`bool bias = true`
: Forwarded to every internal layer's own
  [`bias`](TransformerDecoderLayerOptions.md#bias), and also whether this
  class's own final layer-normalization pair registers a bias.

### `final_norm`

`bool final_norm = true`
: Whether an additional layer normalization runs once, after every
  internal layer has run, on the whole stack's output. When `false`, this
  class registers no `norm_weight`/`norm_bias` parameters, and
  `forward`'s output is exactly the last internal layer's own output.

### `cfg`

`pluss::Config cfg = {}`
: Forwarded to every internal layer's own `cfg`, and also used for this
  class's own final normalization pair.

### `seed`

`std::uint64_t seed = 0`
: The base seed layer `i` derives its own seed from (offset by a distinct
  constant per layer index), and also the seed behind this class's own
  final normalization pair's initialization.

### `training`

`bool training = true`
: Forwarded to every internal layer's own `training`. Ordinarily left at
  its default and controlled afterward through
  [`Module::train()`](../module/Module/train.md) /
  [`Module::eval()`](../module/Module/eval.md), which propagates to
  every internal layer at once.

## Guidance

Identical to
[`TransformerEncoderOptions`](TransformerEncoderOptions.md#guidance):
`num_layers` is a depth choice with no formula, `final_norm` should stay
at its default `true` for the standard Transformer-decoder shape. As with
[`TransformerDecoderLayerOptions`](TransformerDecoderLayerOptions.md#guidance),
`d_model` here must match whatever produces the `memory` this stack's
`forward` is called with -- typically a
[`TransformerEncoder`](TransformerEncoder.md)'s own output, or, when
composed through [`Transformer`](Transformer.md), that composition is
handled automatically.

## See Also

- [`TransformerDecoder`](TransformerDecoder.md) -- the only consumer of
  this options struct.
- [`TransformerDecoderLayerOptions`](TransformerDecoderLayerOptions.md)
  -- the per-layer fields this struct forwards unchanged into every
  internal layer.
- [`TransformerEncoderOptions`](TransformerEncoderOptions.md) -- the
  encoder-side counterpart, field-for-field identical.
