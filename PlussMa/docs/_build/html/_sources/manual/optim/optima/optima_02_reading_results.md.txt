<!-- generated from optima_02_reading_results.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 2: Reading Results and Watching a Run

A solver run can end in many ways, and only one of them means "here is your answer". This chapter is about telling them apart: which exit flag was returned and what each one promises, how the stall category narrows down a failure, what every field of the structured report means, how to watch a run as it happens, how to stop it early, and how to read the Lagrange multipliers. After it you can write code that trusts a result only when it should, and that explains a failure when it does not.

Every program is complete and its output is the output of that program.

## The mental model

Four independent pieces of information come back from every call. Keep them separate.

- **The exit flag** (`result.exit_flag`) is the verdict, one of nine values. It is the thing to branch on.
- **`result.converged`** is a convenience: it is true exactly when the exit flag is `Converged`. It is not true merely because the solver stopped.
- **The stall category** (`result.output.stall_category`) names the kind of difficulty behind a stop that was not a success. It narrows a failure down; it never changes the verdict.
- **The message** (`result.message`) is human-readable text for logs. Its wording can change between versions. Never branch on it.

Whatever the verdict, `result.params` always holds a point: the last iterate, or, for some stops, the best feasible point the run found. It is only a solution when the flag says so.

## The nine exit flags

| Flag | Value | What it promises | What `result.params` holds |
|---|---|---|---|
| `Converged` | 1 | Constraint violation and first-order optimality are within tolerance, or the iterate is a stationary feasible point. | The solution. |
| `MaxIterations` | 0 | The iteration budget was spent first. | The last iterate; `best_feasible` may be better. |
| `Stopped` | -1 | Your progress hook returned `true`. | The point at which you stopped. |
| `NoFeasiblePoint` | -2 | The interior method could not find a point strictly inside the bounds and linear rows to start from. | The start, nudged inward. |
| `ObjectiveTargetReached` | -3 | A feasible point at or below `objective_limit` was found. | That point. |
| `NonFiniteEvaluation` | -4 | The objective, gradient, a constraint or a Jacobian was not a finite number. | The point at which the non-finite value appeared. |
| `InvalidProblem` | -5 | A callback broke its contract (wrong result shape, a thrown error). | The start. |
| `NoProgress` | -6 | The line search, the recovery step or the step to the boundary could not make progress. | The last iterate. |
| `ConstraintStationary` | -7 | The constraint violation stopped decreasing at a point that is still infeasible. | The last iterate, infeasible. |

Only the first row is a success. `ObjectiveTargetReached` is a success you asked for, but it does not claim optimality, which is why `converged` is false for it.

## Programs

- [Exit flags and stall categories](optima_02_reading_results/exit_flags_and_stall_categories.md)
- [The report, the summaries and the multipliers](optima_02_reading_results/the_report_summaries_and_multipliers.md)
- [Watching a run](optima_02_reading_results/watching_a_run.md)

## Decisions

- **What to branch on.** Branch on `exit_flag`. Use `converged` as a shorthand for "the flag is `Converged`". Treat the message as text for people.
- **What to do after a failure.** If the flag is `MaxIterations` or `Stopped`, read `output.best_feasible_available`; the checkpoint may be a usable answer. If it is `NoProgress`, `NoFeasiblePoint` or `ConstraintStationary`, suspect the problem (infeasible constraints, a bad start) before the solver settings.
- **How much progress output.** `reports::iter` for a watched run. A custom hook for a monitor, a stop rule, or a record. Nothing for production.

## Where next

The next chapter compares the four algorithms: which problems each one handles well, where each one fails, and how to choose between them.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optima_02_reading_results/exit_flags_and_stall_categories
optima_02_reading_results/the_report_summaries_and_multipliers
optima_02_reading_results/watching_a_run
```
<!-- apiref:toctree:end -->
