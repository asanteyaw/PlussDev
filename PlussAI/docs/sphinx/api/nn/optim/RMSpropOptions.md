# `pluss::ai::nn::optim::RMSpropOptions`

The hyperparameters [`RMSprop`](RMSprop.md)'s own update formula reads:
learning rate, the smoothing rate `alpha`, epsilon, momentum, weight
decay, centering, and `maximize`, plus
[`clip`](GradientClipOptions.md)/[`execution`](ExecutionOptions.md).

Passed to [`RMSprop`](RMSprop.md)'s constructor.

## Fields

### `learning_rate`

`double learning_rate = 1e-2`
: The base step size, before the running-average scaling. Must be `>
  0.0` and finite.

### `alpha`

`double alpha = 0.99`
: The decay rate for the squared-gradient running average
  (`square_avg = alpha * square_avg + (1 - alpha) * gradient^2`), and for
  the running gradient average when `centered = true`. Closer to `1.0`
  means a longer effective averaging window. Must be in `[0, 1)`.

### `epsilon`

`double epsilon = 1e-8`
: A small constant added to the running average's square root before
  dividing, to avoid dividing by (near) zero early in training. Must be
  `> 0.0` and finite.

### `momentum`

`double momentum = 0.0`
: Applies plain momentum on top of the already-scaled update
  (`momentum_buffer = momentum * momentum_buffer + scaled_update`). `0.0`
  (the default) disables it. Distinct from `alpha`, which governs the
  scaling itself rather than smoothing across updates.

### `weight_decay`

`double weight_decay = 0.0`
: L2 regularization strength, folded into the gradient before the
  running-average update -- coupled decay, the same mechanism
  [`SGDOptions::weight_decay`](SGDOptions.md) uses. `0.0` (the default)
  disables it.

### `centered`

`bool centered = false`
: When `true`, scales the update by the gradient's estimated *variance*
  (the squared-gradient average minus the squared running-gradient
  average) instead of its raw second moment -- a more accurate measure of
  how much the gradient actually varies, at the cost of tracking a second
  running average per parameter. `false` (the default) is plain RMSprop.
  See [`RMSprop`](RMSprop.md) Example 2.

### `maximize`

`bool maximize = false`
: Negates the gradient before the update -- gradient ascent instead of
  descent.

### `clip`

[`GradientClipOptions`](GradientClipOptions.md) `clip = {}`
: Optional gradient clipping, applied before the running-average update.

### `execution`

[`ExecutionOptions`](ExecutionOptions.md) `execution = {}`
: Which update lane runs, strictness, and capturable mode.

## Guidance

`RMSpropOptions{}`'s defaults are a reasonable starting point whenever
adaptive per-parameter scaling is wanted without Adagrad's
ever-shrinking-only behavior. Add `momentum` (values around `0.9`, the
same conventional range [`SGDOptions::momentum`](SGDOptions.md) uses) if
plain RMSprop is converging too slowly or oscillating; reach for
`centered = true` when the gradient's sign flips often enough that its
raw second moment overstates how much it actually varies -- the extra
per-parameter state it costs is rarely significant compared to the
optimizer's other buffers.

## See Also

- [`RMSprop`](RMSprop.md) -- the optimizer this configures.
- [`RMSpropGroup`](RMSpropGroup.md) -- per-group `RMSpropOptions`
  overrides.
