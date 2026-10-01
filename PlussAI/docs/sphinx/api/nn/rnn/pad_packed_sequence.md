# `pluss::ai::nn::pad_packed_sequence`

The inverse of [`pack_padded_sequence`](pack_padded_sequence.md): turns a
[`PackedSequence`](PackedSequence.md) back into an ordinary padded batch,
plus each sequence's real length, restored to the original (pre-sort)
batch order.

## Signature

```cpp
std::tuple<Series, Series> nn::pad_packed_sequence(
    const nn::PackedSequence& packed, bool batch_first = false);
```

## What It Does

Reconstructs an ordinary padded batch from a `PackedSequence`'s flat
`data`: it slices `data` back apart one original timestep position at a
time using `batch_sizes`, zero-fills every position beyond each
sequence's own real length, then applies `unsorted_indices` to undo the
decreasing-length sort [`pack_padded_sequence`](pack_padded_sequence.md)
performed -- restoring the caller's original batch order. It also returns
each sequence's real length in that same restored order, so the pair is
immediately usable wherever an ordinary `(padded_input, lengths)` pair is
expected.

## Parameters

`const PackedSequence& packed`
: Ordinarily the output of [`pack_padded_sequence`](pack_padded_sequence.md),
  or the packed output of an [`RNN`](RNN.md)/[`GRU`](GRU.md)/[`LSTM`](LSTM.md)
  `forward` call run on one.

`bool batch_first`
: Which axis the returned padded batch's sequence-length axis should be --
  `[seq_len, batch, ...]` when `false` (the default), `[batch, seq_len,
  ...]` when `true`. Independent of whatever layout `packed` was
  originally built from; pass whichever layout your own code expects
  next.

## Returns

A `std::tuple<Series, Series>`:

- The padded batch, restored to the original (pre-sort) order. Every
  position beyond a sequence's own real length is filled with `0` --
  **not** whatever value the original padding happened to use before
  packing; see Example 2.
- Each sequence's real length, `[batch]`, in the same restored order.

## Errors

- `packed.data` has rank less than 1 -- throws.

## Practical Examples

Three examples: a full pack/pad round trip reproducing the original
batch exactly, confirming padded positions come back as `0` regardless of
what the original padding value was, and the same round trip laid out
`batch_first`.

### Example 1

Packing and immediately unpacking a batch reproduces the exact original
padded input, including its already-zero padded positions.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series padded = pluss::make_series(
      {10.0, 20.0, 30.0,
       11.0, 21.0, 31.0,
       0.0,  22.0, 32.0,
       0.0,  23.0, 0.0},
      {4, 3, 1}, f64);
  const pluss::Series lengths = pluss::make_series({2.0, 4.0, 3.0}, {3}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded, lengths, /*batch_first=*/false);
  const auto [restored, restored_lengths] = nn::pad_packed_sequence(packed, /*batch_first=*/false);

  std::cout << "restored shape: [" << restored.sizes()[0] << "," << restored.sizes()[1] << "," << restored.sizes()[2] << "]\n";
  std::cout << "restored lengths (back in original order): " << restored_lengths << "\n";
  std::cout << "restored values equal the original padded input: " << std::boolalpha
            << restored.allclose(padded) << "\n";

  return 0;
}
```

Output

```text
restored shape: [4,3,1]
restored lengths (back in original order): pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[2, 4, 3]
restored values equal the original padded input: true
```

### Example 2

When the original padding uses a non-zero filler, the round trip does
**not** preserve it -- `pad_packed_sequence` always fills unused positions
with `0`, because the packed form never stored the original filler value
at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // padded positions filled with 999 instead of 0 -- pack/pad never reads them
  const pluss::Series padded = pluss::make_series(
      {10.0, 20.0, 30.0,
       11.0, 21.0, 31.0,
       999.0, 22.0, 32.0,
       999.0, 23.0, 999.0},
      {4, 3, 1}, f64);
  const pluss::Series lengths = pluss::make_series({2.0, 4.0, 3.0}, {3}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded, lengths, /*batch_first=*/false);
  const auto [restored, restored_lengths] = nn::pad_packed_sequence(packed, /*batch_first=*/false);

  std::cout << "restored: " << restored << "\n";
  std::cout << "restored padded positions are 0, not the original 999 filler\n";

  return 0;
}
```

Output

```text
restored: pluss::Series(shape=[4,3,1], strides=[3,1,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[[10],
  [20],
  [30]],
  [[11],
  [21],
  [31]],
  [[0],
  [22],
  [32]],
  [[0],
  [23],
  [0]]]
restored padded positions are 0, not the original 999 filler
```

### Example 3

The same round trip laid out `batch_first` instead of time-major.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series padded = pluss::make_series(
      {10.0, 11.0, 0.0, 0.0,
       20.0, 21.0, 22.0, 23.0,
       30.0, 31.0, 32.0, 0.0},
      {3, 4, 1}, f64);
  const pluss::Series lengths = pluss::make_series({2.0, 4.0, 3.0}, {3}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded, lengths, /*batch_first=*/true);
  const auto [restored, restored_lengths] = nn::pad_packed_sequence(packed, /*batch_first=*/true);

  std::cout << "restored shape: [" << restored.sizes()[0] << "," << restored.sizes()[1] << "," << restored.sizes()[2] << "]\n";
  std::cout << "restored values equal the original batch_first input: " << std::boolalpha
            << restored.allclose(padded) << "\n";
  std::cout << "restored lengths: " << restored_lengths << "\n";

  return 0;
}
```

Output

```text
restored shape: [3,4,1]
restored values equal the original batch_first input: true
restored lengths: pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[2, 4, 3]
```

## See Also

- [`pack_padded_sequence`](pack_padded_sequence.md) -- the inverse
  operation.
- [`PackedSequence`](PackedSequence.md) -- the type this function
  consumes.
- [`RNN::forward`](RNN/forward.md) -- returns a `PackedSequence` directly
  when called with one; this function is how you get an ordinary padded
  batch back out of that result.
