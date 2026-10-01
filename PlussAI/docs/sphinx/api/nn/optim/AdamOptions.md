# `pluss::ai::nn::optim::AdamOptions`

The hyperparameters [`Adam`](Adam.md)'s own update formula reads: learning
rate, the two moment-decay rates, epsilon, weight decay, AMSGrad, and
`maximize`, plus the two fields every optimizer's `Options` ends with,
[`clip`](GradientClipOptions.md) and
[`execution`](ExecutionOptions.md).

Passed to [`Adam`](Adam.md)'s constructor; every field has a default, so
`AdamOptions{}` reproduces the conventional Adam hyperparameters from the
original paper.

## Fields

### `learning_rate`

`double learning_rate = 1e-3`
: The base step size, before adaptive scaling by the bias-corrected
  second moment. Must be `> 0.0` and finite. An order of magnitude smaller
  than [`SGD`](SGDOptions.md)'s own default -- Adam's adaptive scaling
  tends to make larger raw learning rates unstable.

### `beta1`

`double beta1 = 0.9`
: The decay rate for the first-moment (mean gradient) running estimate --
  `first_moment = beta1 * first_moment + (1 - beta1) * gradient`. Closer
  to `1.0` means a longer effective averaging window (smoother, slower to
  react to recent gradients); closer to `0.0` means the estimate tracks
  the most recent gradient more closely. Must be in `[0, 1)`.

### `beta2`

`double beta2 = 0.999`
: The decay rate for the second-moment (mean squared gradient) running
  estimate. Conventionally kept close to `1.0` (a long averaging window)
  so the adaptive scaling reflects a stable estimate of each parameter's
  typical gradient magnitude, not just the most recent step's. Must be in
  `[0, 1)`.

### `epsilon`

`double epsilon = 1e-8`
: A small constant added to the (bias-corrected) second moment's square
  root before dividing, to avoid dividing by (near) zero early in
  training. Must be `> 0.0` and finite.

### `weight_decay`

`double weight_decay = 0.0`
: L2 regularization strength, folded into the gradient before the moment
  updates -- the same *coupled* decay [`SGDOptions`](SGDOptions.md) uses.
  `0.0` (the default) disables it. See [`AdamW`](AdamW.md) for the
  decoupled alternative most training recipes actually recommend when
  weight decay matters.

### `amsgrad`

`bool amsgrad = false`
: When `true`, the update divides by the *maximum* second-moment estimate
  ever observed for that parameter, instead of the current (decayed)
  estimate -- a fix for a known convergence issue in the original Adam
  analysis. `false` (the default) is plain Adam. See
  [`Adam`](Adam.md) Example 2 for its concrete effect after a gradient
  spike.

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

`AdamOptions{}`'s defaults are a reasonable starting point for most
models -- they are the same defaults most published training recipes
assume when they say "Adam" without further qualification. Lower
`learning_rate` first if training is unstable before touching `beta1`/
`beta2`; both betas are rarely worth tuning away from their defaults
except for very specific, well-understood cases (a very short training
run can benefit from a smaller `beta2`, since the long averaging window
otherwise barely has time to form). Reach for `amsgrad = true` only if
training shows the specific instability it targets (a loss that spikes
and then fails to fully recover); it is not a strict improvement in
general, and adds a third state buffer's worth of memory per parameter.

## See Also

- [`Adam`](Adam.md) -- the optimizer this configures.
- [`AdamGroup`](AdamGroup.md) -- per-group `AdamOptions` overrides.
- [`AdamWOptions`](AdamWOptions.md) -- the same fields, with `weight_decay`
  applied as decoupled decay instead.
