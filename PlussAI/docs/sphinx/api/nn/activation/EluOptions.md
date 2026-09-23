# `pluss::ai::nn::EluOptions`

Controls the negative-side saturation floor.

Passed to [`Elu`](Elu.md)'s constructor; has one field with a default, so
`EluOptions{}` (or omitting the argument entirely) reproduces the standard
curve.

## Fields

### `alpha`

`double alpha = 1.0`
: The magnitude the negative side saturates toward:
  `x > 0 ? x : alpha * (exp(x) - 1)` approaches `-alpha` as `x` goes to
  negative infinity, without ever reaching or crossing it. Larger values
  allow a larger negative output range before saturating; smaller values
  pull the floor closer to `0`. Must be positive for the curve to behave as
  a saturating negative floor -- not validated, so passing `0` or a negative
  value produces a mathematically different (and not meaningfully useful)
  shape.

## Guidance

The default `1.0` is the conventional choice and matches the original ELU
paper. Lower it when you want the negative-side output range constrained
closer to `0` (useful if a downstream layer is sensitive to large negative
inputs); there is rarely a reason to raise it much above `1.0`.

## See Also

- [`Elu`](Elu.md) -- the only consumer of this options struct.
- [`elu`](../functional/elu.md) -- the stateless functional form, which
  takes `alpha` as a plain function argument instead of this struct.
