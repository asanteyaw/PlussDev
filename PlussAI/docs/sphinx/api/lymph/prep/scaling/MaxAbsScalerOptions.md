# `lymph::options::MaxAbsScaler`

`options::MaxAbsScaler` configures `prep::MaxAbsScaler` and the
[`prep::stage::maxabs_scaler`](../stage/maxabs_scaler.md) stage — the two share
this struct. `lymph::prep::MaxAbsScalerOptions` is the family-scoped alias for the
same type.

## Fields

### `floor`

`double floor = 1e-12` — lower bound on `max(|column|)`. An all-zero or near-zero
column has no magnitude to divide by; the floor keeps the division finite. This
is the only field.

## Guidance

Max-abs scaling divides by the largest absolute value and does not subtract
anything, so sign and zero are preserved and every fitted value lands in
`[-1, 1]`. Use it when the sign and the sparsity of a feature carry meaning. For
a bounded range with an arbitrary floor and ceiling use
[`options::MinMaxScaler`](MinMaxScalerOptions.md); for zero mean and unit
variance use [`options::StandardScaler`](StandardScalerOptions.md).

## See Also

- [`prep::stage::maxabs_scaler`](../stage/maxabs_scaler.md)
