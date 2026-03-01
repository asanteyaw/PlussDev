# Optima Parity Matrix

Purpose: track the curated benchmark and pathological cases used to justify `optima(...)` near-parity claims for `SequentialQP` and `InteriorPoint`.

Reference anchors:
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.pdf`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getIpOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getSQPOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/nlconst.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/saveFminconSolution.m`

## Regression Matrix

| Case | Coverage | Backend expectation | Regression |
|---|---|---|---|
| Mixed nonlinear equality + inequality, infeasible start | feasibility recovery, multipliers, nonlinear Jacobians | `SequentialQP`, `InteriorPoint` should recover to a feasible point | `test_patch88_optima_parity_suite.cpp` |
| Active bounds + active nonlinear inequality | bound handling, complementarity, multiplier reporting | both backends should converge with active bound multipliers | `test_patch88_optima_parity_suite.cpp` |
| Ill-conditioned objective + equality scaling | scaling, normalization, stationarity under poor conditioning | both backends should solve with `ObjectiveAndConstraints` scaling | `test_patch88_optima_parity_suite.cpp` |
| Non-finite trial region | NaN/Inf recovery, restoration/recovery paths | both backends should shrink/recover instead of aborting | `test_patch72_optima_recovery.cpp` |
| Invalid callback contracts | callback validation and controlled exit | fail cleanly with `InvalidProblem` | `test_patch73_optima_validation.cpp` |
| Best feasible vs returned iterate | best-feasible tracking and public diagnostics | public output should preserve best-feasible summaries | `test_patch87_optima_output_parity.cpp` |
| Stall taxonomy and progress diagnostics | boundary stalls, recovery failures, complementarity stalls, non-finite regions | public output and progress snapshots should classify pathological exits consistently | `test_patch90_optima_stall_diagnostics.cpp` |
| Exact curvature / HVP interior-point | second-order curvature modes | `InteriorPoint` should support exact/HVP modes | `test_patch83_optima_exact_curvature.cpp` |
| Linear-system routing | direct, pivoted, and Krylov policy | solver should obey explicit routing policy | `test_patch85_optima_linear_policy.cpp`, `test_patch79_optima_cg.cpp` |
| GARCH likelihood benchmark | real objective recurrence and optimizer runtime behavior | compare `optima(...)` against DFO baselines | `PlussMa/examples/Numerical/optimization/garch11_optima_demo.cpp` |
| GARCH clone-parity standing slice | real workload table with wall time, best-feasible, and solver comparisons | `SequentialQP`, `InteriorPoint`, and DFO baselines should be compared on the same seeded likelihood surface | `PlussMa/examples/Numerical/optimization/optima_clone_parity_bench.cpp`, `PlussMa/docs/optima_clone_parity_benchmarks.md` |
| Structured banded family | larger structured problem with banded constraints and smooth chain objective | `SequentialQP` and `InteriorPoint` should be measured on tens-of-variables structured problems before sparse routing is added | `PlussMa/examples/Numerical/optimization/optima_structured_clone_parity_bench.cpp`, `PlussMa/docs/optima_structured_clone_parity_benchmarks.md` |
| Pathological live stall suite | real solver-triggered non-finite, boundary, collapse, recovery, and feasibility failures | public diagnostics should match the actual live exit mode | `PlussMa/examples/Numerical/optimization/optima_pathology_clone_parity_bench.cpp`, `PlussMa/docs/optima_pathology_clone_parity_benchmarks.md`, `test_patch91_optima_pathology_suite.cpp` |

## Remaining Clone-Parity Focus

- richer large-scale sparse structure exploitation
- broader real-world benchmark table with recorded metrics
- pathological benchmark suite tied to the new stall taxonomy
- current structured benchmark still exposes slow `InteriorPoint` convergence at `n = 32`
- `SequentialQP` still needs better iteration efficiency on the standing GARCH slice and the `n = 16` structured family
- the earlier premature SQP exits on the structured family were materially improved by the structured routing and boundary-stall fixes

## Regression Discipline

- hard cases stay hard; do not dilute the underlying problem just to make a regression pass
- if a case is too unstable for a unit-style exact assertion, move it into the benchmark/parity table and record the missing capability explicitly
- the ill-conditioned scaling case in `test_patch88_optima_parity_suite.cpp` currently validates strong normalized improvement plus feasibility
- exact-optimum closure for harder ill-conditioned structured cases is still an open clone-parity benchmark target, not a solved problem
