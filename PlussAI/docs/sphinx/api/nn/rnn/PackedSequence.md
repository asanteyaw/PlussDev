# `pluss::ai::nn::PackedSequence`

A variable-length batch, packed so that no padded (unused) timestep is
ever stored or computed over. Produced by
[`pack_padded_sequence`](pack_padded_sequence.md), consumed directly by
[`RNN`](RNN.md)/[`GRU`](GRU.md)/[`LSTM`](LSTM.md)'s `PackedSequence`
`forward` overload, and reversed back into an ordinary padded batch by
[`pad_packed_sequence`](pad_packed_sequence.md).

## Signature

```cpp
Series data;
Series batch_sizes;
Series sorted_indices;
Series unsorted_indices;
```

A plain aggregate (four public fields, listed above; see
[`## State`](#state) for what each one holds) -- there is no constructor
beyond the implicit default/aggregate ones. You do not build a
`PackedSequence` by hand; [`pack_padded_sequence`](pack_padded_sequence.md)
is the only supported way to produce a correctly-formed one.

## State

`data` -- `Series`
: Every real (non-padded) timestep from every sequence in the batch,
  concatenated along the first axis in decreasing-length order -- the
  longest sequence's timesteps first. Rank matches the original padded
  batch minus one axis (the sequence-length axis is gone; what remains is
  a flat list of per-timestep feature vectors).

`batch_sizes` -- `Series`
: `Int64`, one entry per original timestep position, `batch_sizes[t]` is
  how many sequences are still active (have not yet run out of real
  timesteps) at position `t`. Strictly non-increasing.

`sorted_indices` -- `Series`
: `Int64`, the permutation that sorts the original batch by decreasing
  length -- `data` is built by applying this permutation first.

`unsorted_indices` -- `Series`
: `Int64`, the inverse of `sorted_indices` -- what
  [`pad_packed_sequence`](pad_packed_sequence.md) applies at the end to
  restore the original (pre-sort) batch order.

## Methods

None -- every operation on a `PackedSequence` is a free function
([`pack_padded_sequence`](pack_padded_sequence.md),
[`pad_packed_sequence`](pad_packed_sequence.md)) or a `forward` overload
on [`RNN`](RNN.md)/[`GRU`](GRU.md)/[`LSTM`](LSTM.md), never a member of
this type itself.

## Changelog

- **0.1** — the packed-batch representation: `data`/`batch_sizes`/
  `sorted_indices`/`unsorted_indices`.

## See Also

- [`pack_padded_sequence`](pack_padded_sequence.md) -- the only supported
  way to build one.
- [`pad_packed_sequence`](pad_packed_sequence.md) -- unpacks one back into
  an ordinary padded batch.
- [`RNN`](RNN.md) / [`GRU`](GRU.md) / [`LSTM`](LSTM.md) -- each has a
  `forward` overload that accepts one directly.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
