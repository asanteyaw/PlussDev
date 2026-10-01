# `pluss::ai::nn::RNNOptions`

The layer/hidden sizes, the nonlinearity, bias/bidirectionality/layout
toggles, and where the weights are allocated.

Passed to [`RNN`](RNN.md)'s constructor -- or, more commonly, spelled out
as separate positional arguments, since `RNN` also has a constructor that
takes the fields individually.

## Fields

### `input_size`

`std::int64_t input_size = 1`
: The width of each timestep's feature vector -- the size `forward`'s
  input must have on its last axis.

### `hidden_size`

`std::int64_t hidden_size = 1`
: The width of the hidden state, per direction, per layer. Also the
  output's last axis width when `bidirectional == false`.

### `num_layers`

`std::int64_t num_layers = 1`
: How many recurrent layers are stacked. Layer 0 reads `input_size`-wide
  input; every layer above reads the previous layer's output
  (`hidden_size * (bidirectional ? 2 : 1)` wide).

### `nonlinearity`

`std::string nonlinearity = "tanh"`
: The elementwise activation applied to each timestep's combined
  input+hidden projection -- `"tanh"` or `"relu"`, nothing else. Unlike
  [`GRU`](GRU.md)/[`LSTM`](LSTM.md), whose gate nonlinearities are fixed
  by the architecture itself, this is the one nonlinearity choice a plain
  `RNN` actually exposes.

### `bias`

`bool bias = true`
: When `true` (the default), each layer/direction's weight block includes
  a learned bias (see `split_bias` for whether that is one tensor or two).
  When `false`, no bias is registered anywhere.

### `batch_first`

`bool batch_first = false`
: When `false` (the default), `forward` expects and returns `[seq_len,
  batch, features]` (time-major). When `true`, it expects and returns
  `[batch, seq_len, features]` instead -- the same values either way, just
  transposed.

### `dropout`

`double dropout = 0.0`
: Dropout probability applied to each layer's output before it feeds the
  next layer -- never applied after the last layer, and never applied at
  all when `num_layers == 1`. Must be in `[0, 1]`.

### `bidirectional`

`bool bidirectional = false`
: When `true`, each layer runs twice -- once forward over the sequence,
  once over the time-reversed sequence -- and the two outputs are
  concatenated along the feature axis. Doubles `params()`'s size per
  layer and the output's feature width; the final hidden state gains a
  matching extra entry per layer.

### `split_bias`

`bool split_bias = false`
: When `false` (the default), each weight block's bias lives in one
  combined tensor (`NetWeight::bias`), added once to the input-side
  projection only. When `true`, the bias splits into two separate tensors
  (`bias_ih`, `bias_hh`), added to the input-side and hidden-side
  projections independently -- see [`RNN::forward`](RNN/forward.md) for
  exactly how that changes the per-timestep formula.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform every weight block is allocated with. Defaults
  to the engine's default `Config` (Float32 on CPU).

### `seed`

`std::uint64_t seed = 0`
: The base seed behind every weight block's random initialization; each
  `(layer, direction)` pair derives its own seed from this one, so
  different blocks never draw identical values.

### `training`

`bool training = true`
: The module's initial training-mode flag -- equivalent to constructing
  and then calling [`train(training)`](../module/Module/train.md).
  Affects whether `dropout` is actually applied (dropout is a no-op in
  evaluation mode) and, internally, whether backward-mode bookkeeping is
  set up at all.

## Guidance

Set `input_size`/`hidden_size` to match the data and the desired
representation width; `num_layers`/`bidirectional` are the main
architecture choices beyond that. Leave `split_bias` at its default
`false` unless a specific downstream consumer (a state-dict loaded from
elsewhere, for instance) requires the two-tensor bias layout. Leave `cfg`
at its default and use `.to(...)` on the constructed module for
platform/dtype changes.

## See Also

- [`RNN`](RNN.md) -- the only consumer of this options struct.
- [`GRUOptions`](GRUOptions.md) / [`LSTMOptions`](LSTMOptions.md) -- the
  same shared fields, on the gated recurrent architectures.
