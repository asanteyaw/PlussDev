<!-- generated from septima_02_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Septima 2: Constraints

Chapter 1 minimized a function with nothing in its way. Real problems have limits: a parameter must stay positive, two quantities must add up to a total, a measure must not exceed a cap. This chapter shows the three kinds of constraint Septima accepts, how each is enforced, and what to expect. After it you can state a constrained problem, read the multipliers, tell whether a constrained run has converged, and compare Septima with Optima on the same problem.

Every program is complete and its output is the output of that program.

## The mental model

Septima uses only gradients, so it enforces constraints in the way first-order methods do. It treats the three kinds differently.

| Constraint | Written as | Enforced by |
|---|---|---|
| Bounds | `constraints.lbs`, `constraints.ubs`: one `Series` per parameter block, in block order | **projection**: after every step each parameter is clipped into its bounds, so the iterates never leave the box |
| Equalities | `constraints.equalities`: a function from the list of blocks to a list of residual `Series`, which must be zero | **augmented Lagrangian** (the method of multipliers) |
| Inequalities | `constraints.inequalities`: the same, with residuals that must be at most zero | **augmented Lagrangian**, with a non-negative multiplier |

An inequality residual is positive when the constraint is violated, as in Optima. The residual functions receive the current parameter blocks as `Series` and return a `std::vector<Series>`; a residual may itself be a vector, with one entry per constraint.

The augmented Lagrangian adds to the objective, for each equality residual `h`, the terms `lambda * h + 0.5 * rho * h^2`, and for each inequality residual `h` the term `(max(0, lambda + rho * h)^2 - lambda^2) / (2 * rho)`, which is flat while the constraint is comfortably slack and grows once it is violated. `rho` is the **penalty**, `lambda` the **multiplier**. The run is organised in cycles:

1. **Solve.** The optimizer (Adam or L-BFGS) works on the augmented function, with the multipliers held fixed.
2. **Update.** Once that function is solved well enough, each multiplier moves by the penalty times its residual: `lambda += rho * h` for an equality, `lambda = max(0, lambda + rho * h)` for an inequality. "Well enough" means that the stationarity of the augmented function is at most `dual_update_ratio` (0.1) times the larger of the current violation and the multipliers' distance from complementarity, or that the optimizer can no longer move, or that `dual_update_max_steps` (50) steps have passed.
3. **Raise the penalty if needed.** If, after an update that followed a solved inner problem, the violation is still more than `penalty_growth_trigger` (a quarter) of what it was at the previous update, the penalty is multiplied by `penalty_growth` (10), up to `max_penalty`. A penalty that was too small for the problem is therefore raised by the run itself.

At the answer the multipliers are the Lagrange multipliers, the prices of the constraints. The solver reports convergence when `stationarity` (defined in chapter 1: the gradient of the augmented problem with the components that an active bound blocks removed), the last step and `constraint_violation` are all within tolerance.

Because the penalty and the multiplier step set themselves, the settings rarely need tuning. The starting penalty (`equality_penalty` and `inequality_penalty`, default 10) matters little, as Program 2 shows.

## Programs

- [Bounds, equalities and inequalities](septima_02_constraints/bounds_equalities_and_inequalities.md)

## Settings you will use

| Setting (in `options.step` unless noted) | Meaning | Default |
|---|---|---|
| `equality_penalty`, `inequality_penalty` | starting weight of the quadratic penalty | 10 |
| `dual_learning_rate` | step of the multiplier update; 0 means the penalty itself | 0 |
| `dual_update_ratio`, `dual_update_max_steps`, `dual_update_floor` | when the augmented problem counts as solved | 0.1, 50, 1e-10 |
| `penalty_growth`, `penalty_growth_trigger`, `max_penalty` | when and how the run raises the penalty; a growth of 1 keeps it fixed | 10, 0.25, 1e8 |
| `project_bounds` | clip into bounds after every step; if `false` the bounds are only measured | `true` |
| `enable_constraint_autodiff` | include the penalty and multiplier terms in the gradient; if `false` only bounds are enforced | `true` |
| `constraint_tolerance` (in the solve options) | violation allowed at convergence | 1e-6 |

The multipliers are in `result.state.lambda_equalities` and `result.state.lambda_inequalities`, one `Series` per residual function, shaped like the residuals. `result.state.penalty_scale` is the factor by which the run raised the penalty, and `result.state.dual_updates` the number of multiplier updates.

## What to remember

- Bounds are enforced exactly, by clipping. Equalities and inequalities are enforced by the method of multipliers, and converge to the true solution and the true multipliers.
- The schedule sets itself: the multiplier step is the penalty, the multipliers are updated once the augmented problem is solved, and the penalty is raised when the violation does not shrink. The starting penalty rarely matters.
- A converged multiplier is the true Lagrange multiplier, so Septima can report the price of each constraint.
- Read `stationarity` for optimality and `constraint_violation` for feasibility. The solver's violation ends near its tolerance (1e-6), looser than Optima's.
- Do not override `dual_learning_rate` with a value far below the penalty: the run then inflates the penalty and the multipliers stay wrong.
- For a small problem with nonlinear constraints that must hold to high precision, Optima needs far fewer evaluations.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

septima_02_constraints/bounds_equalities_and_inequalities
```
<!-- apiref:toctree:end -->
