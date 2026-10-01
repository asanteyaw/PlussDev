# `pluss::ai::nn::optim::StepDiagnosticsOptions`

Opt-in fail-fast checks run during [`step`](Optimizer/step.md), each
independently toggled and each off by default. Unlike
[`GradientClipOptions`](GradientClipOptions.md)/
[`ExecutionOptions`](ExecutionOptions.md), this is not a field on every
optimizer's own `Options` struct -- it is set once directly on the
optimizer itself, via
[`Optimizer::set_step_diagnostics`](Optimizer.md#state)/`step_diagnostics()`,
and applies to every `step()` call that optimizer makes afterward,
regardless of which overload is used.

## Fields

### `fail_if_no_sensitivity`

`bool fail_if_no_sensitivity = false`
: Throws from the no-arg [`step()`](Optimizer/step.md) overload when at
  least one parameter tracks its slope but *none* of the tracked
  parameters have a recorded sensitivity to read -- the training-loop
  equivalent of "I expected `.reverse()` to have run before this call, and
  it clearly did not." Only the no-arg overload's own gradient-collection
  path checks this; it has no effect on the `gradients` or closure
  overloads.

### `minimum_tracked_coverage`

`double minimum_tracked_coverage = 0.0`
: A fraction in `[0, 1]`. When `> 0.0`, the no-arg `step()` overload
  throws if the proportion of tracked parameters that actually received a
  sensitivity this round falls below it. `0.0` (the default) disables the
  check entirely. Set this below `1.0` deliberately for a model where some
  parameters are legitimately expected to go untouched on a given step
  (a mixture-of-experts gate that only some parameters route through, for
  instance); set it to `1.0` to require every tracked parameter to have
  received a gradient.

### `fail_if_non_finite_sensitivity`

`bool fail_if_non_finite_sensitivity = false`
: Throws when any gradient supplied to `step` -- whichever overload -- is
  defined but contains a `NaN` or `Inf` value. Checked against every
  gradient individually, whether it came from `.reverse()`, an explicit
  `gradients` argument, or a closure.

### `fail_if_non_finite_parameter`

`bool fail_if_non_finite_parameter = false`
: Throws when any parameter *itself* (its current value, before this
  step's update is applied) contains a `NaN` or `Inf`. Catches a parameter
  that already diverged on a previous step, before spending an update on
  it.

### `fail_if_non_finite_loss`

`bool fail_if_non_finite_loss = false`
: Throws when a closure passed to either `step(closure)` overload returns
  a non-finite loss. Checked immediately after the closure returns, before
  `.reverse()` runs (for the `Series`-returning overload) or before the
  supplied gradients are used (for the pair-returning overload). Has no
  effect on the no-arg or `gradients` overloads, since neither one is
  given a loss value to check.

## Guidance

All five default to `false`/`0.0` -- an optimizer with no diagnostics set
behaves exactly as it did before this options struct existed, with zero
added checking cost. Turning them on trades a small amount of per-step
validation cost for failing loudly, at the exact step where something
went wrong, instead of training silently on garbage (a `NaN` parameter
that quietly propagates for hundreds of steps before a loss finally prints
as `nan`) or not training at all (a forgotten `.reverse()` call that
`fail_if_no_sensitivity` would have caught on step one). Turning on
`fail_if_non_finite_sensitivity`/`fail_if_non_finite_parameter`/
`fail_if_non_finite_loss` together during early development of a new
model or a new loss function is a reasonable default; relaxing them once
training is stable is a matter of preference, not correctness.

## See Also

- [`Optimizer`](Optimizer.md) -- `set_step_diagnostics`/`step_diagnostics`
  are covered in that page's own `## State` section, not on a dedicated
  method page, since both are a plain one-line get/set pair.
- [`Optimizer::step`](Optimizer/step.md) -- exactly which overloads each
  check above applies to.
