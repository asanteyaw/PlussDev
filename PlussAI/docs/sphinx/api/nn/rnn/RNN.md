# `pluss::ai::nn::RNN`

A multi-layer Elman RNN: at each timestep, every layer combines that
timestep's input with its own previous hidden state through a single
learned linear projection, then applies `tanh` or `relu`. Optionally
stacked (`num_layers > 1`) and optionally bidirectional (each layer also
runs over the time-reversed sequence, concatenating both directions'
output). See [`Introduction`](Introduction.md) for how this compares to
[`GRU`](GRU.md)/[`LSTM`](LSTM.md), and [`forward`](RNN/forward.md) for the
exact per-timestep formula and every input/output shape.

## Signature

```cpp
explicit nn::RNN::RNN(nn::RNNOptions options);
nn::RNN::RNN(std::int64_t input_size, std::int64_t hidden_size, std::int64_t num_layers = 1,
             std::string nonlinearity = "tanh", bool bias = true, bool batch_first = false,
             double dropout = 0.0, bool bidirectional = false, bool split_bias = false);

void nn::RNN::reset_parameters();
```

## Construction

Two equivalent forms: pass an [`RNNOptions`](RNNOptions.md) directly, or
spell out the same fields as positional arguments:

```cpp
nn::RNN a(nn::RNNOptions{16, 32});  // input_size=16, hidden_size=32, everything else defaulted
nn::RNN b(16, 32);                  // equivalent positional form
```

Both forms call `reset_parameters()` once, which allocates and fills
every `(layer, direction)` weight block.

## State

`params()` (mutable and `const` overloads, reachable only via `->`) --
`vector<pluss::ai::rnn::NetWeight>&` / `const vector<...>&`
: One entry per `(layer, direction)` pair, in that nested order (layer 0's
  forward direction, then layer 0's backward direction if bidirectional,
  then layer 1's, ...). Every entry's `weight_ih`/`weight_hh` (and
  `bias`/`bias_ih`/`bias_hh`) are registered parameters -- see
  [`params`](RNN/params.md) for the exact field-by-field contract.

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the [`RNNOptions`](RNNOptions.md) this instance was constructed
with -- see [`options`](RNN/options.md).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `forward(input, hx = {})` / `forward(packed, hx = {})` / `operator()(...)` | `tuple<Series, Series>` / `tuple<PackedSequence, Series>` | [`forward`](RNN/forward.md) |
| `params()` | `vector<NetWeight>&` (or `const vector<NetWeight>&`) | [`params`](RNN/params.md) |
| `options()` | `RNNOptions&` (or `const RNNOptions&`) | [`options`](RNN/options.md) |

`reset_parameters()` re-fills every weight block in place with a fresh
uniform draw, using `options().seed` (offset per block); not covered on
its own page since it follows the same pattern
[`Fusion::reset_parameters`](../fusion/Fusion.md#methods) and every other
parameterized layer in this reference already documents.

## Changelog

- **0.1** — the Elman RNN, `input_size`/`hidden_size`/`num_layers`/
  `nonlinearity`/`bias`/`batch_first`/`dropout`/`bidirectional`/`split_bias`
  options.

## See Also

- [`RNNOptions`](RNNOptions.md) -- the options struct.
- [`GRU`](GRU.md) / [`LSTM`](LSTM.md) -- the gated alternatives, same
  shared shape conventions.
- [`PackedSequence`](PackedSequence.md) -- the variable-length-batch input
  form `forward` also accepts.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

RNN/Introduction
RNN/forward
RNN/options
RNN/params
RNNOptions
```
<!-- apiref:toctree:end -->
