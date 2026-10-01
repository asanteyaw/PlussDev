# `pluss::ai::nn::TransformerOptions`

Model width, head count, independent encoder/decoder layer counts,
feedforward width, dropout, activation, norm placement.

Passed to [`Transformer`](Transformer.md)'s constructor -- or, more
commonly, spelled out as separate positional arguments, since
`Transformer` also has a constructor that takes the most-used fields
individually. Every field here except `num_encoder_layers` and
`num_decoder_layers` is forwarded, unchanged, into **both** the internal
[`TransformerEncoder`](TransformerEncoder.md) and
[`TransformerDecoder`](TransformerDecoder.md) this constructs -- the two
stacks share every option except their own layer count and their own
independent per-layer random seeds (offset from this struct's own `seed`
by different constants for the encoder side and the decoder side).

## Fields

### `d_model`

`std::int64_t d_model = 512`
: Forwarded to both the internal encoder's and decoder's own `d_model`.
  Must be evenly divisible by [`nhead`](#nhead).

### `nhead`

`std::int64_t nhead = 8`
: Forwarded to both the internal encoder's and decoder's own `nhead`.
  Must divide `d_model` evenly.

### `num_encoder_layers`

`std::int64_t num_encoder_layers = 6`
: How many layers the internal [`encoder()`](Transformer.md#state) has.
  Independent of `num_decoder_layers` -- an encoder-decoder `Transformer`
  is not required to have matching depths on both sides.

### `num_decoder_layers`

`std::int64_t num_decoder_layers = 6`
: How many layers the internal [`decoder()`](Transformer.md#state) has.
  Independent of `num_encoder_layers`.

### `dim_feedforward`

`std::int64_t dim_feedforward = 2048`
: Forwarded to both the internal encoder's and decoder's own
  `dim_feedforward`.

### `dropout`

`double dropout = 0.1`
: Forwarded to both the internal encoder's and decoder's own `dropout`.

### `activation`

`nn::TransformerActivation activation = nn::TransformerActivation::ReLU`
: Forwarded to both the internal encoder's and decoder's own
  `activation`.

### `layer_norm_eps`

`double layer_norm_eps = 1e-5`
: Forwarded to both the internal encoder's and decoder's own
  `layer_norm_eps`.

### `batch_first`

`bool batch_first = false`
: Forwarded to both the internal encoder's and decoder's own
  `batch_first` -- must match the layout of `src`/`tgt`/`memory` that
  [`Transformer::forward`](Transformer.md#methods) is called with.

### `norm_first`

`bool norm_first = false`
: Forwarded to both the internal encoder's and decoder's own
  `norm_first` -- pre- versus post-norm placement, identical on both
  sides.

### `bias`

`bool bias = true`
: Forwarded to both the internal encoder's and decoder's own `bias`.

### `cfg`

`pluss::Config cfg = {}`
: Forwarded to both the internal encoder's and decoder's own `cfg`.

### `seed`

`std::uint64_t seed = 0`
: The base seed both the internal encoder and decoder derive their own,
  independently-offset seed from (`seed + 1001` for the encoder,
  `seed + 2003` for the decoder internally) -- so changing `seed`
  reproducibly changes every weight on both sides together, while the
  encoder and decoder still never start with matching weights even when
  their shapes happen to coincide.

### `training`

`bool training = true`
: Forwarded to both the internal encoder's and decoder's own `training`.
  Ordinarily left at its default and controlled afterward through
  [`Module::train()`](../module/Module/train.md) /
  [`Module::eval()`](../module/Module/eval.md), which propagates to both
  internal stacks at once.

## Guidance

There is no field here for `final_norm` -- both the internal encoder and
decoder are always built with `final_norm=true`, unconditionally.
Otherwise, treat `d_model`/`nhead`/`dim_feedforward`/`dropout`/
`norm_first` exactly as with
[`TransformerEncoderOptions`](TransformerEncoderOptions.md#guidance).
`num_encoder_layers`/`num_decoder_layers` default to the original
Transformer paper's `6`/`6`; deeper on one side than the other is a
legitimate architecture choice (a strong encoder feeding a lighter
decoder, for instance), not a constraint this struct enforces either way.

## See Also

- [`Transformer`](Transformer.md) -- the only consumer of this options
  struct.
- [`TransformerEncoderOptions`](TransformerEncoderOptions.md) /
  [`TransformerDecoderOptions`](TransformerDecoderOptions.md) -- what
  this struct's shared fields are forwarded into, on each side
  respectively.
