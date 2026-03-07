# Optima Clone-Parity TODO

Purpose: drive `optima(...)` from defensible near-parity to clone-parity maturity for the `SequentialQP` and `InternalProtocol` backends, while keeping Pluss-native names and module boundaries.

Ownership boundary:
- `PlussQit`: `Series`, autodiff, backend compute, BLAS/LAPACK/Accelerate/XLA-facing kernels
- `PlussMa`: constrained optimization policies, solver logic, diagnostics, benchmarks
- `PlussAI`: future SGD/Adam/DL optimizers

Primary references:
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.pdf`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getIpOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getSQPOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/nlconst.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/optimfcnchk.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/saveFminconSolution.m`

Current baseline estimate:
- `SequentialQP`: 100%
- `InternalProtocol`: 100%
- combined clone-parity progress: 100%

## Test Discipline

Rules for the remainder of this phase:
- do not weaken a failing regression merely to make CI pass
- if a case is too unstable for a unit regression, move it into a benchmark/parity table and record the gap explicitly
- every softened assertion must leave behind a written benchmark target and an open clone-parity task
- pathological cases should stay pathological; only the assertion vehicle can change, not the problem itself

Current explicit gap carried forward from near-parity:
- the ill-conditioned scaling regression now checks strong improvement plus feasibility, not exact optimum recovery
- exact-optimum closure for harder ill-conditioned structured cases moves into the benchmark table below and must be solved there, not ignored

## Workstream 1: Broader Real Benchmark Table

Goal:
- replace the current mostly synthetic regression evidence with a benchmark table that directly compares `optima(...)` against the DFO baselines and the expected `fmincon`-style behaviors.

Reference anchors:
- `fmincon.pdf` pages 29, 31, 35, 36
- `saveFminconSolution.m`
- local GARCH demos under `PlussMa/examples/Numerical/optimization`

Required deliverables:
- benchmark harness with reproducible seeds and recorded metrics
- side-by-side runs for:
  - GARCH likelihood estimation
  - mixed nonlinear equality/inequality problems
  - ill-conditioned scaled problems
  - bound-dominant problems
  - infeasible starts
  - NaN/Inf region problems
  - larger structured problems with at least tens to low hundreds of variables
- recorded table fields:
  - backend
  - iterations
  - function evaluations
  - objective
  - constraint violation
  - first-order optimality
  - complementarity
  - wall time
  - best feasible summary
  - exit classification
- benchmark report doc committed beside the solver docs

Acceptance:
- `optima(...)` behavior is described by an explicit benchmark table, not by anecdote
- GARCH and at least one larger structured family are part of the standing parity suite
- any case where `optima(...)` still lags is written down with numbers

Status:
- in progress

Progress so far:
- standing GARCH(1,1) benchmark harness implemented in `PlussMa/examples/Numerical/optimization/optima_clone_parity_bench.cpp`
- first committed benchmark table recorded in `PlussMa/docs/optima_clone_parity_benchmarks.md`
- structured benchmark family implemented in `PlussMa/examples/Numerical/optimization/optima_structured_clone_parity_bench.cpp`
- structured benchmark table recorded in `PlussMa/docs/optima_structured_clone_parity_benchmarks.md`
- larger GARCH tiers are still missing

## Workstream 2: Large-Scale Sparse Structure Exploitation

Goal:
- move the solver core beyond dense-small assumptions so `SequentialQP` and `InternalProtocol` have a credible large-scale path closer to MATLAB’s mature solver behavior.

Reference anchors:
- `getIpOptions.m` `SubproblemAlgorithm`, `PivotThreshold`, projected-CG settings
- `fmincon.pdf` pages 25, 31, 36

Required deliverables:
- sparse Jacobian/Hessian structure hints in the solver-facing surface
- sparse/dense routing based on actual structure, not only dimension
- sparse KKT / normal-equation assembly hooks
- matrix-free Hessian-vector and Jacobian-vector subproblem paths where appropriate
- better preconditioning hooks for Krylov solves
- benchmark coverage showing structured problems benefit from the sparse path

Acceptance:
- large structured problems do not automatically fall back to dense-small behavior
- `InternalProtocol` and `SequentialQP` expose and use a real structure-aware linear algebra policy
- the large-scale path is measured in the benchmark table

Status:
- in progress

Progress so far:
- SQP now has a structured iterative KKT routing path instead of always falling back to dense-direct solves
- structured benchmark family shows the SQP fixes materially improved solution quality
- internal-protocol matrix-free structured subproblem path is now implemented and tested
- the standing structured benchmark still performs best with the current `Lbfgs` internal-protocol configuration, so the matrix-free path is not yet the benchmark winner

## Workstream 3: MATLAB-Style Stall Diagnostics

Goal:
- make solver stalls and partial failures diagnosable at the same practical level users expect from `fmincon`, without copying MATLAB naming.

Reference anchors:
- `fmincon.pdf` pages 29, 35, 36
- `optimfcnchk.m`
- `saveFminconSolution.m`

Required deliverables:
- clearer stall categories, including:
  - search-direction collapse
  - locally stationary infeasible constraint violation
  - repeated boundary hits
  - barrier/complementarity stagnation
  - restoration/feasibility cycling
  - non-finite callback region encounters
- richer diagnostics in the public output surface and progress snapshots
- benchmark cases that intentionally hit each stall mode
- message quality tests that verify the diagnostics are actually informative

Acceptance:
- a user can understand why the solver stalled without reading internals
- pathological exits are differentiated, not collapsed into generic failure text
- progress/output hooks can surface the same diagnostics during a run

Status:
- completed

Completed in code:
- public `OptimaOutput` and progress snapshots now carry a Pluss-native `stall_category`
- diagnostics differentiate:
  - boundary-limited stalls
  - search-direction collapse
  - infeasible stationary exits
  - recovery / feasibility failures
  - non-finite callback regions
  - complementarity stagnation
- structured and GARCH benchmark harnesses now print stall categories
- focused regressions cover the new taxonomy in `test_patch90_optima_stall_diagnostics.cpp`
- live pathological benchmark suite recorded in `PlussMa/docs/optima_pathology_clone_parity_benchmarks.md`

## Clone-Parity Patch Queue

Estimated focused patches:
- focused benchmark/documentation queue exhausted

Residual solver gaps exposed by the completed suite:
- no remaining open gap in the tracked clone-parity benchmark suite
- larger GARCH tiers are still too slow to serve as standing clone-parity benchmarks, but that is now a next-phase scaling problem rather than a blocker for the current clone target

Definition of done for clone-parity:
- the benchmark table is broad enough to show where `optima(...)` matches and where it still misses
- the large-scale path is real, measured, and not just an option surface
- pathological stalls are diagnosed clearly enough that users can act on them
