# `pluss::ai::nn::losses::L1Options`

The reduction mode.

Passed to [`L1`](L1.md)'s third argument, defaulted -- `L1(prediction,
target)` alone is equivalent to `L1(prediction, target, {})`.

## Fields

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element absolute errors are combined into the returned
  value. See [`Introduction`](Introduction.md#the-shared-reduction-mode)
  for the full three-way explanation shared by every loss in this
  family.

## Guidance

Identical guidance to [`MSEOptions::reduction`](MSEOptions.md#guidance):
`Mean` is the right default for training; `Sum`/`None` are for the less
common cases where batch-size-independence or the per-element values
themselves matter more than a single scalar.

## See Also

- [`L1`](L1.md) -- the only consumer of this options struct.
- [`SmoothL1Options`](SmoothL1Options.md) -- `SmoothL1` with `beta=0`
  reduces to exactly this loss.
