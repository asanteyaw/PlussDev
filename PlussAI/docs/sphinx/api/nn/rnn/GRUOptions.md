# `pluss::ai::nn::GRUOptions`

The layer/hidden sizes, bias/bidirectionality/layout toggles, and where
the weights are allocated -- identical to
[`RNNOptions`](RNNOptions.md) except for one field: there is no
`nonlinearity` choice, since a GRU's gate nonlinearities (`sigmoid` for
the reset/update gates, `tanh` for the candidate) are fixed by the
architecture itself, not a configurable option.

Passed to [`GRU`](GRU.md)'s constructor -- or, more commonly, spelled out
as separate positional arguments.

## Fields

### `input_size`

`std::int64_t input_size = 1`
: The width of each timestep's feature vector.

### `hidden_size`

`std::int64_t hidden_size = 1`
: The width of the hidden state, per direction, per layer.

### `num_layers`

`std::int64_t num_layers = 1`
: How many recurrent layers are stacked.

### `bias`

`bool bias = true`
: When `true` (the default), each weight block includes a learned bias.

### `batch_first`

`bool batch_first = false`
: When `false` (the default), `forward` expects/returns `[seq_len, batch,
  features]`; when `true`, `[batch, seq_len, features]`.

### `dropout`

`double dropout = 0.0`
: Dropout probability applied between layers (never after the last, never
  when `num_layers == 1`). Must be in `[0, 1]`.

### `bidirectional`

`bool bidirectional = false`
: When `true`, each layer runs forward and over the time-reversed
  sequence, concatenating both outputs.

### `split_bias`

`bool split_bias = false`
: When `false` (the default), each weight block's bias lives in one
  combined tensor, added to the input-side gate projection only. When
  `true`, the bias splits into `bias_ih`/`bias_hh`, each added to its own
  side's projection independently -- see [`GRU::forward`](GRU/forward.md)
  for exactly how that changes the gate formulas.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform every weight block is allocated with.

### `seed`

`std::uint64_t seed = 0`
: The base seed behind every weight block's random initialization.

### `training`

`bool training = true`
: The module's initial training-mode flag.

## Guidance

Same guidance as [`RNNOptions`](RNNOptions.md): set `input_size`/
`hidden_size` to match the data, `num_layers`/`bidirectional` for the
architecture, leave `split_bias`/`cfg` at their defaults unless a specific
reason requires otherwise.

## See Also

- [`GRU`](GRU.md) -- the only consumer of this options struct.
- [`RNNOptions`](RNNOptions.md) / [`LSTMOptions`](LSTMOptions.md) -- the
  same shared fields on the other two recurrent architectures.
