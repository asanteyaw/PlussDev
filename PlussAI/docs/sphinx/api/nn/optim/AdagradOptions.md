# `pluss::ai::nn::optim::AdagradOptions`

The hyperparameters [`Adagrad`](Adagrad.md)'s own update formula reads:
learning rate, learning-rate decay, weight decay, epsilon, the
accumulator's initial value, and `maximize`, plus the two fields every
optimizer's `Options` ends with, [`clip`](GradientClipOptions.md) and
[`execution`](ExecutionOptions.md).

Passed to [`Adagrad`](Adagrad.md)'s constructor; every field has a
default.

## Fields

### `learning_rate`

`double learning_rate = 1e-2`
: The base step size, before the per-parameter accumulator scales it down
  and before `lr_decay` (if set) shrinks it further over time. Must be
  `> 0.0` and finite.

### `lr_decay`

`double lr_decay = 0.0`
: Shrinks the effective learning rate as training progresses,
  independent of the accumulator: the learning rate actually used at step
  `t` is `learning_rate / (1 + (t - 1) * lr_decay)`. `0.0` (the default)
  disables this entirely -- the accumulator alone governs how the
  effective rate changes. Must be `>= 0.0` and finite. See
  [`Adagrad`](Adagrad.md) Example 2.

### `weight_decay`

`double weight_decay = 0.0`
: L2 regularization strength, folded into the gradient before the
  accumulator update: `gradient += weight_decay * parameter`. `0.0` (the
  default) disables it. Must be `>= 0.0` and finite.

### `epsilon`

`double epsilon = 1e-10`
: A small constant added to the accumulator's square root before
  dividing, to avoid dividing by (near) zero for a parameter whose
  accumulator is still tiny (early in training, or a parameter that has
  received very small gradients so far). Must be `> 0.0` and finite.

### `initial_accumulator_value`

`double initial_accumulator_value = 0.0`
: The value every parameter's squared-gradient accumulator starts at,
  before any gradient has been seen. `0.0` (the default) is the
  conventional choice; a small positive value can be used to make the
  very first few updates slightly more conservative, at the cost of a
  less "pure" first step.

### `maximize`

`bool maximize = false`
: Negates the gradient before the update -- gradient ascent instead of
  descent. Same meaning as every other optimizer's `maximize` field.

### `clip`

[`GradientClipOptions`](GradientClipOptions.md) `clip = {}`
: Optional gradient clipping, applied before the accumulator update.

### `execution`

[`ExecutionOptions`](ExecutionOptions.md) `execution = {}`
: Which update lane runs, strictness, and capturable mode.

## Guidance

`AdagradOptions{}`'s defaults are a reasonable starting point whenever
per-parameter adaptive scaling is wanted and the accumulator's
never-shrinks-back behavior (see [`Adagrad`](Adagrad.md)'s own `See Also`)
is not expected to be a problem over the training run's length -- sparse
features, where most parameters see very few nonzero gradients over the
whole run, are the classic case Adagrad was designed for. For a long
training run where the effective learning rate decaying to near zero is a
real concern, reach for [`RMSprop`](RMSprop.md) or [`Adam`](Adam.md)
instead, both of which use a decayed running average in place of
Adagrad's ever-growing sum.

## See Also

- [`Adagrad`](Adagrad.md) -- the optimizer this configures.
- [`AdagradGroup`](AdagradGroup.md) -- per-group `AdagradOptions`
  overrides.
