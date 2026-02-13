# Broadcasting

PlussQit follows Torch-style broadcasting rules for elementwise ops.

## Shape alignment

Given shapes `A` and `B`, align from the trailing dimension:

- Missing leading dimensions are treated as size `1`
- For each aligned dimension:
  - if sizes equal -> output size is that size
  - else if one is `1` -> output size is the other
  - else -> error (not broadcastable)

## Strides

Broadcasted dimensions behave as **stride 0** (repeating the same element).

PlussQit implements broadcasting through `SeriesIterator`, which:
- computes a broadcasted logical shape
- expands each input's strides to that shape (using 0 stride for broadcast)
- coalesces adjacent dimensions when safe

## Current status

- `where`, arithmetic, comparisons, and most elementwise ops broadcast.
- Some reductions and cumulative ops currently require contiguous input and do not broadcast (by design).
