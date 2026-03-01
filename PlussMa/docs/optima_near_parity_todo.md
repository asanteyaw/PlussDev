# Optima Near-Parity TODO

Purpose: drive `optima(...)` to defensible near-parity with MATLAB `fmincon` for the `SequentialQP` and `InteriorPoint` backends while keeping Pluss-native naming and architecture.

Ownership boundary:
- `PlussQit`: `Series`, autodiff, backend compute, linear algebra kernels
- `PlussMa`: optimization logic, benchmarks, solver policies, outputs
- `PlussAI`: future ML/DL first-order optimizers

Reference sources used for this plan:
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.pdf`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getIpOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getSQPOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/nlconst.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/optimfcnchk.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/saveFminconSolution.m`

Current rough maturity estimate:
- `SequentialQP`: 84%
- `InteriorPoint`: 89%
- combined near-parity progress: 87%

## Completed So Far

- `optima(...)` public entry point in `PlussMa`
- strict callback validation and derivative audits
- best-feasible tracking and richer outputs
- SQP restoration fallback and non-finite recovery
- interior-point restoration fallback and non-finite recovery
- scaling/preconditioning from initial-point magnitudes
- interior-point CG subproblem path with Krylov reporting
- interior-point L-BFGS Hessian approximation
- GARCH benchmark harness beside the DFO demos

## Workstream 1: Primal-Dual Interior-Point

Goal:
- replace the current barrier-gradient-only step logic with a primal-dual system using explicit slack and complementarity residuals

Reference anchors:
- `fmincon.pdf` pages 31, 35, 36
- `getIpOptions.m`
- `barrier.p` is opaque, so infer behavior from options/docs/call sites

Required deliverables:
- explicit slack representation for all inequality classes
- complementarity residual tracking
- primal, dual, and centrality residual norms in solver state
- primal-dual Newton system assembly
- fraction-to-boundary logic for primal and dual updates
- monotone barrier update policy and centrality control

Acceptance:
- `InteriorPoint` output includes meaningful complementarity/stationarity measures
- inequality handling no longer relies only on the current log-barrier surrogate

Status:
- completed

## Workstream 2: Feasibility Mode

Goal:
- add a dedicated feasibility-search mode for interior-point aligned with the behavior described for `EnableFeasibilityMode`

Reference anchors:
- `fmincon.pdf` page 35
- `getIpOptions.m`

Required deliverables:
- explicit option surface with Pluss-native naming
- separate restoration/feasibility objective
- transition rules between feasibility mode and main mode
- benchmark coverage for infeasible starts and hard constraint recovery

Acceptance:
- hard infeasible-start problems recover more reliably than the current fallback-only behavior

Status:
- completed

## Workstream 3: Exact Hessian / Hessian-Vector

Goal:
- allow interior-point to use exact Hessian or Hessian-vector products from autodiff rather than only quasi-Newton approximations

Reference anchors:
- `fmincon.pdf` page 31
- `getIpOptions.m` Hessian/HessMult handling

Required deliverables:
- Pluss-native exact-Hessian callback surface
- autodiff Hessian-of-Lagrangian assembly where possible
- Hessian-vector path using `hvp(...)`
- compatibility rules between Hessian mode and subproblem solver mode

Acceptance:
- interior-point can run in exact-Hessian and Hessian-vector modes under `Series` inputs only

Status:
- completed

## Workstream 4: Stronger SQP

Goal:
- bring SQP closer to the `nlconst.m` behavior on Hessian updates, step acceptance, and constraint handling

Reference anchors:
- `nlconst.m`
- `getSQPOptions.m`
- `fmincon.pdf` pages 35 and 36

Required deliverables:
- second-order correction step
- stricter bound-feasibility maintenance
- more mature merit/acceptance logic
- stronger active-set and multiplier filtering
- safer BFGS update safeguards following the `YL'*sdiff` logic

Acceptance:
- SQP behavior on constraint-dominant problems is materially more robust than the current prototype

Status:
- completed

## Workstream 5: Linear Algebra Policy

Goal:
- move from a mostly dense-small linear algebra policy toward a more defensible solver policy split

Reference anchors:
- `getIpOptions.m` SubproblemAlgorithm, PivotThreshold
- `fmincon.pdf` pages 25 and 36

Required deliverables:
- separate direct/factorization and Krylov routes
- pivot/factorization controls
- sparse/dense routing hooks
- subproblem iteration reporting
- exact/HVP compatibility with CG path

Acceptance:
- linear-system policy is explicit and benchmarkable instead of implicit

Status:
- completed

## Workstream 6: Scaling and Normalization

Goal:
- move from initial-point scaling only to fuller objective/constraint normalization

Reference anchors:
- `getIpOptions.m`
- `getSQPOptions.m`
- `fmincon.pdf` page 25

Required deliverables:
- objective scaling
- constraint row normalization
- linear and nonlinear constraint scaling
- better scaling diagnostics in outputs

Acceptance:
- ill-scaled problems improve under normalization without manual user retuning

Status:
- completed

## Workstream 7: Outputs / Multipliers / Messages

Goal:
- close the output semantics gap while keeping Pluss-native names

Reference anchors:
- `fmincon.pdf` page 29
- `saveFminconSolution.m`

Required deliverables:
- richer step and line-search reporting
- better multiplier quality and decomposition
- stronger exit classification
- best-feasible vs final-point distinction in outputs and messages

Acceptance:
- user-facing diagnostics are good enough for solver triage without reading internals

Status:
- in progress

## Workstream 8: Parity Benchmarks and Pathological Cases

Goal:
- prove behavior on curated constrained benchmarks, not just toy examples

Reference anchors:
- `fmincon.pdf` behavior descriptions
- local GARCH example
- local MATLAB source behaviors around infeasible handling and best-feasible tracking

Required deliverables:
- ill-conditioned problems
- infeasible starts
- NaN/Inf regions
- bound-active problems
- nonlinear equality/inequality cases
- GARCH likelihood benchmark comparisons

Acceptance:
- near-parity claim is backed by a benchmark table and regression suite

Status:
- in progress

## Patch Queue

Estimated remaining focused patches to near-parity:
1. richer outputs/messages/multipliers
2. parity benchmarks and hardening

Important note:
- these are not cosmetic patches
- the last 20% is mostly robustness and pathological-case handling
- after near-parity, a separate phase can pursue clone-level behavioral matching if needed
