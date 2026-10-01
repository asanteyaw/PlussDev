# `pluss::ai::nn::optim::SGDOptions`

The hyperparameters [`SGD`](SGD.md)'s own update formula reads: learning
rate, momentum, dampening, weight decay, Nesterov, and `maximize`, plus the
two fields every optimizer's `Options` ends with,
[`clip`](GradientClipOptions.md) and
[`execution`](ExecutionOptions.md).

Passed to [`SGD`](SGD.md)'s constructor; every field has a default, so
`SGDOptions{}` reproduces plain gradient descent with no momentum, decay,
or clipping.

## Fields

### `learning_rate`

`double learning_rate = 1e-2`
: The step size: each update moves a parameter by `learning_rate *
  update` (where `update` already folds in momentum/Nesterov if enabled).
  Must be `> 0.0` and finite -- construction throws otherwise. Too large
  and training diverges; too small and training crawls -- there is no
  universal default, `1e-2` is simply a conventional starting point.

### `momentum`

`double momentum = 0.0`
: A fraction in `[0, ~1)` of the previous update carried forward into the
  current one (`velocity = momentum * velocity + (1 - dampening) *
  gradient`). `0.0` (the default) disables momentum entirely -- plain
  gradient descent. Values around `0.9` are the conventional choice when
  momentum is wanted at all; it accelerates descent along consistent
  directions and damps oscillation across steep, narrow valleys in the
  loss surface. Must be `>= 0.0` and finite.

### `dampening`

`double dampening = 0.0`
: Scales how much of the *current* gradient enters the velocity buffer
  (`(1 - dampening) * gradient`), independent of `momentum`'s own decay of
  the *previous* velocity. `0.0` (the default) is the conventional
  choice -- the current gradient contributes at full weight. Only
  meaningful when `momentum > 0`; has no effect otherwise. Must be `>=
  0.0` and finite.

### `weight_decay`

`double weight_decay = 0.0`
: L2 regularization strength, added directly into the gradient before the
  momentum/update step: `gradient += weight_decay * parameter`. `0.0`
  (the default) disables it. Must be `>= 0.0` and finite. See
  [`AdamW`](AdamW.md) for the *decoupled* alternative, which applies decay
  to the parameter directly rather than folding it into the gradient.

### `nesterov`

`bool nesterov = false`
: When `true`, applies Nesterov's lookahead correction: the velocity
  update uses the gradient evaluated as if the momentum step had already
  been taken, rather than the plain current gradient. Requires `momentum >
  0` -- construction throws if `nesterov = true` with `momentum <= 0`. See
  [`SGD`](SGD.md) Example 2 for its effect against classic momentum.

### `maximize`

`bool maximize = false`
: When `true`, negates the gradient before the update, turning `step`
  into gradient *ascent* -- useful for objectives phrased as "maximize
  this score" rather than "minimize this loss," without the caller having
  to negate the objective by hand. `false` (the default) is ordinary
  minimization.

### `clip`

[`GradientClipOptions`](GradientClipOptions.md) `clip = {}`
: Optional gradient clipping, applied before the momentum/update
  computation. Disabled (`mode = None`) by default.

### `execution`

[`ExecutionOptions`](ExecutionOptions.md) `execution = {}`
: Which update lane runs, strictness, and capturable mode. `Auto`/`false`/
  `false` by default.

## Guidance

Start from `SGDOptions{}` (a bare `learning_rate`) for the simplest,
most predictable baseline; add `momentum = 0.9` once plain gradient
descent is converging too slowly or oscillating across a narrow valley;
add `nesterov = true` on top of that once momentum alone is helping but
overshooting. Reach for [`Adam`](Adam.md)/[`AdamW`](AdamW.md) instead of
tuning `SGD`'s momentum further when different parameters clearly need
different effective learning rates -- that per-parameter adaptivity is
exactly what `SGD` does not have.

## See Also

- [`SGD`](SGD.md) -- the optimizer this configures.
- [`SGDGroup`](SGDGroup.md) -- per-group `SGDOptions` overrides.
