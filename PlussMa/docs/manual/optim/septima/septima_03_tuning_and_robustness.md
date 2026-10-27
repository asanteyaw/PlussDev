<!-- generated from septima_03_tuning_and_robustness.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Septima 3: Tuning and Robustness

Chapters 1 and 2 used Septima's settings without explaining them. This chapter measures what each one does: the Adam learning rate and its companions, the L-BFGS history and line search, what to do when the objective is undefined in part of the space, what each stopping rule means, and how precision and complex parameters behave. After it you can choose the learning rate for a new problem, predict which optimizer a problem needs, keep a run out of a region where the objective is undefined, and read the exit flag as information rather than as a verdict.

Every program is complete and its output is the output of that program.

## The mental model

Septima has two independent groups of settings. The **step** settings, in `options.step`, decide how a point moves: the optimizer and its parameters. The **solve** settings, directly in `SeptimaSolveOptions`, decide when to stop: the budget and the tolerances. Keep them apart in your mind. A run that stops too early is a solve-setting question; a run that moves badly is a step-setting question.

| Group | Setting | What it does |
|---|---|---|
| step, Adam | `adam.learning_rate` | the size of every step; the setting that matters most |
| | `adam.beta1`, `adam.beta2` | how long the gradient average and the squared-gradient average remember (0.9, 0.999) |
| | `adam.eps` | a small constant that keeps the division safe |
| | `adam.weight_decay`, `adam.decoupled_weight_decay` | a pull of the parameters toward zero, added to the gradient or applied to the parameter directly |
| | `adam.amsgrad` | use the largest squared-gradient average seen so far, which stops steps from growing back |
| | `adam.maximize` | maximize instead of minimize |
| step, L-BFGS | `lbfgs.history_size` | how many recent step pairs are remembered (20) |
| | `lbfgs.line_search` | `None` takes every step at the full learning rate; `StrongWolfe` shortens a step until the objective has fallen enough |
| | `lbfgs.learning_rate` | the length of the first trial step (1.0) |
| | `lbfgs.curvature_tolerance` | a step pair is remembered only if its curvature is positive by this margin |
| | `lbfgs.wolfe_c1`, `lbfgs.max_line_search_iterations`, `lbfgs.tolerance_change` | the sufficient-decrease constant, the cap on step shortenings, and the smallest slope or step treated as real |
| solve | `max_iterations` | the budget (200) |
| | `gradient_tolerance`, `step_tolerance`, `constraint_tolerance` | the three tests for `Converged` (1e-6, 1e-10, 1e-6) |
| | `objective_tolerance`, `max_no_progress_iterations` | the test for `NoProgress` (1e-12, 25) |
| | `stop_on_nonfinite_objective` | end the run when the objective or gradient is NaN or infinite (`true`) |

## Programs

- [Tuning Adam and L-BFGS](septima_03_tuning_and_robustness/tuning_adam_and_lbfgs.md)
- [Robustness, stopping rules and precision](septima_03_tuning_and_robustness/robustness_and_stopping.md)

## A procedure for a new problem

1. Start with L-BFGS if the problem is smooth and unconstrained; it needs no tuning and few iterations. Use Adam when the gradient is noisy, when the problem has constraints, or when L-BFGS misbehaves.
2. For Adam, find the learning rate by trying 0.001, 0.01, 0.1 and 1 for a few hundred iterations each, and keep the largest one whose objective keeps falling.
3. Bound any parameter whose objective is undefined outside a range, a little inside the edge.
4. Run once with a progress hook (chapter 1) to see whether the objective falls steadily.
5. Set the tolerances to what you will use. Read the exit flag together with `objective_value`, `stationarity` and `constraint_violation`.

## What to remember

- The learning rate is the setting that matters. Too small wastes the budget; too large oscillates; the working range is about one order of magnitude wide.
- Weight decay changes the answer on purpose. Leave the momentum settings alone unless you have a reason.
- L-BFGS needs a history of about 10 to 20. The strong-Wolfe line search saves iterations and costs evaluations; use it when full steps are not safe. With constraints a backtracking test is applied automatically.
- `nan_recovery_attempts` guards the update, not the objective. Keep iterates inside the objective's domain with bounds.
- Asking for a tolerance the method cannot reach changes the exit flag, not the answer.
- Single and double precision, real and complex parameters all work; a complex parameter needs a real loss and no bounds.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

septima_03_tuning_and_robustness/tuning_adam_and_lbfgs
septima_03_tuning_and_robustness/robustness_and_stopping
```
<!-- apiref:toctree:end -->
