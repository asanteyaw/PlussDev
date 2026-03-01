# Optima Interior Point

`InteriorPoint` is the primal-dual barrier backend for `optima(...)`.

It is the main constrained solver when you want barrier dynamics, complementarity tracking, and a more global constrained treatment than a pure working-set style loop.

## Solver Shape

The backend uses a primal-dual interior formulation with:

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

## Main State

The interior-point state tracks:

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

## Option Surface

`InteriorPointOptions` exposes:

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

Interior point supports richer curvature policies than SQP:

- `Bfgs`
- `Identity`
- `Lbfgs`
- `Exact`
- `HessianVector`

`Exact` and `HessianVector` rely on autodiff-capable callbacks. They are intended for problems where second-order information is worth the additional cost.

## Linear-System Policy

The same public linear-solver enum is available here, but it matters more because the barrier system can become expensive:

- `Auto`
- `Cholesky`
- `PivotedDirect`
- `Inverse`
- `PseudoInverse`
- `ConjugateGradient`

For larger structured systems, the iterative route is often the more informative path because `krylov_iterations` becomes part of the output trace.

## Complementarity and Barrier Progress

The backend surfaces:

- `backend_parameter` for the barrier parameter
- `complementarity` for the complementarity residual

Those two fields are the quickest way to see whether the solver is:

- staying central
- reducing barrier pressure
- stalling in the complementarity system

## Feasibility and Recovery Modes

`InteriorPoint` has two distinct fallback mechanisms:

### Feasibility Phase

When the main barrier loop stalls in a way that suggests feasibility is the main issue, the backend can temporarily switch to a violation-reduction mode.

### Restoration

When a step cannot produce a valid improvement in the barrier model, the backend can enter a restoration process intended to regain a useful interior point.

The public stall taxonomy distinguishes these cases:

- `feasibility-failure`
- `recovery-failure`
- `complementarity-stagnation`

## Reading the Iteration Trace

For this backend, the most useful per-iteration fields are:

- objective value
- max violation
- stationarity
- complementarity
- barrier parameter
- step size
- Krylov iterations
- stall category

Example:

```cpp
options.algorithm = OptimaAlgorithm::InteriorPoint;
options.interior_point.progress_hook = [](const OptimaProgressSnapshot& s) {
  std::cout << s.iteration
            << " f=" << s.evaluation.value.item<double>()
            << " viol=" << s.evaluation.max_constraint_violation
            << " stat=" << s.stationarity
            << " compl=" << s.complementarity
            << " mu=" << s.backend_parameter
            << " step=" << s.step_size
            << " krylov=" << s.krylov_iterations
            << '\n';
  return false;
};
```

## Dedicated GARCH Demo

If you want to inspect this backend in isolation, use:

- `PlussMa/examples/Numerical/optimization/garch11_interior_point_demo.cpp`

That demo prints:

- the full `InteriorPointOptions` block
- every major iteration
- final exit/message details
- final complementarity and barrier parameter
- multiplier summaries

It is intentionally more verbose than the combined `optima(...)` demos so solver behavior is visible directly.

