# `pluss::ai::classic::prep::MaxAbsScalerOptions`

The single guard field consumed by [`MaxAbsScaler::learn`](MaxAbsScaler.md).

## Fields

### `floor`

`double floor = 1e-12`

The minimum fitted `max(|value|)` `learn` will actually divide by. A feature
whose largest magnitude does not reach `floor` — including an all-zero or
entirely-missing feature — gets a fitted scale of `1` instead, leaving its
(already near-zero) values unchanged. Must be a positive, finite number;
`learn` rejects any other value with `Status::invalid_argument` before
reading data.

## Guidance

`MaxAbsScalerOptions` has one field for a reason: there is nothing else to
configure — no centering toggle (this scaler never centers), no output
interval (dividing by the max absolute value always lands in `[-1, 1]`). Leave
`floor` at its default unless your own numerical tolerance for "close enough
to zero to be degenerate" differs from `1e-12`.

## See Also

- [`MaxAbsScaler`](MaxAbsScaler.md) — the consuming type.
- [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback) — the
  fallback mechanism compared across all four scalers.
