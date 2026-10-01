# `pluss::ai::nn::optim::MinimaxOptions`

Every hyperparameter [`Minimax`](Minimax.md)'s primal-dual update reads:
which primal algorithm drives the parameter update and its own
hyperparameters, the shared [`clip`](GradientClipOptions.md) gradient
clipping, and the constraint-specific controls -- dual learning rate,
penalty weights, bound projection, the constraint-autodiff toggle, NaN
recovery, and the schedules governing how the dual learning rate and
penalty weights themselves evolve over training.

Passed to [`Minimax`](Minimax.md)'s constructor, separately from
[`MinimaxConstraints`](MinimaxConstraints.md) (the residual callables and
bounds themselves).

## Fields

### `primal_method`

`MinimaxPrimalMethod primal_method = MinimaxPrimalMethod::Adam`
: Which of the other four optimizers' own update formula drives the
  primal (parameter) step: `Adam`, `AdamW`, `SGD`, or `RMSprop`. The
  fields below that are specific to one primal method (`beta1`/`beta2`/
  `amsgrad` for `Adam`/`AdamW`; `momentum`/`rmsprop_alpha` for `SGD`/
  `RMSprop`) are simply ignored when a different method is selected.

### `learning_rate`

`double learning_rate = 1e-3`
: The primal step size -- same role as every other optimizer's own
  `learning_rate`, interpreted by whichever `primal_method` is selected.

### `beta1`

