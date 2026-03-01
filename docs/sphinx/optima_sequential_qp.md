# Optima Sequential QP

`SequentialQP` is the dense constrained nonlinear-programming backend for `optima(...)`.

It is designed for derivative-based constrained optimization over moderate parameter counts, with gradients and Jacobians coming from `PlussQit` autodiff or analytic callbacks.

## Solver Shape

The backend works by repeating this major loop:

1. evaluate objective and constraints
2. linearize constraints at the current point
3. build a local quadratic model of the Lagrangian
4. solve a KKT-style subproblem
5. apply line search and bound-feasible step control
6. optionally run second-order correction
7. optionally run restoration if the main step fails
8. update the Hessian model and multiplier estimate

## Main State

The internal SQP state tracks:

- current parameters
- current evaluation
- iteration count
- function evaluation count
- current step size
- current penalty parameter
- current merit value
- KKT residual
- convergence flag
- message
- best-feasible snapshot
- Hessian approximation
- multiplier estimate
- variable scaling
- normalization statistics
- Krylov iteration count

## Option Surface

`SequentialQPOptions` exposes:

| Option | Meaning |
| --- | --- |
| `max_iterations` | Major-iteration budget |
| `max_line_search_iterations` | Backtracking budget per major step |
| `max_second_order_corrections` | Second-order correction attempts |
| `max_restoration_iterations` | Restoration-phase major iterations |
| `max_restoration_line_search_iterations` | Backtracking budget inside restoration |
| `initial_step_size` | Starting trial step fraction |
| `backtracking` | Multiplicative line-search shrink |
| `armijo` | Sufficient decrease coefficient |
| `initial_penalty` | Starting merit penalty |
| `constraint_tolerance` | Feasibility tolerance |
| `optimality_tolerance` | First-order stop threshold |
| `step_tolerance` | Small-step stop threshold |
| `value_progress_tolerance` | Small-objective-change stop threshold |
| `active_set_tolerance` | Constraint activity threshold |
| `regularization` | KKT diagonal regularization |
| `bfgs_curvature_tolerance` | BFGS skip threshold |
| `second_order_correction_tolerance` | SOC activation threshold |
| `fraction_to_boundary` | Bound-feasible step fraction |
| `pivot_threshold` | Pivoted-direct linear solve threshold |
| `max_krylov_iterations` | Iterative solve budget |
| `iterative_dimension_threshold` | Size threshold for iterative routing |
| `krylov_tolerance` | Relative Krylov stop |
| `krylov_absolute_tolerance` | Absolute Krylov stop |
| `objective_limit` | Early exit target |
| `project_bounds` | Bound projection toggle |
| `enable_restoration` | Restoration fallback toggle |
| `system_structure` | Dense or sparse hint |
| `linear_system_solver` | Linear solve policy |
| `hessian_approximation` | Hessian model policy |
| `problem_scaling` | Scaling and normalization policy |
| `progress_hook` | Iteration callback |

## Hessian Handling

Current SQP Hessian policies:

- `Bfgs`
- `Identity`

The Hessian model is applied to the Lagrangian approximation inside the QP step.

## Linear-System Policy

The backend supports a policy-driven solve path:

- `Auto`
- `Cholesky`
- `PivotedDirect`
- `Inverse`
- `PseudoInverse`
- `ConjugateGradient`

`Auto` routes based on dimension and structure hints. Small dense systems usually stay on direct solves. Larger or structured problems can route to Krylov iterations when configured.

## Step Acceptance

`SequentialQP` uses a merit-based acceptance policy with additional constrained safeguards:

- Armijo decrease on the merit model
- stricter handling when the iterate is still infeasible
- second-order correction when linearized feasibility is too optimistic
- boundary-limited capping
- restoration fallback when the main step cannot improve the merit or feasibility model

This makes the backend more robust than a plain line-search QP loop.

## Restoration

When the main SQP step fails repeatedly, the backend can switch to a restoration phase that prioritizes violation reduction. A successful restoration step resets the local Hessian model before returning to the main loop.

## Reading the Iteration Trace

A useful progress hook for SQP usually prints:

- iteration index
- objective value
- max constraint violation
- KKT residual or stationarity
- merit or penalty parameter
- accepted step size
- Krylov iterations
- stall category

For example:

```cpp
options.algorithm = OptimaAlgorithm::SequentialQP;
options.sequential_qp.progress_hook = [](const OptimaProgressSnapshot& s) {
  std::cout << s.iteration
            << " f=" << s.evaluation.value.item<double>()
            << " viol=" << s.evaluation.max_constraint_violation
            << " stat=" << s.stationarity
            << " step=" << s.step_size
            << " penalty=" << s.backend_parameter
            << '\n';
  return false;
};
```

## Typical Exit Patterns

Expected clean exits:

- `Converged`
- `ObjectiveTargetReached`

Expected hard-case exits with diagnostics:

- `NoProgress` plus `search-direction-collapse`
- `NoProgress` plus `boundary-limited`
- `ConstraintStationary`
- `MaxIterations`
- `NonFiniteEvaluation`
- `InvalidProblem`

Best-feasible bookkeeping remains available even when the final iterate is not the best feasible point found during the run.

