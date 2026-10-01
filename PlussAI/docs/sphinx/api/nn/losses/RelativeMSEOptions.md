# `pluss::ai::nn::losses::RelativeMSEOptions`

Reduction mode, the divisor floor, and whether the divisor is taken by
absolute value.

Passed to [`RelativeMSE`](RelativeMSE.md)'s fourth argument, defaulted --
`RelativeMSE(prediction, target, divisor)` alone is equivalent to
`RelativeMSE(prediction, target, divisor, {})`.

## Fields

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element `(prediction - target)^2 / divisor` values are
  combined into the returned value. See
  [`Introduction`](Introduction.md#the-shared-reduction-mode) for the
  full three-way explanation shared by every loss in this family.

### `epsilon`

`double epsilon = 1e-12`
: The floor `divisor` (after the `use_absolute_divisor` step, if
  enabled) is clamped up to before dividing -- `denom =
  clamp_min(divisor_maybe_abs, epsilon)`. Must be positive and finite.
  This is a floor, not a threshold with special-case handling: a
  `divisor` element already below `epsilon` (including exactly `0`, or a
  negative value when `use_absolute_divisor == false`) is simply raised
  to `epsilon` before the division runs, producing a very large (not
  infinite or `NaN`) per-element result rather than a division error.
  See Example 3.

### `use_absolute_divisor`

`bool use_absolute_divisor = true`
: When `true` (the default), `|divisor|` is taken before the `epsilon`
  floor is applied -- appropriate when `divisor` represents a magnitude
  (a target's own scale, for instance) whose sign carries no meaning for
  this purpose. When `false`, the raw signed `divisor` is used instead --
  but since `clamp_min` only ever raises a value, a negative `divisor`
  still ends up floored to the small **positive** `epsilon`, not left
  negative. See Example 2 for exactly what that produces.

## Guidance

Leave `use_absolute_divisor` at its default `true` whenever `divisor`
represents a scale or magnitude (most uses -- typically the target's own
value, or some other always-meaningful-as-a-magnitude quantity). Only
set it `false` when `divisor`'s sign is known to never be negative in
practice, since a negative value passed through unabsorbed does not
produce a negative (or otherwise sign-preserving) result -- it produces
an artificially huge one, from the `epsilon` floor. Leave `epsilon` at
its default unless a specific numerical-stability reason calls for a
larger floor.

## See Also

- [`RelativeMSE`](RelativeMSE.md) -- the only consumer of this options
  struct.
- [`MSEOptions`](MSEOptions.md) -- the plain (unscaled) counterpart's
  options, sharing the same `reduction` field.
