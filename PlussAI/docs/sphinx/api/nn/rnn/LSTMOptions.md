# `pluss::ai::nn::LSTMOptions`

The layer/hidden sizes, bias/bidirectionality/layout toggles, the optional
recurrent projection size, and where the weights are allocated --
identical to [`GRUOptions`](GRUOptions.md) plus one extra field,
`proj_size`.

Passed to [`LSTM`](LSTM.md)'s constructor -- or, more commonly, spelled
out as separate positional arguments.

## Fields

### `input_size`

`std::int64_t input_size = 1`
: The width of each timestep's feature vector.

### `hidden_size`

`std::int64_t hidden_size = 1`
: The width of the **cell** state, per direction, per layer. When
  `proj_size == 0`, this is also the hidden/output state's width; when
  `proj_size > 0`, the hidden/output state is `proj_size`-wide instead --
  see `proj_size` below.

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
: Dropout probability applied between layers.

### `bidirectional`

`bool bidirectional = false`
: When `true`, each layer runs forward and over the time-reversed
  sequence, concatenating both outputs.

### `split_bias`

`bool split_bias = false`
: When `false` (the default), each weight block's bias lives in one
  combined tensor, added to the input-side gate projection only. When
  `true`, it splits into `bias_ih`/`bias_hh`.

### `proj_size`

`std::int64_t proj_size = 0`
: When `> 0`, the hidden/output state (but **not** the cell state) is
  projected down from `hidden_size` to `proj_size` at every timestep, via
  an extra learned `weight_hr` matrix -- see
  [`LSTM::forward`](LSTM/forward.md)'s Example 2 for exactly which shapes
  change. `0` (the default) disables the projection entirely; `weight_hr`
  stays undefined in that case. Must be `>= 0`, and (when non-zero)
  conventionally smaller than `hidden_size`, though nothing enforces that
  ordering.

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

Same guidance as [`GRUOptions`](GRUOptions.md) for the shared fields.
Reach for `proj_size` specifically when the cell state needs more capacity
than the recurrent connection actually needs to carry forward -- a larger
`hidden_size` gives the cell more room to remember, while a smaller
`proj_size` keeps the per-timestep recurrent matmul (and the next layer's
input width, if stacked) cheaper. Leave it at `0` otherwise; there is no
benefit to projecting when `hidden_size` alone is already the right width.

## See Also

- [`LSTM`](LSTM.md) -- the only consumer of this options struct.
- [`RNNOptions`](RNNOptions.md) / [`GRUOptions`](GRUOptions.md) -- the
  same shared fields on the other two recurrent architectures, neither of
  which has a projection.
