# Optima Sequential QP

`SequentialQP` is the dense constrained nonlinear-programming backend for `optima(...)`.

It is designed for derivative-based constrained optimization over moderate parameter counts, with gradients and Jacobians coming from `PlussQit` autodiff or analytic callbacks.

## Mathematical Problem

The backend targets nonlinear programs of the form

```{math}
\begin{aligned}
\min_x \quad & f(x) \\
\text{s.t.} \quad & c_{\mathrm{eq}}(x) = 0, \\
& c_{\mathrm{ineq}}(x) \le 0, \\
& A_{\mathrm{eq}} x = b_{\mathrm{eq}}, \\
& A_{\mathrm{ineq}} x \le b_{\mathrm{ineq}}, \\
& \ell \le x \le u.
\end{aligned}
```

At a candidate solution, the first-order KKT conditions are expressed in terms of the Lagrangian

```{math}
\mathcal{L}(x,\lambda,\nu)=
f(x)
 + \lambda_{\mathrm{eq}}^\top c_{\mathrm{eq}}(x)
 + \lambda_{\mathrm{ineq}}^\top c_{\mathrm{ineq}}(x)
 + \nu_{\mathrm{eq}}^\top (A_{\mathrm{eq}}x-b_{\mathrm{eq}})
 + \nu_{\mathrm{ineq}}^\top (A_{\mathrm{ineq}}x-b_{\mathrm{ineq}}).
```

`SequentialQP` tracks a practical KKT residual derived from the gradient of this Lagrangian together with active constraint residuals.

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

## Local Quadratic Subproblem

At iteration $k$, the solver forms a quadratic model around the current point $x_k$:

```{math}
\begin{aligned}
\min_p \quad &
\nabla f(x_k)^\top p + \tfrac12 p^\top B_k p \\
\text{s.t.} \quad &
c_{\mathrm{eq}}(x_k) + J_{\mathrm{eq}}(x_k) p = 0, \\
& c_{\mathrm{ineq}}(x_k) + J_{\mathrm{ineq}}(x_k) p \le 0,
\end{aligned}
```

where:

- $B_k$ is the current Hessian approximation to the Lagrangian
- $J_{\mathrm{eq}}$ and $J_{\mathrm{ineq}}$ are Jacobians assembled from autodiff or analytic callbacks

The implementation solves the corresponding KKT system using the configured linear-system policy.

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

When `Bfgs` is active, the update is based on the standard secant pair

```{math}
s_k = x_{k+1} - x_k,\qquad
y_k = \nabla \phi_{k+1} - \nabla \phi_k,
```

where $\phi$ is the merit-gradient model used by the solver. The update is skipped when curvature is too small, controlled by `bfgs_curvature_tolerance`.

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

The main merit model is an exact-penalty style scalarization:

```{math}
\Phi(x;\rho) = f(x) + \rho \, \|v(x)\|_1,
```

where $v(x)$ denotes the stacked constraint residual used by the implementation. The penalty parameter $\rho$ is adapted when feasibility reduction is lagging.

## Second-Order Correction

After a first QP step $p_k$ is found, the solver can attempt a second-order correction when nonlinear residuals are not captured well by the first linearization. In practice this means re-solving with a corrected residual target instead of accepting a step that only looks feasible in the local linear model.

## Restoration

When the main SQP step fails repeatedly, the backend can switch to a restoration phase that prioritizes violation reduction. A successful restoration step resets the local Hessian model before returning to the main loop.

The restoration target is conceptually

```{math}
\min_x \; \|v(x)\|,
```

subject to the same bound handling logic, so that the solver can recover a usable feasible or near-feasible point before resuming the main SQP loop.

## Outputs

The top-level `optima(...)` call returns a state object and a structured `OptimaOutput` payload. For `SequentialQP`, the most important output fields are:

| Output field | Meaning |
| --- | --- |
| `iterations` | Major SQP iterations |
| `function_evaluations` | Objective/constraint evaluation count |
| `constraint_violation` | Maximum final constraint violation |
| `first_order_optimality` | Final KKT-style stationarity measure |
| `step_size` | Final accepted step fraction |
| `line_search_fraction` | Same accepted line-search fraction reported explicitly |
| `backend_parameter` | Current penalty parameter |
| `krylov_iterations` | Krylov work if iterative linear solves were used |
| `best_feasible_available` | Whether a feasible checkpoint was found |
| `returned_best_feasible` | Whether the solver returned that checkpoint instead of the final iterate |
| `multiplier_max_norm` | Largest multiplier magnitude in the reported multiplier structure |
| `active_multiplier_count` | Number of active multipliers |
| `detail_message` | Expanded exit detail |
| `stall_category` | Structured stop reason |

The returned multiplier structure contains:

- `linear_equalities`
- `nonlinear_equalities`
- `linear_inequalities`
- `nonlinear_inequalities`
- `lower_bounds`
- `upper_bounds`

## Example Trace

A typical progress line from an instrumented SQP run looks like:

```text
[sqp:iter] iter=12 evals=50 value=1.125000 viol=0.000000 stat=8.5e-08 step=1.000000 penalty=10.000000
```

Interpretation:

- `value`: current objective
- `viol`: current max constraint violation
- `stat`: KKT-style first-order measure
- `step`: accepted step fraction
- `penalty`: current merit penalty parameter

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
