# Pattern Search Implementation Blueprint

## Scope
Add a native `pluss::subsp::math::optimization::pattern_search_minimizer(...)` solver family in `PlussMa/Math/Numerical/optimization` inspired by MATLAB `patternsearch.m` and private helpers, but with Pluss-native API/naming and staged delivery.

## MATLAB Reference Map
Primary files reviewed:
- `/Applications/MATLAB_R2025b.app/toolbox/globaloptim/globaloptim/patternsearch.m`
- `private/pfminunc.m`, `pfminbnd.m`, `pfminlcon.m`, `pfmincon.m`
- `private/poll.m`, `private/search.m`, `private/psnextfeasible.m`
- `private/getinitial.m`, `updateparam.m`, `isconverged.m`, `checkoptions.m`
- `+globaloptim/+patternsearch/psArgCheck.m`, `pscommon.m`
- `+globaloptim/+internal/+directions/*` (`PollPoints`, `gssdirections`, `lcondirections`, `ratioTest`, `findActiveSet`)
- nonlinear AL pieces: `psAugInit.m`, `psAugUpdate.m`, `psAugConverged.m`, `psAugReset.m`

Pluss mapping:
- MATLAB option struct + string selectors -> enum-first `PSConfig`
- MATLAB `Iterate/optimState` mutable structs -> internal typed C++ structs
- MATLAB `poll/search/update/isconverged` pipeline -> single solver loop with explicit phases
- MATLAB constrained feasible-only acceptance -> configurable constraint policy (`ExtremeBarrier` / `MeritPenalty`)

## Target Public Surface
New files:
- `pattern_search_minimizer.h`
- `pattern_search_minimizer.cpp`

Public entities:
- `PSConfig`
- `PSConstraintValues`
- `PSOutcome`
- `PSDiagnostics`
- `PSStatus`
- `pattern_search_minimizer(...)`

## Solver Architecture
1. Validate objective/initial/config/contracts.
2. Normalize to flat work-space vector while preserving original shape/platform for callback evaluation.
3. Build constraints model:
- bounds
- linear inequalities/equalities
- optional nonlinear callback
4. Evaluate incumbent (`f`, max-violation, merit).
5. Iteration loop:
- check convergence/budgets/time
- optional search step (expand along last successful direction)
- poll step (positive basis directions)
- accept step by merit decrease
- update mesh, penalty, success direction, deltas
6. Return best feasible point when available; otherwise incumbent.

## Phase Plan
### Phase 1 (implemented now)
- Poll patterns: `PositiveBasis2N`, `PositiveBasisNp1`
- Poll ordering: `Consecutive`, `Random`, `SuccessFirst`
- Search strategy: one-step expansion along last successful direction
- Constraint policies:
- `ExtremeBarrier`: skip objective for infeasible trial
- `MeritPenalty`: objective + penalty * max_violation
- Bounds, linear constraints, nonlinear constraints callback
- Iter/eval/time budgets, mesh expansion/contraction, tolerances
- Display + diagnostics

### Phase 2 (mostly implemented)
- Active-set-aware linear polling now implemented:
- tangent-space projection of poll/search directions using active linear constraints and active bounds
- ratio-tested feasible step sizing against linear inequalities and bounds
- linear feasibility restoration pass for infeasible starts
- mesh scaling mode added (`RangeLog`) with per-dimension direction scaling
- cache-aware trial reuse added (tolerance-based point cache + cache-hit diagnostics)
- GSS-augmented direction family implemented:
- active-constraint inward directions + equality-preserving projection
- blended augmented poll directions around active sets
- basis cycling implemented:
- canonical basis or random orthonormal basis with configurable refresh interval
- dynamic scaling parity implemented:
- `NupsDynamic` nonuniform per-axis scaling with success/failure adaptation and diagnostics
- search-stage family expanded:
- `RandomDirections` and `ExpandAndRandomDirections` modes with configurable trial count and diagnostics

### Phase 3
- Augmented-Lagrangian outer loop for nonlinear constrained parity (implemented baseline):
- warm-started outer iterations around inner pattern-search solves
- nonlinear multiplier updates (eq/ineq) with adaptive penalty growth
- best-feasible selection across outer iterations
- AL hardening patch (implemented):
- global iteration/evaluation/time budgeting enforced across outer iterations
- stronger outer-stop checks (primal feasibility + step/value/dual stabilization)
- outer-loop budget/status propagation (`ReachedEvaluationBudget`/`ReachedTimeBudget`/etc.)
- Subproblem solve strategy split (unconstrained/bounds/linear) with tightened inner tolerances
- richer exit flags/messages aligned with constrained/nonlinear regimes
- DFO integration closure (implemented):
- full status passthrough for `PatternSearch` (`ReachedTimeBudget`, `InfeasibleStationary`)
- richer diagnostics passthrough (`max_constraint_violation`, `outer_iterations`, cache/search/NUPS counters)
- dedicated Catch2 closure suite for constrained + infeasible + time-budget regimes

## File/Build Integration Tasks
1. Add header include export in `PlussMa/include/pluss/ma/math/optimization.h`.
2. Add demos under `PlussMa/Math/examples/Numerical/optimization/`.
3. Add regression tests under `PlussMa/Math/Numerical/optimization/test/`.
4. `PS` route added to `dfo_solver` as explicit solver kind (`DfoSolverKind::PatternSearch` + `DfoOptions::pattern_search`).
5. `DFO` now supports equality-aware nonlinear adapter for `PatternSearch` via `DfoConstraintValues` (`inequality` + `equality`).

## Acceptance Criteria (Phase 1)
- Compiles cleanly with current tree.
- Works on unconstrained, bound-constrained, linear-constraint, and nonlinear-callback constrained toy problems.
- Returns deterministic results with fixed seed.
- Exposes diagnostics for iterations, evaluations, mesh, violation, and last step.
- Adds at least one demo and one automated test.
