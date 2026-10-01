# `nn::optim` Introduction

## What This Category Owns

`optim` owns everything that turns a computed gradient into a parameter
update: the [`Optimizer`](Optimizer.md) base class every concrete
algorithm derives from, the three options structs shared across all of
them ([`GradientClipOptions`](GradientClipOptions.md),
[`ExecutionOptions`](ExecutionOptions.md),
[`StepDiagnosticsOptions`](StepDiagnosticsOptions.md)), the per-step
report type ([`StepLaneTelemetry`](StepLaneTelemetry.md)), and seven
concrete optimizers:

- [`SGD`](SGD.md) -- momentum, dampening, weight decay, Nesterov.
- [`Adagrad`](Adagrad.md) -- per-parameter accumulated-squared-gradient
  scaling; a learning rate that shrinks fastest for parameters that have
  moved the most.
- [`Adam`](Adam.md) -- the default choice for most models: adaptive
  per-parameter learning rates from tracked first and second moments of
  the gradient.
- [`AdamW`](AdamW.md) -- `Adam` with weight decay applied directly to the
  parameter (decoupled) rather than folded into the gradient.
- [`NAdam`](NAdam.md) -- `Adam` with Nesterov-style momentum folded into
  the first-moment update.
- [`RMSprop`](RMSprop.md) -- root-mean-square gradient scaling, with
  optional momentum and centering.
- [`Minimax`](Minimax.md) -- a primal-dual optimizer for training under
  equality/inequality constraints, built on top of any of the other six's
  own primal update rule.

## The Shared Contract

Every concrete optimizer is an [`Optimizer`](Optimizer.md) subclass and
inherits its entire public surface unchanged: construct it against a
parameter list (or a [`Module`](../module/Module.md) directly), then call
one of [`step`](Optimizer/step.md)'s four overloads repeatedly. Nothing
about [`parameters`](Optimizer/parameters.md), [`size`](Optimizer/size.md),
[`clear`](Optimizer/clear.md), or
[`last_step_lane_telemetry`](Optimizer/last_step_lane_telemetry.md) differs
between `SGD` and `Adam` -- read [`Optimizer`](Optimizer.md) once, and that
knowledge carries to every algorithm on this page unchanged.

What *does* differ per algorithm is its own `Options` struct's fields (the
actual update formula's hyperparameters -- learning rate, momentum,
betas, ...) and its own per-parameter state (a velocity buffer, a running
squared-gradient accumulator, first/second moment estimates, ...). Every
`Options` struct, across all seven optimizers, ends in the same two
fields -- `clip` ([`GradientClipOptions`](GradientClipOptions.md)) and
`execution` ([`ExecutionOptions`](ExecutionOptions.md)) -- so gradient
clipping and update-lane selection are configured identically everywhere,
independent of which algorithm's own formula runs.

## Per-Group Options And `capture_state`/`restore_state`

Every concrete optimizer supports two additional things beyond
`Optimizer`'s own contract, on every one of the seven pages in this
category:

- **Per-group options.** A second constructor overload accepts a list of
  `<Algorithm>Group` values (`SGDGroup`, `AdamGroup`, ...) -- each a
  parameter subset paired with its own `Options`, so different parts of a
  model (say, a pretrained backbone versus a freshly initialized head) can
  train under different learning rates, weight decay, or clipping in one
  optimizer instance, rather than needing one optimizer per subset.
- **`capture_state()` / `restore_state(state)`.** Every optimizer's
  internal state (velocity, moment estimates, accumulators, step count --
  whatever that algorithm tracks) can be captured into a plain `State`
  struct and restored later, independent of
  [`Module::snapshot`](../module/Module/snapshot.md)/
  [`restore`](../module/Module/restore.md) (which capture parameter
  *values*, not optimizer state). Checkpointing training resumably means
  saving both.

## Reading Order

1. [`Optimizer`](Optimizer.md) -- the shared base: construction,
   `step`'s four overloads, and the rest of the contract every concrete
   optimizer inherits unchanged.
2. [`GradientClipOptions`](GradientClipOptions.md) /
   [`ExecutionOptions`](ExecutionOptions.md) /
   [`StepDiagnosticsOptions`](StepDiagnosticsOptions.md) --
   the three shared options structs every algorithm's own `Options` ends
   with (the first two) or composes separately (the third, set once on the
   optimizer rather than per algorithm).
3. Any one concrete algorithm ([`SGD`](SGD.md) is the simplest place to
   start) -- once the shared contract is familiar, each algorithm's own
   page is short: its formula, its own options fields, its own state.

## Related Pages

- [`module`](../module.md) -- `Module::parameters()` is what most
  constructors here consume directly.
- [`grad`](../grad.md) -- the autograd toggle that decides whether there is
  a recorded sensitivity for the no-arg `step()` overload to read.
- [`losses`](../losses.md) -- the loss functions a `step(closure)` call
  typically runs internally.
