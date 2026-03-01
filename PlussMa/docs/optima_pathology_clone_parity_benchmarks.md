# Optima Pathology Clone-Parity Benchmarks

Purpose: record live `optima(...)` runs that intentionally trigger pathological exits, so stall diagnostics are justified by real solver behavior instead of synthetic state injection alone.

Benchmark executable:
- `plussma_optima_pathology_clone_parity_bench`

Primary references:
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.pdf`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/optimfcnchk.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/saveFminconSolution.m`

## Results

| Case | Solver | Exit | Stall | EvalStatus | Iter | Eval | Objective | Violation | First-order | Complementarity | Step |
|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| NonFiniteInitial | SequentialQP | NonFiniteEvaluation | nonfinite-callback-region | 1 | 0 | 1 | nan | 0.0 | inf | 0.0 | 0.0 |
| NonFiniteInitial | InteriorPoint | NonFiniteEvaluation | nonfinite-callback-region | 1 | 0 | 2 | nan | 0.0 | inf | inf | 0.0 |
| BoundaryLimited | SequentialQP | NoProgress | boundary-limited | 0 | 0 | 1 | 1.0 | 0.0 | 2.0 | 0.0 | 0.0 |
| SearchDirectionCollapse | SequentialQP | NoProgress | search-direction-collapse | 0 | 0 | 1 | 1.0 | 0.0 | 2.0 | 0.0 | 0.0 |
| RecoveryFailure | SequentialQP | NoProgress | recovery-failure | 0 | 0 | 1 | 0.5625 | 0.8 | 1.5 | 0.0 | 0.0 |
| FeasibilityFailure | InteriorPoint | NoProgress | feasibility-failure | 0 | 0 | 4 | 0.0 | 0.0 | 99.8999 | 0.0 | 0.0 |
| ComplementarityProbe | InteriorPoint | MaxIterations | complementarity-stagnation | 0 | 1 | 4 | 1.65832 | 0.0 | 1.18433 | 0.0268761 | 0.0522248 |

## Assessment

- the public stall taxonomy is now backed by live solver runs for:
  - non-finite callback regions
  - boundary-limited exits
  - search-direction collapse
  - SQP recovery failure
  - interior-point feasibility failure
  - interior-point complementarity stagnation
- this closes the benchmark side of the diagnostics workstream.
