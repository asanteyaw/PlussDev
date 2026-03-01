# Optima Internal Protocol

`InternalProtocol` is the Pluss-native public label for the primal-dual barrier backend currently implemented under the `OptimaAlgorithm::InteriorPoint` enum branch.

The intent is straightforward:

- the algorithmic family is still interior-point
- the public product naming stays Pluss-native

## Mathematical Problem

The backend targets the same constrained nonlinear program:

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

For inequality handling, the solver introduces slack variables

```{math}
c_{\mathrm{ineq}}(x) + s = 0,\qquad s > 0,
```

and dual variables $z > 0$ for the slack equations.

## Solver Shape

This backend uses a primal-dual barrier formulation with:

- explicit inequality slacks
- explicit inequality duals
- complementarity tracking
- barrier reduction over outer progress
- feasibility-phase fallback
- restoration handling

The major loop is:

1. evaluate the objective and constraints
2. assemble the barrier or primal-dual stationarity model
3. form a Newton-like or quasi-Newton direction
4. cap the step to preserve interiority
5. apply line search
6. update slack, dual, barrier, and curvature state
7. switch to feasibility or restoration modes if the main step stalls

## Primal-Dual System

The implementation follows a primal-dual barrier structure built around:

```{math}
\nabla_x \mathcal{L}(x,\lambda,z) = 0,
```

```{math}
c_{\mathrm{eq}}(x)=0,
\qquad
c_{\mathrm{ineq}}(x)+s=0,
```

```{math}
S z = \mu \mathbf{1},
\qquad
s>0,\; z>0,
```

where:

- $S=\operatorname{diag}(s)$
- $\mu$ is the barrier parameter

The complementarity residual reported by the solver is derived from the mismatch in the relation $Sz=\mu \mathbf{1}$.

## Main State

The internal state tracks:

- current parameters
- current evaluation
- iteration count
- function evaluation count
- current step size
- current barrier parameter
- current barrier value
- barrier gradient norm
- convergence flag
- message
- best-feasible snapshot
- Hessian model
- variable scaling
- normalization statistics
- Krylov iteration count
- limited-memory history
- inequality slacks
- inequality duals
- primal residual
- complementarity residual
- feasibility-phase status
- feasibility metric

## Barrier Objective View

In addition to the primal-dual residual system, it is useful to read the method as minimizing a barrier model of the form

```{math}
\phi_\mu(x,s) =
f(x)
- \mu \sum_i \log(s_i)
+ \tfrac12 \rho \, \|c_{\mathrm{eq}}(x)\|_2^2,
```

subject to $c_{\mathrm{ineq}}(x)+s=0$. This is also the right mental model for interpreting:

- `backend_parameter` as the current $\mu$
- `complementarity` as the centrality/complementarity error
- `constraint_violation` as primal infeasibility

## Option Surface

The underlying C++ option type is still `InteriorPointOptions`. It exposes:

| Option | Meaning |
| --- | --- |
| `max_iterations` | Major-iteration budget |
| `max_line_search_iterations` | Backtracking budget per main step |
| `max_restoration_iterations` | Recovery-phase iterations |
| `max_restoration_line_search_iterations` | Recovery line-search budget |
| `initial_step_size` | Starting trial step fraction |
| `backtracking` | Multiplicative line-search shrink |
| `armijo` | Sufficient decrease coefficient |
| `initial_barrier_parameter` | Starting barrier parameter |
| `minimum_barrier_parameter` | Lower barrier floor |
| `barrier_decay` | Barrier reduction factor |
| `equality_penalty` | Equality residual weight inside the barrier model |
| `centrality_fraction` | Centrality target strength |
| `dual_recentering_fraction` | Dual recentering amount |
| `feasibility_activation_factor` | Feasibility-phase trigger scale |
| `constraint_tolerance` | Feasibility threshold |
| `optimality_tolerance` | First-order stop threshold |
| `fraction_to_boundary` | Strict interior step cap |
| `pivot_threshold` | Pivoted-direct threshold |
| `initialization_margin` | Distance kept from the boundary at initialization |
| `bfgs_curvature_tolerance` | Curvature threshold for updates |
| `lbfgs_memory` | L-BFGS history size |
| `max_krylov_iterations` | Iterative solve budget |
| `iterative_dimension_threshold` | Size threshold for Krylov routing |
| `krylov_tolerance` | Relative Krylov stop |
| `krylov_absolute_tolerance` | Absolute Krylov stop |
| `hvp_options` | Hessian-vector autodiff controls |
| `objective_limit` | Early exit target |
| `project_bounds` | Bound projection toggle |
| `enable_restoration` | Recovery phase toggle |
| `enable_feasibility_phase` | Feasibility phase toggle |
| `system_structure` | Dense or sparse hint |
| `linear_system_solver` | Linear solve policy |
| `hessian_approximation` | Curvature policy |
| `problem_scaling` | Scaling and normalization policy |
| `progress_hook` | Iteration callback |

