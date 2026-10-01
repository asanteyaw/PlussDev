# `pluss::ai::nn::GRU`

A multi-layer gated recurrent unit network: at each timestep, three
learned projections (reset, update, candidate) combine that timestep's
input with the previous hidden state to decide how much of the previous
state to keep versus overwrite. Optionally stacked and optionally
bidirectional, with the same shape conventions
[`RNN`](RNN.md)/[`LSTM`](LSTM.md) use. See
[`Introduction`](Introduction.md) for the exact gate formulas, and
[`forward`](GRU/forward.md) for every input/output shape.

## Signature

```cpp
explicit nn::GRU::GRU(nn::GRUOptions options);
nn::GRU::GRU(std::int64_t input_size, std::int64_t hidden_size, std::int64_t num_layers = 1,
             bool bias = true, bool batch_first = false, double dropout = 0.0,
             bool bidirectional = false, bool split_bias = false);

void nn::GRU::reset_parameters();
```

## Construction

Two equivalent forms: pass a [`GRUOptions`](GRUOptions.md) directly, or
spell out the same fields as positional arguments:

```cpp
nn::GRU a(nn::GRUOptions{16, 32});  // input_size=16, hidden_size=32, everything else defaulted
nn::GRU b(16, 32);                  // equivalent positional form
```

## State

`params()` (mutable and `const` overloads, reachable only via `->`) --
`vector<pluss::ai::rnn::NetWeight>&` / `const vector<...>&`
: One entry per `(layer, direction)` pair. Every entry's `weight_ih`/
  `weight_hh` are `[3 * hidden_size, ...]` -- three gates' worth of
  projection stacked along the first axis, in `(reset, update,
  candidate)` order. See [`params`](GRU/params.md).

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the [`GRUOptions`](GRUOptions.md) this instance was constructed
with -- see [`options`](GRU/options.md).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `forward(input, hx = {})` / `forward(packed, hx = {})` / `operator()(...)` | `tuple<Series, Series>` / `tuple<PackedSequence, Series>` | [`forward`](GRU/forward.md) |
| `params()` | `vector<NetWeight>&` (or `const vector<NetWeight>&`) | [`params`](GRU/params.md) |
| `options()` | `GRUOptions&` (or `const GRUOptions&`) | [`options`](GRU/options.md) |

`reset_parameters()` re-fills every weight block in place with a fresh
uniform draw, using `options().seed`; not covered on its own page since it
follows the same pattern every other parameterized layer in this
reference already documents.

## Changelog

- **0.1** — the gated recurrent unit, `input_size`/`hidden_size`/
  `num_layers`/`bias`/`batch_first`/`dropout`/`bidirectional`/`split_bias`
  options.

## See Also

- [`GRUOptions`](GRUOptions.md) -- the options struct.
- [`RNN`](RNN.md) -- the cheaper, ungated alternative.
- [`LSTM`](LSTM.md) -- the other gated alternative, with a separate cell
  state.
- [`PackedSequence`](PackedSequence.md) -- the variable-length-batch input
  form `forward` also accepts.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

GRU/Introduction
GRU/forward
GRU/options
GRU/params
GRUOptions
```
<!-- apiref:toctree:end -->
