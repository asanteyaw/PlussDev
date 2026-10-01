# `pluss::ai::nn::losses::SmoothL1Options`

The quadratic/linear transition beta and the reduction mode.

Passed to [`SmoothL1`](SmoothL1.md)'s third argument, defaulted --
`SmoothL1(prediction, target)` alone is equivalent to `SmoothL1(prediction,
target, {})`.

## Fields

### `beta`

`double beta = 1.0`
: The absolute-error threshold where the loss switches from quadratic
  (`error^2 / (2 * beta)`, for `|error| < beta`) to linear (`|error| -
  0.5 * beta`, for `|error| >= beta`). Must be finite and non-negative.
  At exactly `beta = 0`, [`SmoothL1`](SmoothL1.md) delegates directly to
  [`L1`](L1.md) rather than evaluating this formula (which would divide
  by zero) -- see [`L1`](L1.md#example-3). Despite the superficial
  resemblance to [`HuberOptions::delta`](HuberOptions.md#delta), the two
  are **not** the same formula at matching threshold values --
  `SmoothL1`'s quadratic term is additionally scaled down by `1/beta`,
  `Huber`'s is not. See Example 2 for the numeric difference.

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element losses are combined into the returned value. See
  [`Introduction`](Introduction.md#the-shared-reduction-mode) for the
  full three-way explanation shared by every loss in this family.

## Guidance

Treat `beta` the same way as [`Huber::delta`](HuberOptions.md#guidance):
larger widens the quadratic region, smaller narrows it toward plain
`L1` (which `beta=0` reaches exactly). Whether `SmoothL1` or `Huber` is
the better fit for a given problem is not decided by picking matching
threshold values -- since the two formulas genuinely differ, compare
them directly on real data rather than assuming a shared `beta`/`delta`
value produces a shared loss.

## See Also

- [`SmoothL1`](SmoothL1.md) -- the only consumer of this options struct.
- [`HuberOptions`](HuberOptions.md) -- the related but numerically
  different quadratic/linear loss's options.
- [`L1Options`](L1Options.md) -- what this reduces to exactly at
  `beta = 0`.
