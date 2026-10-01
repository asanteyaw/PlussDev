# `pluss::ai::nn::rnn`

`rnn` owns the recurrent layers -- networks that process a sequence one
timestep at a time, carrying a hidden state (and, for
[`LSTM`](rnn/LSTM.md), a separate cell state) forward from each step to
the next. Unlike [`Fusion`](fusion/Fusion.md) or
[`cnn`](cnn.md), which see their whole input at once, a recurrent layer's
output at any timestep can depend on every timestep that came before it.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`RNN`](rnn/RNN.md) | the plain (Elman) recurrent layer -- one projection per timestep |
| [`GRU`](rnn/GRU.md) | a gated recurrent layer -- three projections per timestep, resistant to vanishing gradients |
| [`LSTM`](rnn/LSTM.md) | a gated layer with a separate cell state -- four projections per timestep, optional recurrent projection |
| [`PackedSequence`](rnn/PackedSequence.md) | a variable-length-batch representation all three layers accept directly |
| [`pack_padded_sequence`](rnn/pack_padded_sequence.md) / [`pad_packed_sequence`](rnn/pad_packed_sequence.md) | build and unpack the packed form |

## How To Call It

```cpp
namespace nn = pluss::ai::nn;

nn::GRU rnn(16, 32, /*num_layers=*/2);  // input_size=16, hidden_size=32
const auto [output, hidden] = rnn.forward(x);  // x: [seq_len, batch, 16]

nn::Schedule net;
net.append_as<pluss::Series, std::tuple<pluss::Series, pluss::Series>>("gru_stage", nn::GRU(16, 32));
```

All three layer types share the same shape conventions -- see
[`Introduction`](rnn/Introduction.md) for the full comparison. Because
`forward` returns a `tuple`, not a plain `Series`, adding one to a
[`Schedule`](module/Schedule.md) requires
[`Schedule::append_as`](module/Schedule/append_as.md) with the tuple type
stated explicitly -- plain `append` (which assumes `Series -> Series`)
does not compile for any of `RNN`/`GRU`/`LSTM`, not even as a less
convenient alternative. See
[`RNN::forward`'s tuple return](rnn/RNN/forward.md#returns) for exactly
what each layer's own return type is.

## Related Pages

- [`module`](module.md) -- the `Module` contract every type here
  implements, and `Schedule`, the container they compose into.
- [`fusion`](fusion.md) / [`cnn`](cnn.md) -- the non-recurrent layer
  families, for contrast.
- [`activation`](activation.md) -- `Sigmoid`/`Tanh` in particular are the
  same nonlinearities every gate in `GRU`/`LSTM` uses internally, though
  neither layer exposes them as separate stages.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

rnn/Introduction
rnn/RNN
rnn/GRU
rnn/LSTM
rnn/PackedSequence
rnn/pack_padded_sequence
rnn/pad_packed_sequence
```
<!-- apiref:toctree:end -->
