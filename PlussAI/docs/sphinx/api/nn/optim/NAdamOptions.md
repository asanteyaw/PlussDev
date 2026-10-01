# `pluss::ai::nn::optim::NAdamOptions`

The hyperparameters [`NAdam`](NAdam.md)'s own update formula reads:
learning rate, the two moment-decay rates, epsilon, weight decay, a
momentum-schedule decay rate, a coupled-vs-decoupled weight decay toggle,
and `maximize`, plus [`clip`](GradientClipOptions.md)/
[`execution`](ExecutionOptions.md).

Passed to [`NAdam`](NAdam.md)'s constructor.

## Fields

### `learning_rate`

`double learning_rate = 2e-3`
: The base step size. Note the default differs from
  [`AdamOptions::learning_rate`](AdamOptions.md) (`1e-3`) -- `NAdam`'s
  own momentum schedule is tuned assuming this larger default. Must be
  `> 0.0` and finite.

### `beta1`

`double beta1 = 0.9`
: Same role as [`AdamOptions::beta1`](AdamOptions.md) -- the first-moment
  decay rate the Nesterov correction is layered on top of.

### `beta2`

`double beta2 = 0.999`
: Same role as [`AdamOptions::beta2`](AdamOptions.md).

### `epsilon`

`double epsilon = 1e-8`
: Same role as [`AdamOptions::epsilon`](AdamOptions.md).

### `weight_decay`

`double weight_decay = 0.0`
: Weight decay strength -- applied as coupled or decoupled decay
  depending on `decoupled_weight_decay` below. `0.0` (the default)
  disables it regardless of that toggle.

### `momentum_decay`

`double momentum_decay = 4e-3`
: Governs how the effective momentum coefficient grows across steps
  (`mu_t = beta1 * (1 - 0.5 * 0.96^(t * momentum_decay))`), separately
  from `beta1` itself. Larger values ramp the momentum coefficient toward
  its asymptote faster. The conventional default is rarely worth
  changing.

### `decoupled_weight_decay`

`bool decoupled_weight_decay = false`
: Selects which of the two weight-decay mechanisms `weight_decay` uses:
  `false` (the default) folds it into the gradient, the same coupled
  mechanism [`AdamOptions::weight_decay`](AdamOptions.md) uses; `true`
  applies it as a direct multiplicative shrink on the parameter, the same
  decoupled mechanism [`AdamWOptions::weight_decay`](AdamWOptions.md)
  always uses. See [`NAdam`](NAdam.md) Example 2 for the isolated
  comparison.

### `maximize`

`bool maximize = false`
: Negates the gradient before the update -- gradient ascent instead of
  descent.

### `clip`

[`GradientClipOptions`](GradientClipOptions.md) `clip = {}`
: Optional gradient clipping, applied before the moment updates.

### `execution`

[`ExecutionOptions`](ExecutionOptions.md) `execution = {}`
: Which update lane runs, strictness, and capturable mode.

## Guidance

Leave `decoupled_weight_decay` at its default `false` unless there is a
specific reason to want `AdamW`-style decoupled decay together with
`NAdam`'s own Nesterov correction -- most training recipes that want
decoupled decay simply reach for [`AdamW`](AdamW.md) directly instead.
`momentum_decay` is rarely worth tuning away from its default; if `NAdam`
is not converging well, check `learning_rate` first, remembering its
default is deliberately larger than `Adam`'s own.

## See Also

- [`NAdam`](NAdam.md) -- the optimizer this configures.
- [`NAdamGroup`](NAdamGroup.md) -- per-group `NAdamOptions` overrides.
- [`AdamOptions`](AdamOptions.md) / [`AdamWOptions`](AdamWOptions.md) --
  the two weight-decay mechanisms `decoupled_weight_decay` chooses
  between.
