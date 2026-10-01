# `pluss::ai::nn::LSTM`

A multi-layer long short-term memory network: at each timestep, four
learned projections (input, forget, cell-candidate, output gates)
maintain a **separate cell state** alongside the hidden state, giving the
network an explicit, gated mechanism for preserving information across
long sequences. Optionally stacked, optionally bidirectional, and
optionally projects its hidden/output state down to a narrower width than
its cell state (`proj_size`). See [`Introduction`](Introduction.md) for
the exact gate formulas, and [`forward`](LSTM/forward.md) for every
input/output shape, including the projection case.

## Signature

```cpp
explicit nn::LSTM::LSTM(nn::LSTMOptions options);
nn::LSTM::LSTM(std::int64_t input_size, std::int64_t hidden_size, std::int64_t num_layers = 1,
               bool bias = true, bool batch_first = false, double dropout = 0.0,
               bool bidirectional = false, bool split_bias = false, std::int64_t proj_size = 0);

void nn::LSTM::reset_parameters();
```

## Construction

Two equivalent forms: pass an [`LSTMOptions`](LSTMOptions.md) directly, or
spell out the same fields as positional arguments:

```cpp
nn::LSTM a(nn::LSTMOptions{16, 32});  // input_size=16, hidden_size=32, everything else defaulted
nn::LSTM b(16, 32);                   // equivalent positional form
```

## State

`params()` (mutable and `const` overloads, reachable only via `->`) --
`vector<pluss::ai::rnn::NetWeight>&` / `const vector<...>&`
: One entry per `(layer, direction)` pair. Every entry's `weight_ih`/
  `weight_hh` are `[4 * hidden_size, ...]` -- four gates' worth of
  projection, in `(input, forget, cell, output)` order. `weight_hr`, when
  `options().proj_size > 0`, is `[proj_size, hidden_size]`. See
  [`params`](LSTM/params.md).

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the [`LSTMOptions`](LSTMOptions.md) this instance was constructed
with -- see [`options`](LSTM/options.md).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `forward(input, state = {})` / `forward(packed, state = {})` / `operator()(...)` | `tuple<Series, tuple<Series, Series>>` / `tuple<PackedSequence, tuple<Series, Series>>` | [`forward`](LSTM/forward.md) |
| `params()` | `vector<NetWeight>&` (or `const vector<NetWeight>&`) | [`params`](LSTM/params.md) |
| `options()` | `LSTMOptions&` (or `const LSTMOptions&`) | [`options`](LSTM/options.md) |

`reset_parameters()` re-fills every weight block (and `weight_hr`, when
projecting) in place with a fresh uniform draw, using `options().seed`;
not covered on its own page since it follows the same pattern every other
parameterized layer in this reference already documents.

## Changelog

- **0.1** — the long short-term memory network, `input_size`/`hidden_size`/
  `num_layers`/`bias`/`batch_first`/`dropout`/`bidirectional`/`split_bias`/
  `proj_size` options.

## See Also

- [`LSTMOptions`](LSTMOptions.md) -- the options struct.
- [`RNN`](RNN.md) -- the cheaper, ungated alternative.
- [`GRU`](GRU.md) -- the other gated alternative, without a separate cell
  state or projection.
- [`PackedSequence`](PackedSequence.md) -- the variable-length-batch input
  form `forward` also accepts.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

LSTM/Introduction
LSTM/forward
LSTM/options
LSTM/params
LSTMOptions
```
<!-- apiref:toctree:end -->
