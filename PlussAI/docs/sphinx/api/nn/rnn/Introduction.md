# `nn::rnn` Introduction

## What This Category Owns

`rnn` owns the recurrent layers -- networks that carry state forward
across a sequence's timesteps, rather than seeing the whole input at
once:

- [`RNN`](RNN.md) -- one learned projection per timestep, the cheapest
  and least capable of the three at preserving information over long
  sequences.
- [`GRU`](GRU.md) -- three learned (gated) projections per timestep,
  substantially more resistant to vanishing gradients than plain `RNN`.
- [`LSTM`](LSTM.md) -- four learned (gated) projections per timestep,
  with an explicit cell state separate from the hidden/output state, and
  an optional recurrent projection (`proj_size`).

All three share the same shape conventions (see each one's own
Introduction for the exact per-timestep formula) and the same
construction pattern (an `Options` struct, or the equivalent positional
arguments).

## Same Shape Conventions Throughout

- Input: `[seq_len, batch, input_size]` (time-major, the default), or
  `[batch, seq_len, input_size]` when `batch_first == true`.
- Output: same layout convention as the input, last axis `hidden_size *
  dirs` wide (`dirs` is `2` if `bidirectional` else `1`) -- or
  `proj_size * dirs` wide for `LSTM` when projecting.
- Recurrent state: `[num_layers * dirs, batch, hidden_size]` (two states,
  `(hidden, cell)`, for `LSTM`) -- the same shape the optional second
  `forward` argument takes and the second return value produces, so a
  sequence can be continued across separate `forward` calls by feeding
  one call's returned state into the next.

## Why Recurrence At All

[`Fusion`](../fusion/Fusion.md) and [`cnn`](../cnn.md)'s convolutions both
see a bounded window of their input at once -- a `Fusion` sees the whole
feature vector, a convolution sees one kernel-sized window. Neither has
any notion of "what came before" beyond what is explicitly in that window.
A recurrent layer's hidden state, by contrast, is a summary of
*everything* the layer has seen in the sequence so far, updated one
timestep at a time -- the natural fit whenever a prediction genuinely
depends on unbounded history (language, time series, anything where order
and long-range dependency matter).

## `PackedSequence`: Variable-Length Batches Without Wasted Computation

A batch of sequences rarely all have the same real length. The
conventional fix -- pad every sequence to the batch's longest length --
wastes computation on padded positions unless the layer is told to skip
them. [`PackedSequence`](PackedSequence.md) (built by
[`pack_padded_sequence`](pack_padded_sequence.md), consumed directly by
all three layers' `forward`, and reversed by
[`pad_packed_sequence`](pad_packed_sequence.md)) is exactly that: a
representation holding only the real timesteps, so a shorter sequence in
the batch simply stops contributing computation once its own length is
reached, rather than continuing to run over zeros.

## Choosing Among The Three

1. Default to [`GRU`](GRU.md) for a new model -- it is the best
   cost/capability balance of the three for most sequence lengths.
2. Reach for [`LSTM`](LSTM.md) specifically when a separate cell state
   (or a cheaper recurrent connection via `proj_size`) genuinely helps;
   otherwise it costs one more gate's worth of computation than `GRU` for
   no benefit.
3. Reach for [`RNN`](RNN.md) only when the sequences are short enough
   that vanishing gradients are not a practical concern, or when
   deliberately reproducing a plain-RNN baseline.
4. Reach for [`PackedSequence`](PackedSequence.md) whenever a batch's
   sequences have genuinely different real lengths -- padding alone
   still works, but wastes computation and can (depending on the padding
   value) subtly influence a bidirectional layer's backward-direction
   pass over padded positions.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract every type here implements, and how layers compose into a
  `Schedule` (via `append_as`, since none of these layers return a plain
  `Series`).
- [`fusion`](../fusion.md) / [`cnn`](../cnn.md) -- the non-recurrent layer
  families.
