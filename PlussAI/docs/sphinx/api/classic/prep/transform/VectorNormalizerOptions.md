# `pluss::ai::classic::prep::VectorNormalizerOptions`

The norm choice and degenerate-row floor consumed by
[`VectorNormalizer::map`](VectorNormalizer.md).

## Fields

### `norm`

`VectorNorm norm = VectorNorm::L2`

Which norm to divide each row by:

- `L1` — sum of absolute values. Makes every row's features sum (in
  absolute value) to `1`; common for frequency/count-style rows.
- `L2` (the default) — Euclidean norm. Preserves the row's direction on the
  unit hypersphere; the usual choice for embedding-style vectors.
- `Linf` — maximum absolute value. Scales the row so its single largest
  entry becomes `±1`.

### `floor`

`double floor = 1e-12`

The minimum row norm `map` will actually divide by. A row whose norm does
not reach this (an all-zero, or entirely missing, row) is left unchanged
instead. Must be a positive, finite number.

## Guidance

`L2` is the right default for most uses. Reach for `L1` when the row's
values are counts or frequencies that should sum to a fixed total; reach for
`Linf` when only the single largest feature's scale should anchor the row.

## See Also

- [`VectorNormalizer`](VectorNormalizer.md) — the consuming type.
