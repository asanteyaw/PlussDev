# `pluss::ai::nn::optim::AdamWOptions`

The same fields as [`AdamOptions`](AdamOptions.md) -- learning rate,
`beta1`/`beta2`, epsilon, weight decay, AMSGrad, `maximize`, plus
[`clip`](GradientClipOptions.md)/[`execution`](ExecutionOptions.md) -- with
one difference: `weight_decay` applies as *decoupled* decay (a direct
shrink on the parameter) rather than folded into the gradient, and its
default value differs accordingly.

Passed to [`AdamW`](AdamW.md)'s constructor.

## Fields

### `learning_rate`

`double learning_rate = 1e-3`
: Same meaning as [`AdamOptions::learning_rate`](AdamOptions.md).

### `beta1`

`double beta1 = 0.9`
: Same meaning as [`AdamOptions::beta1`](AdamOptions.md).

### `beta2`

`double beta2 = 0.999`
: Same meaning as [`AdamOptions::beta2`](AdamOptions.md).

### `epsilon`

`double epsilon = 1e-8`
: Same meaning as [`AdamOptions::epsilon`](AdamOptions.md).

### `weight_decay`

`double weight_decay = 1e-2`
: **Decoupled** weight decay: each step first shrinks the parameter
  directly by `1 - learning_rate * weight_decay`, independent of the
  gradient or the moment estimates, then the ordinary Adam moment-based
  update runs on top using the (undecayed) gradient. Defaults to `1e-2`
  here -- unlike [`AdamOptions::weight_decay`](AdamOptions.md), which
  defaults to `0.0` -- since decoupled decay is meant to be on by default
  in `AdamW`. Set to `0.0` to disable it entirely. Must be `>= 0.0` and
  finite.

### `amsgrad`

`bool amsgrad = false`
: Same meaning as [`AdamOptions::amsgrad`](AdamOptions.md).

### `maximize`

`bool maximize = false`
: Same meaning as every other optimizer's `maximize` field.

### `clip`

[`GradientClipOptions`](GradientClipOptions.md) `clip = {}`
: Optional gradient clipping, applied before the moment updates -- never
  touches the decoupled decay step, which runs on the parameter directly.

### `execution`

[`ExecutionOptions`](ExecutionOptions.md) `execution = {}`
: Which update lane runs, strictness, and capturable mode.

## Guidance

Reach for `AdamW` over `Adam` by default whenever weight decay matters at
all for the model being trained -- the decoupled formulation is what most
modern training recipes actually mean by "Adam with weight decay," and
`AdamWOptions{}`'s own default already reflects that. Tune
`weight_decay` the way an L2 penalty is normally tuned: larger values
regularize more aggressively at the cost of underfitting if pushed too
far; `0.0` disables it, leaving the same moment-based update `Adam` itself
runs (see [`AdamW`](AdamW.md) Example 1 -- rerun with `weight_decay =
0.0` instead of its default, it matches `Adam`'s own Example 1 up to
floating-point rounding).

## See Also

- [`AdamW`](AdamW.md) -- the optimizer this configures.
- [`AdamWGroup`](AdamWGroup.md) -- per-group `AdamWOptions` overrides.
- [`AdamOptions`](AdamOptions.md) -- the coupled-decay counterpart.
