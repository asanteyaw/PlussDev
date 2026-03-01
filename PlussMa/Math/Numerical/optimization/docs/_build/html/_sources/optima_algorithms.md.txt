# Optima Algorithms

`Optima` is the constrained optimization layer built in `PlussMa` on top of the `Series` and autodiff runtime from `PlussQit`.

This split is intentional:

- `PlussQit` owns `Series`, autodiff transforms, and backend compute
- `PlussMa` owns numerical optimization logic and solver policy
- `optima(...)` is the public constrained-solver entry point

## Public Entry Point

The top-level call takes a constrained problem, a `ParameterPack<Series>` initial point, and an `OptimaOptions` value:

```cpp
using namespace pluss::subsp::math::optim;

ConstrainedProblem problem;
ParameterPack<Series> x0;
OptimaOptions options;

options.algorithm = OptimaAlgorithm::SequentialQP;
const auto result = optima(problem, x0, options);
```

## Problem Model

`Optima` works with:

- a scalar objective
- nonlinear equality constraints
- nonlinear inequality constraints
- linear equality constraints
- linear inequality constraints
- lower and upper bounds

The public parameter surface is always `ParameterPack<Series>`. Derivatives come from either:

- autodiff callbacks over `ParameterPack<Trace>`
- user-supplied analytic callbacks

Finite differences are not the primary derivative path in `Optima`.

## Shared Runtime Contract

All current backends reuse the same evaluation layer and diagnostics:

- callback validation and shape checks
- non-finite evaluation detection
- best-feasible tracking
- multiplier reporting
- progress hooks
- stall classification
- scaling and normalization
- dense and iterative linear-system policies

The mature constrained backends currently exposed through `OptimaAlgorithm` are:

- `SequentialQP`
- `InternalProtocol`

This document focuses on:

- [Sequential QP](optima_sequential_qp.md)
- [Internal Protocol](optima_internal_protocol.md)

## Common Outputs

Every `optima(...)` result exposes:

- final parameters
- final evaluation bundle
- iteration count
- function evaluation count
- backend parameter
- stationarity metric
- exit flag
- message
- `OptimaOutput`
- `ConstraintMultipliers`
- optional best-feasible snapshot

The `OptimaOutput` payload is the main reporting surface:

| Field | Meaning |
| --- | --- |
| `algorithm` | Backend label |
| `iterations` | Major iterations |
| `function_evaluations` | Total evaluation count |
| `constraint_violation` | Max violation at the returned point |
| `first_order_optimality` | First-order stationarity metric |
| `step_size` | Final accepted step fraction |
| `line_search_fraction` | Accepted line-search scaling |
| `backend_parameter` | Penalty or barrier parameter |
| `complementarity` | Complementarity residual where applicable |
| `krylov_iterations` | Iterative linear-solver work |
| `best_feasible_available` | Whether a feasible checkpoint was found |
| `returned_best_feasible` | Whether the solver returned that checkpoint |
| `best_feasible_value` | Objective at the best feasible checkpoint |
| `best_feasible_constraint_violation` | Violation at the best feasible checkpoint |
| `best_feasible_first_order_optimality` | Stationarity at the best feasible checkpoint |
| `multiplier_max_norm` | Largest multiplier magnitude |
| `active_multiplier_count` | Count of active multipliers |
| `detail_message` | Expanded termination message |
| `stall_category` | Structured stall classification |
| `stall_measure` | Diagnostic measure associated with the stall |
| `stall_reference` | Reference threshold associated with the stall |

## Progress Hooks

Each backend option block accepts a `progress_hook`:

```cpp
options.sequential_qp.progress_hook = [](const OptimaProgressSnapshot& s) {
  return false;
};
```

The snapshot includes:

- `stage`
- `algorithm`
- `params`
- `evaluation`
- `iteration`
- `function_evaluations`
- `step_size`
- `backend_parameter`
- `stationarity`
- `complementarity`
- `krylov_iterations`
- `message`
- `stall_category`
- `stall_measure`
- `stall_reference`
- `best_feasible`

Returning `true` requests an orderly stop.

## Stall Taxonomy

`Optima` surfaces structured failure modes instead of only generic stop messages:

- `none`
- `user-stop`
- `invalid-callback-contract`
- `nonfinite-callback-region`
- `search-direction-collapse`
- `boundary-limited`
- `infeasible-stationary`
- `complementarity-stagnation`
- `feasibility-failure`
- `recovery-failure`
- `iteration-limit`

## Example Demos

Current demos relevant to these backends:

- `PlussMa/examples/Numerical/optimization/optima_sequential_qp_demo.cpp`
- `PlussMa/examples/Numerical/optimization/optima_interior_point_demo.cpp`
- `PlussMa/examples/Numerical/optimization/garch11_optima_demo.cpp`
- `PlussMa/examples/Numerical/optimization/garch11_interior_point_demo.cpp`
