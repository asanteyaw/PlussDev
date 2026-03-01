# Optima Clone-Parity Benchmarks

Purpose: record reproducible benchmark evidence for `optima(...)` clone-parity work, starting with a standing GARCH(1,1) benchmark slice and growing toward larger structured workloads.

Primary references:
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/fmincon.pdf`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getIpOptions.m`
- `/Users/yawasante/Documents/Doctrate/Thesis/C++/second_paper/with_returns/DFO/getSQPOptions.m`
- local demos under `PlussMa/examples/Numerical/optimization`

## GARCH(1,1) Standing Slice

Benchmark executable:
- `plussma_optima_clone_parity_bench`

Configuration:
- simulated observations: `40`
- truth:
  - `omega = 0.02`
  - `alpha = 0.08`
  - `beta = 0.90`
  - persistence `= 0.98`
- same likelihood surface for:
  - `SequentialQP`
  - `InteriorPoint`
  - `NelderMead`
  - `DE_PRMM`

Why this is the standing slice:
- it completes quickly enough to be a routine clone-parity benchmark
- larger GARCH tiers are still too slow for a standing run on the current implementation
- that performance gap is an open clone-parity issue, not a solved problem

### Results

| Solver | Family | Exit | Stall | Iter | Eval | Objective | Constr viol | First-order | Complementarity | Best feasible | Best feas viol | Time ms | Persistence | \|omega err\| | \|alpha err\| | \|beta err\| |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| SequentialQP | Optima | MaxIterations | iteration-limit | 50 | 160 | 13.763152 | 0.000000 | 0.000016 | 0.000000 | 13.763152 | 0.000000 | 1698.915167 | 0.953954 | 0.013711 | 0.079999 | 0.053953 |
| InteriorPoint | Optima | Converged | none | 46 | 50 | 13.763152 | 0.000000 | 0.000001 | 0.000000 | 13.763152 | 0.000000 | 531.767375 | 0.953847 | 0.013789 | 0.079999 | 0.053846 |
| NelderMead | DFO | Converged | n/a | 433 | 799 | 13.763136 | 0.000000 | 0.000000 | 0.000000 | n/a | n/a | 2183.660583 | 0.430625 | 0.396838 | 0.080000 | 0.469375 |
| DE_PRMM | DFO | Converged | n/a | 30 | 1068 | 13.763137 | 0.000000 | 0.000000 | 0.000000 | n/a | n/a | 2855.994500 | 0.652633 | 0.234286 | 0.080000 | 0.247367 |

### Assessment

- `InteriorPoint` is currently the strongest `optima(...)` backend on this standing GARCH slice.
- `SequentialQP` reaches the same objective level here, but it hits the iteration cap.
- the new stall diagnostics classify that `SequentialQP` exit as an iteration-limit stall, not a feasibility or boundary failure.
- Both `optima(...)` backends materially outperform the DFO baselines in parameter recovery on this benchmark slice.
- The DFO baselines match the objective value closely, but land on much worse parameter estimates, which suggests this likelihood surface is not strongly identifying parameters at this sample size.

### Open Gaps

- `SequentialQP` should not be hitting its iteration cap on this standing slice.
- GARCH tiers above `n = 40` are still too slow to serve as standing parity benchmarks.
- The benchmark table currently covers only the first real workload family; larger structured problems still need to be added.
