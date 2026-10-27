<!-- generated from optima_06_curvature_linear_algebra_scaling.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 6: Curvature, Linear Algebra and Scaling

The three algorithms of chapters 4 and 5 differ in how they treat constraints. They share three things that decide how fast and how reliably any of them runs: the model of curvature each step uses, the linear system each step solves, and the units the problem is written in. This chapter measures each. After it you can pick a curvature model for the size of your problem, choose a linear solver with a reason, and know when to rewrite your model's units instead of asking a solver to cope.

Every program is complete and its output is the output of that program.

## The mental model

A step of any of these methods minimizes a quadratic model of the problem. The model has a gradient and a curvature matrix `B`, an approximation to the Hessian of the Lagrangian (the objective plus the multiplier-weighted constraints). Three layers sit under every iteration.

| Layer | The question | The settings |
|---|---|---|
| Curvature | where does `B` come from? | `hessian_approximation`, `lbfgs_memory`, `bfgs_curvature_tolerance` |
| Linear algebra | how is the step's linear system solved? | `linear_system_solver`, `system_structure`, `max_krylov_iterations`, `iterative_dimension_threshold` |
| Scaling | what are the units of the numbers it sees? | `problem_scaling`, and your own choice of units |

The curvature models are `Bfgs` (a dense matrix, updated from each step and the change in gradient), `Lbfgs` (rebuilt from the last few pairs), `Identity` (no curvature), and the exact models `Exact` and `HessianVector` (the true Hessian of the Lagrangian). All three algorithms accept all of them.

## Curvature

- [Curvature models](optima_06_curvature_linear_algebra_scaling/curvature_models.md)
- [The step's linear algebra, and units](optima_06_curvature_linear_algebra_scaling/linear_algebra_and_units.md)

## Decisions

| Situation | Choice |
|---|---|
| Up to a few hundred variables | `Bfgs` (the default) |
| Many variables, interior method | `Lbfgs`; the default memory of 10 is on the flat part of the curve |
| Tempted to skip curvature to keep things simple | do not: `Identity` did not converge at any size in Program 1 |
| Tempted to raise `bfgs_curvature_tolerance` | do not; leave it at its default (Program 4) |
| Small or medium problem | leave `linear_system_solver` on `Auto` (factorization) |
| Thousands of variables | leave it on `Auto`; conjugate gradients switch on at the threshold |
| Quantities that differ by orders of magnitude | leave `problem_scaling` on its default (`ObjectiveAndConstraints`); it converges in every case of Program 6 |

## Pitfalls

- Asking for an exact model on an objective given only as a value and a gradient: refused with exit flag `-5` (Program 3).
- Switching `problem_scaling` to `None` or `InitialPoint` on a problem whose numbers span many orders of magnitude (Program 6).
- Reading `Converged` as `Optimal`: the flag certifies first-order conditions at the point returned and nothing more.

## Where next

Chapter 7 covers derivatives: where the gradients and Jacobians come from, how to supply your own, and the exact-derivative curvature models that the interior method accepts.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optima_06_curvature_linear_algebra_scaling/curvature_models
optima_06_curvature_linear_algebra_scaling/linear_algebra_and_units
```
<!-- apiref:toctree:end -->
