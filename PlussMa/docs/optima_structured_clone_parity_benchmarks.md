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
| BandedProfile | 16 | SequentialQP | MaxIterations | iteration-limit | 120 | 1619 | 0.002774 | 0.000000 | 0.000033 | 0.000000 | 0.006053 | 9092.993542 | yes |
| BandedProfile | 16 | InteriorPoint | Converged | none | 126 | 443 | 0.002774 | 0.000000 | 0.000000 | 0.000000 | 0.005960 | 3672.357500 | yes |
| BandedProfile | 32 | SequentialQP | Converged | none | 79 | 485 | 0.001356 | 0.000000 | 0.000001 | 0.000000 | 0.002896 | 6327.803875 | yes |
| BandedProfile | 32 | InteriorPoint | MaxIterations | iteration-limit | 200 | 509 | 0.001356 | 0.000001 | 0.000025 | 0.000000 | 0.002918 | 9904.350750 | yes |

## Assessment

- `SequentialQP` improved materially on this family after the structured routing and boundary-stall fixes.
- `SequentialQP` now reaches high-quality solutions on both sizes, but `n = 16` still needs too many evaluations and hits the iteration cap.
- `InteriorPoint` now converges on `n = 16` and keeps best-feasible reporting on both sizes.
- `InteriorPoint` is still accurate on this family, but it remains slow and still hits the iteration cap at `n = 32`.
- the new stall diagnostics show these remaining exits are generic iteration-limit stalls, not complementarity stalls or boundary stalls.

## Open Gaps Exposed By This Family

- `SequentialQP` still needs better iteration efficiency on the structured family.
- `InteriorPoint` needs better large-scale structure exploitation and/or better Krylov behavior.
- This family is the right benchmark to use while implementing sparse routing and stall taxonomy.
