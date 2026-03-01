# Optima Structured Clone-Parity Benchmarks

Purpose: record a larger structured benchmark family for `optima(...)` clone-parity work, beyond the standing GARCH slice.

Benchmark executable:
- `plussma_optima_structured_clone_parity_bench`

Family:
- `BandedProfile`

Structure:
- scalar parameter blocks `x_0 ... x_{n-1}`
- banded linear equality and inequality constraints
- box bounds
- smooth chain-coupled autodiff objective
- infeasible alternating start

Why this family matters:
- it is larger than the current GARCH standing slice
- it already exposes the difference between “finds a feasible point fast” and “solves the structured problem well”
- it is the right baseline family for the upcoming sparse-routing work

## Results

| Family | n | Solver | Exit | Stall | Iter | Eval | Objective | Constr viol | First-order | Complementarity | Max \|x-target\| | Time ms | Best feasible |
|---|---:|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| BandedProfile | 16 | SequentialQP | Converged | none | 14 | 15 | 0.002778 | 0.000000 | 0.000323 | 0.000000 | 0.006869 | 211.358667 | yes |
| BandedProfile | 16 | InteriorPoint | Converged | none | 126 | 443 | 0.002774 | 0.000000 | 0.000000 | 0.000000 | 0.005960 | 3533.626042 | yes |
| BandedProfile | 32 | SequentialQP | Converged | none | 30 | 53 | 0.001375 | 0.000000 | 0.000267 | 0.000000 | 0.004377 | 1130.025833 | yes |
| BandedProfile | 32 | InteriorPoint | Converged | none | 352 | 1675 | 0.001356 | 0.000000 | 0.000000 | 0.000000 | 0.002891 | 24866.150750 | yes |

## Assessment

- `SequentialQP` improved materially on this family after the structured routing and boundary-stall fixes.
- `SequentialQP` now converges on both sizes under the current benchmark tolerances.
- the SQP parity gap on `n = 16` was closed by adding a mature feasible-point stop based on tiny predicted improvement, instead of forcing repeated nearly-null steps.
- `InteriorPoint` now converges on `n = 16` and keeps best-feasible reporting on both sizes.
- `InteriorPoint` now converges on `n = 32` as well under the MATLAB-scale benchmark budget.
- the new stall diagnostics show these remaining exits are generic iteration-limit stalls, not complementarity stalls or boundary stalls.

## Open Gaps Exposed By This Family

- This family is the right benchmark to use while implementing sparse routing and stall taxonomy.