## Hessian Modes

Supported curvature policies:

- `Bfgs`
- `Identity`
- `Lbfgs`
- `Exact`
- `HessianVector`

`Exact` and `HessianVector` rely on autodiff-capable callbacks.

For `Exact`, the target object is the Hessian of the Lagrangian-like barrier model with respect to the primal variables. For `HessianVector`, the solver applies matrix-free curvature products inside the iterative subproblem path.

## Linear-System Policy

Supported policies:

- `Auto`
- `Cholesky`
- `PivotedDirect`
- `Inverse`
- `PseudoInverse`
- `ConjugateGradient`

For larger structured systems, `ConjugateGradient` is the main matrix-free path and feeds `krylov_iterations` in the final output.

## Barrier and Complementarity Trace

The most useful live fields are:

- `backend_parameter`
- `complementarity`
- `stationarity`
- `constraint_violation`
- `step_size`
- `krylov_iterations`

Those reveal whether the backend is:

- making central progress
- reducing barrier pressure
- stalling in complementarity
- stuck in feasibility or recovery

## Feasibility Phase and Recovery

When the main primal-dual loop stalls, the backend can switch into a feasibility-focused mode. The intent is analogous to minimizing a violation measure

```{math}
\min_x \; \|v(x)\|,
```

before returning to the main primal-dual trajectory.

The recovery path is separate from the feasibility phase:

- feasibility phase: reduce primal infeasibility
- recovery phase: regain a usable interior step when the barrier step itself fails

These show up in the output through:

- `stall_category`
- `detail_message`
- `best_feasible_available`
- `returned_best_feasible`

## Outputs

The most informative final output fields for `InternalProtocol` are:

| Output field | Meaning |
| --- | --- |
| `iterations` | Major primal-dual iterations |
| `function_evaluations` | Objective/constraint evaluation count |
| `constraint_violation` | Maximum primal infeasibility |
| `first_order_optimality` | Final stationarity measure |
| `backend_parameter` | Current barrier parameter $\mu$ |
| `complementarity` | Final complementarity residual |
| `krylov_iterations` | Krylov work from iterative subproblem solves |
| `best_feasible_available` | Whether a feasible checkpoint was found |
| `returned_best_feasible` | Whether that checkpoint was returned |
| `multiplier_max_norm` | Largest reported multiplier magnitude |
| `active_multiplier_count` | Active multiplier count |
| `detail_message` | Expanded termination detail |
| `stall_category` | Structured stop reason |

The final multiplier structure is split into:

- `linear_equalities`
- `nonlinear_equalities`
- `linear_inequalities`
- `nonlinear_inequalities`
- `lower_bounds`
- `upper_bounds`

## Example Trace

A typical progress line from an instrumented run looks like:

```text
[ip:iter] iter=35 evals=189 value=16.329592 viol=0.000000 stat=0.244045 compl=0.000000 barrier=0.010000 step=0.062500 krylov=0 stall=none
```

Interpretation:

- `value`: current objective
- `viol`: max constraint violation
- `stat`: first-order stationarity
- `compl`: complementarity residual
- `barrier`: current barrier parameter
- `step`: accepted step fraction
- `krylov`: inner iterative solver work

## Dedicated Demo

For an isolated walkthrough, use:

- `PlussMa/examples/Numerical/optimization/garch11_interior_point_demo.cpp`

That demo prints:

- the full option block
- every major iteration
- barrier parameter
- complementarity
- stall category
- final multiplier summary