`double beta1 = 0.9`
: The `Adam`/`AdamW` first-moment decay rate -- see
  [`AdamOptions::beta1`](AdamOptions.md#fields). Ignored when
  `primal_method` is `SGD` or `RMSprop`.

### `beta2`

`double beta2 = 0.999`
: The `Adam`/`AdamW` second-moment decay rate -- see
  [`AdamOptions::beta2`](AdamOptions.md#fields). Ignored when
  `primal_method` is `SGD` or `RMSprop`.

### `epsilon`

`double epsilon = 1e-8`
: The denominator-stabilizing constant every primal method's adaptive
  scaling uses (`RMSprop`'s own denominator when selected; the
  bias-corrected second moment's when `Adam`/`AdamW` is selected). Ignored
  by plain `SGD`, which has no denominator to stabilize.

### `weight_decay`

`double weight_decay = 0.0`
: Weight decay strength for whichever `primal_method` is selected --
  coupled for `Adam`/`SGD`, decoupled for `AdamW` (see
  [`AdamWOptions::weight_decay`](AdamWOptions.md#fields)). Ignored when
  `primal_method` is `RMSprop`.

### `amsgrad`

`bool amsgrad = false`
: Same meaning as [`AdamOptions::amsgrad`](AdamOptions.md#fields).
  Ignored unless `primal_method` is `Adam` or `AdamW`.

### `maximize`

`bool maximize = false`
: Negates the objective gradient before the primal update -- gradient
  ascent instead of descent for the *objective*; the constraint penalty
  and dual updates are unaffected.

### `clip`

[`GradientClipOptions`](GradientClipOptions.md) `clip = {}`
: Optional gradient clipping on the objective gradient, applied before
  the primal update -- shared across every `primal_method`.

### `momentum`

`double momentum = 0.0`
: The `SGD`/`RMSprop` momentum coefficient -- see
  [`SGDOptions::momentum`](SGDOptions.md#fields). Ignored when
  `primal_method` is `Adam` or `AdamW`.

### `rmsprop_alpha`

`double rmsprop_alpha = 0.99`
: The `RMSprop` running-average smoothing rate -- see
  [`RMSpropOptions::alpha`](RMSpropOptions.md#fields). Ignored unless
  `primal_method` is `RMSprop`.

### `dual_learning_rate`

`double dual_learning_rate = 1e-2`
: The step size for the dual (Lagrange-multiplier) ascent update -- how
  quickly each constraint's own multiplier grows in response to that
  constraint being violated. Independent of `learning_rate`, which
  governs the primal side only.

### `equality_penalty`

`double equality_penalty = 10.0`
: The quadratic penalty weight applied to equality residuals
  (`0.5 * equality_penalty * sum(c(x)^2)`, added to the constraint
  objective the primal gradient is corrected by). `0.0` disables the
  quadratic penalty term entirely, leaving only the multiplier term to
  enforce the constraint (slower to converge, but still eventually
  enforced through the dual update alone).

### `inequality_penalty`

`double inequality_penalty = 10.0`
: The same role as `equality_penalty`, for inequality residuals
  (`0.5 * inequality_penalty * sum(relu(h(x))^2)`) and for bound
  violations.

### `project_bounds`

`bool project_bounds = true`
: When `true` (the default) and `lower_bounds`/`upper_bounds` are set on
  [`MinimaxConstraints`](MinimaxConstraints.md), the candidate parameter
  value is hard-clamped into the bounds after every step, on top of the
  soft penalty already pulling it there -- see [`Minimax`](Minimax.md)
  Example 3. `false` relies on the soft penalty alone, which can leave a
  parameter measurably outside its bounds, especially early in training.

### `enable_constraint_autodiff`

`bool enable_constraint_autodiff = true`
: When `true` (the default), the constraint penalty's gradient
  contribution to the primal step is computed via autodiff over the
  `_trace` residual callables
  ([`MinimaxConstraints::equalities_trace`](MinimaxConstraints.md#state)/
  `inequalities_trace`) every step -- this is what actually *enforces* a
  constraint against the primal trajectory, not just tracks its
  violation. `false` disables this: the dual multipliers still update
  (via the plain, non-`_trace` callables), but nothing pulls the primal
  parameter toward feasibility unless the caller's own supplied gradient
  already accounts for it.

### `nan_recovery_attempts`

`std::int64_t nan_recovery_attempts = 2`
: How many times a step that would produce a non-finite parameter value
  retries with a shrunk delta (see `nan_recovery_shrink`) before giving
  up and applying the non-finite update anyway. `0` disables recovery
  entirely.

### `nan_recovery_shrink`

`double nan_recovery_shrink = 0.5`
: The multiplicative factor applied to the update delta on each NaN
  recovery attempt -- `0.5` halves it each retry. Must be in `(0, 1)`.

### `dual_schedule`

`MinimaxSchedule dual_schedule = MinimaxSchedule::Constant`
: How `dual_learning_rate` evolves across steps: `Constant` (the
  default) leaves it unchanged; `Exponential` scales it by
  `dual_exponential_decay` raised to the step count; `ViolationAdaptive`
  scales it by the current moving constraint-violation estimate relative
  to `violation_target`, clamped to `[schedule_min_scale,
  schedule_max_scale]`.

### `penalty_schedule`

`MinimaxSchedule penalty_schedule = MinimaxSchedule::Constant`
: The same three schedule shapes as `dual_schedule`, applied instead to
  `equality_penalty`/`inequality_penalty` together, independently of
  whatever `dual_schedule` is set to.

### `dual_exponential_decay`

`double dual_exponential_decay = 1.0`
: Used only when `dual_schedule == Exponential`: the dual learning rate
  at step `t` is `dual_learning_rate * dual_exponential_decay^(t - 1)`.
  `1.0` (the default) is a no-op multiplier.

### `penalty_exponential_growth`

`double penalty_exponential_growth = 1.0`
: Used only when `penalty_schedule == Exponential`: both penalty weights
  at step `t` are scaled by `penalty_exponential_growth^(t - 1)`. `1.0`
  (the default) is a no-op multiplier.

### `violation_target`

`double violation_target = 1e-3`
: Used only by a `ViolationAdaptive` schedule: the constraint-violation
  magnitude the schedule treats as "on target" -- the scale factor is
  `clamp(moving_violation / violation_target, schedule_min_scale,
  schedule_max_scale)`, so violation well above this target scales the
  dual rate/penalties up, and violation well below it scales them down.

### `schedule_min_scale`

`double schedule_min_scale = 0.25`
: The lower clamp bound for a `ViolationAdaptive` schedule's own scale
  factor, preventing it from shrinking the dual rate/penalties to
  near-zero from a single noisy low-violation estimate.

### `schedule_max_scale`

`double schedule_max_scale = 4.0`
: The upper clamp bound for a `ViolationAdaptive` schedule's own scale
  factor, preventing it from growing the dual rate/penalties unboundedly
  from a single noisy high-violation estimate.

## Guidance

Leave every constraint-control field at its default unless training shows
a specific need: `Constant` schedules and the default penalty weights are
a reasonable starting point for most constrained problems.
`enable_constraint_autodiff = true` is required for
`equalities`/`inequalities` to actually pull the primal trajectory toward
feasibility (see [`Minimax`](Minimax.md)'s own `## Construction` section)
-- do not turn it off unless the caller's own supplied gradient already
accounts for the constraint penalty. Raise `equality_penalty`/
`inequality_penalty` if a constraint is being violated more than
expected at convergence; consider a `ViolationAdaptive` schedule instead
of hand-tuning a fixed penalty weight when the right weight is not known
in advance. Every group in a multi-group `Minimax` must share identical
values for every constraint-control field above (`dual_learning_rate`
through `schedule_max_scale`) -- only `primal_method` and its own
hyperparameters may vary per group; construction throws otherwise.

## See Also

- [`Minimax`](Minimax.md) -- the optimizer this configures.
- [`MinimaxConstraints`](MinimaxConstraints.md) -- the residual callables
  and bounds these constraint-control fields govern.
- [`MinimaxGroup`](MinimaxGroup.md) -- per-group `MinimaxOptions`
  overrides.
