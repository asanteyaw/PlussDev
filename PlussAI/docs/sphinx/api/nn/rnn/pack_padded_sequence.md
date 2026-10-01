# `pluss::ai::nn::pack_padded_sequence`

Turns an ordinary padded batch plus its per-sequence real lengths into a
[`PackedSequence`](PackedSequence.md) -- the form
[`RNN`](RNN.md)/[`GRU`](GRU.md)/[`LSTM`](LSTM.md) can run over without
ever computing across a padded (unused) timestep.

## Signature

```cpp
nn::PackedSequence nn::pack_padded_sequence(
    const Series& input, const Series& lengths, bool batch_first = false);
```

## What It Does

Sorts the batch by decreasing real length, then concatenates every
sequence's real (non-padded) timesteps into one flat `data` tensor --
discarding every padded position entirely rather than merely masking it.
`batch_sizes` records how many sequences are still active at each
original timestep position (used to slice `data` back apart one timestep
at a time), and `sorted_indices`/`unsorted_indices` record the sort
permutation and its inverse so the result can eventually be restored to
the caller's original (pre-sort) batch order. This is the step that lets
[`RNN`](RNN.md)/[`GRU`](GRU.md)/[`LSTM`](LSTM.md) skip computing over
padding altogether, rather than computing over it and discarding the
result.

## Parameters

`const Series& input`
: The padded batch. `[seq_len, batch, ...features]` when `batch_first ==
  false` (the default), or `[batch, seq_len, ...features]` when `true`.
  Whatever padding value fills the unused positions is never read --
  `lengths` alone decides which positions are real.

`const Series& lengths`
: `[batch]`, integer-valued, sequence `i`'s real (unpadded) length. Every
  entry must be in `(0, seq_len]`.

`bool batch_first`
: Which axis of `input` is the sequence-length axis -- must match how
  `input` is actually laid out, the same convention
  [`RNNOptions::batch_first`](RNNOptions.md) (and its `GRU`/`LSTM`
  counterparts) uses.

## Returns

A [`PackedSequence`](PackedSequence.md) -- `data` holds every real
timestep from every sequence, concatenated in decreasing-length order (see
Example 1); `batch_sizes` records how many sequences are still active at
each original timestep position; `sorted_indices`/`unsorted_indices`
record the length-based permutation and its inverse, so
[`pad_packed_sequence`](pad_packed_sequence.md) can restore the original
batch order later.

## Errors

- `input` has rank less than 2, or `lengths` has rank less than 1 --
  throws.

## Practical Examples

Three examples: a batch of three different-length sequences with the
resulting packed structure inspected field by field, the same batch laid
out `batch_first` instead, and the degenerate case where every sequence
already has the same length.

### Example 1

Three sequences of lengths `{2, 4, 3}` -- `batch_sizes` records how many
are still active at each of the four original timestep positions, and
`sorted_indices` records the permutation that reordered them by
decreasing length before packing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // 3 sequences, max_seq_len=4, feature_size=1, time-major (batch_first=false)
  // seq 0: length 2, seq 1: length 4, seq 2: length 3
  const pluss::Series padded = pluss::make_series(
      {10.0, 20.0, 30.0,   // t=0
       11.0, 21.0, 31.0,   // t=1
       0.0,  22.0, 32.0,   // t=2 (seq 0 already ended)
       0.0,  23.0, 0.0},   // t=3 (only seq 1 still going)
      {4, 3, 1}, f64);
  const pluss::Series lengths = pluss::make_series({2.0, 4.0, 3.0}, {3}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded, lengths, /*batch_first=*/false);

  std::cout << "sorted_indices (by decreasing length): " << packed.sorted_indices << "\n";
  std::cout << "batch_sizes (active sequences per original timestep): " << packed.batch_sizes << "\n";
  std::cout << "data numel: " << packed.data.numel() << " -- 2+4+3=9 real timesteps, not 3*4=12\n";
  // first batch_sizes[0]=3 rows are every sequence's own t=0, in sorted order [1,2,0]
  std::cout << "data[0:3] (every sequence's t=0, sorted order [seq1,seq2,seq0]): "
            << packed.data.reshape({9}).slice(0, 0, 3) << "\n";

  return 0;
}
```

Output

```text
sorted_indices (by decreasing length): pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 0]
batch_sizes (active sequences per original timestep): pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[3, 3, 2, 1]
data numel: 9 -- 2+4+3=9 real timesteps, not 3*4=12
data[0:3] (every sequence's t=0, sorted order [seq1,seq2,seq0]): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[20, 30, 10]
```

### Example 2

The exact same three sequences, laid out `batch_first` instead of
time-major -- `batch_sizes` and `sorted_indices` come out identical;
only `input`'s (and thus the intermediate layout) differs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // same data as the time-major example, but laid out batch_first: [batch=3, seq_len=4, feature=1]
  // seq 0: [10,11,0,0] (length 2), seq 1: [20,21,22,23] (length 4), seq 2: [30,31,32,0] (length 3)
  const pluss::Series padded = pluss::make_series(
      {10.0, 11.0, 0.0, 0.0,
       20.0, 21.0, 22.0, 23.0,
       30.0, 31.0, 32.0, 0.0},
      {3, 4, 1}, f64);
  const pluss::Series lengths = pluss::make_series({2.0, 4.0, 3.0}, {3}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded, lengths, /*batch_first=*/true);

  std::cout << "sorted_indices: " << packed.sorted_indices << "\n";
  std::cout << "batch_sizes: " << packed.batch_sizes << "\n";
  std::cout << "same batch_sizes/sorted_indices as the time-major layout -- only data's own layout differs\n";

  return 0;
}
```

Output

```text
sorted_indices: pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 0]
batch_sizes: pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[3, 3, 2, 1]
same batch_sizes/sorted_indices as the time-major layout -- only data's own layout differs
```

### Example 3

When every sequence already has the same length, no reordering happens at
all -- `sorted_indices` is the identity permutation, `batch_sizes` is
constant, and `data`'s size exactly equals the padded input's own size
(there was never any padding to remove).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // every sequence the same length -- no padding needed at all
  const pluss::Series padded = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3, 1}, f64);  // seq_len=2, batch=3
  const pluss::Series lengths = pluss::make_series({2.0, 2.0, 2.0}, {3}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded, lengths, /*batch_first=*/false);

  std::cout << "sorted_indices (all lengths equal, order preserved): " << packed.sorted_indices << "\n";
  std::cout << "batch_sizes (constant -- every timestep has every sequence active): " << packed.batch_sizes << "\n";
  std::cout << "data numel: " << packed.data.numel() << " -- 2*3=6, identical to the padded input's own size\n";

  return 0;
}
```

Output

```text
sorted_indices (all lengths equal, order preserved): pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
batch_sizes (constant -- every timestep has every sequence active): pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, 3]
data numel: 6 -- 2*3=6, identical to the padded input's own size
```

## See Also

- [`PackedSequence`](PackedSequence.md) -- the type this function produces.
- [`pad_packed_sequence`](pad_packed_sequence.md) -- the inverse operation.
- [`RNN::forward`](RNN/forward.md) -- accepts a `PackedSequence` directly.
