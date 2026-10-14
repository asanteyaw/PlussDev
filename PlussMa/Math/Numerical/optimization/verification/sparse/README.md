# Sparse industry-scale LP / MILP stack — verification record

Code: `include/pluss/subsp/math/lp/` (namespace `pluss::subsp::math::lp`; alias it locally, `namespace lp = pluss::subsp::math::lp;`).

| Header | Role |
| --- | --- |
| `sparse.h` | `Model` (CSC matrix, ranged rows, integer flags), `SolveStatus` |
| `lu.h`, `simplex.h` | sparse LU (threshold pivoting, eta updates); dual simplex (steepest edge, bound flipping, Harris, perturbation) + Devex primal; warm start, snapshots, tableau rows |
| `presolve.h`, `solve.h` | presolve (singleton/forcing/redundant rows, dual fixing, integer bound strengthening, coefficient tightening) and the `lp::solve` facade; postsolve warm-starts the simplex on the original model so duals and bases are exact |
| `chol.h`, `ipm.h` | sparse Cholesky (minimum degree), homogeneous self-dual interior point with crossover |
| `mip.h`, `mip_cuts.h` | branch-and-cut: pseudocost/strong branching, Gomory MI, c-MIR, lifted covers, dives, RENS/RINS, reduced-cost fixing |
| `mps.h` | MPS reader (fixed and free format, RANGES, bound types, integer markers) |
| `series_bridge.h` | `linprog::Problem` / `milp::Problem` (Series) -> sparse model; `lp::solve_sparse` |

Use this stack for large sparse models; the dense batched solvers (`linprog.h`, `milp.h`) remain the right tool for many small related LPs.

## What is verified, and how

* **Permanent test** `test/lp_sparse_test.cpp` (CTest `lp_sparse_test`): the HiGHS reference LPs through the simplex (primal feasibility, objective, an independent dual
  certificate rebuilt from the original data, Farkas certificate for infeasible, ray for unbounded) and the interior point; the HiGHS reference MILPs through branch-and-cut;
  400 random binary programs against exhaustive enumeration, each under three option sets (presolve on/off, cuts on/off, heuristics on/off); presolve regression models;
  warm-start re-solves against cold solves; the MPS reader; the Series bridge. A mutation check (reinstating the presolve bug below) makes it fail.
* **Differential campaigns** (not committed, they need SciPy/HiGHS): 30 random seeds x ~120 MILPs vs HiGHS with the MIP gap set to 0, objective/feasibility/integrality re-checked
  in numpy; NETLIB 88/88 LPs against HiGHS (objective to 1e-7 relative); MIPLIB 3/2003 subset against HiGHS reference runs.
* **Soundness fixes this verification found** (each has a regression in the test above):
  1. presolve turned an infeasible integer model into a "solved" empty model: singleton-row tightening created crossing bounds and dual fixing overwrote them;
  2. coefficient tightening on integer columns with a nonzero lower bound and a negative coefficient used the wrong right-hand-side shift, and used a tighter implied bound than the column's own -
     `rentacar` (MIPLIB) was reported infeasible;
  3. the interior point reported "unbounded" for infeasible models that also have a cost-improving recession ray; `solve_ipm` now decides with a zero-objective feasibility solve;
  4. earlier: stale marks in the LU workspace, false unbounded from negative Harris steps, dual cycling, node pruning that silently dropped nodes, unit-ordering of the iteration budget across warm-started solves.

## Known limits (honest)

* FTRAN/BTRAN are hypersparse (reach-based) and the dual simplex updates use index lists, but pricing is not partial and large degenerate LPs (cre-b, osa-60, ken-18) are still 5-15x slower than HiGHS; the interior point has no supernodal Cholesky.
* Branch-and-cut has clique cuts, node propagation and root probing but no conflict analysis: node counts are above HiGHS on several MIPLIB instances (see the plan document).
* CPU only, single threaded.
