# `pluss::ai::nn::losses::HuberOptions`

The quadratic/linear transition delta and the reduction mode.

Passed to [`Huber`](Huber.md)'s third argument, defaulted -- `Huber(prediction,
target)` alone is equivalent to `Huber(prediction, target, {})`.

## Fields

### `delta`

`double delta = 1.0`
: The absolute-error threshold where the loss switches from quadratic
  (`0.5 * error^2`, for `|error| <= delta`) to linear
  (`delta * |error| - 0.5 * delta^2`, for `|error| > delta`). Must be
  positive and finite. A larger `delta` widens the quadratic region --
  more errors get the (steeper-gradient) quadratic treatment before the
  loss flattens into its linear, outlier-resistant regime; a smaller
  `delta` narrows it, making the loss behave more like plain L1 (see
  [`L1`](L1.md)) sooner.

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element losses are combined into the returned value. See
  [`Introduction`](Introduction.md#the-shared-reduction-mode) for the
  full three-way explanation shared by every loss in this family.

## Guidance

`delta=1.0` (the default) is a reasonable starting point when prediction
and target are on a roughly unit scale; rescale `delta` (or the
data itself) when they are not, since the quadratic/linear transition is
an absolute threshold, not a relative one. Reach for a smaller `delta`
when outliers are common and should be down-weighted aggressively;
reach for a larger `delta` (or plain [`MSE`](MSE.md)) when the data is
believed to be genuinely outlier-free and the steeper quadratic gradient
throughout is preferred.

## See Also

- [`Huber`](Huber.md) -- the only consumer of this options struct.
- [`SmoothL1Options`](SmoothL1Options.md) -- a related but numerically
  different quadratic/linear loss, using `beta` in place of `delta`.
- [`MSEOptions`](MSEOptions.md) -- the purely quadratic counterpart,
  sharing the same `reduction` field.
