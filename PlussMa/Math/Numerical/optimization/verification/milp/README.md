# milp verification

`include/pluss/subsp/math/milp.h` is a branch-and-bound mixed-integer LP solver on top of `linprog.h` (batched interior point or
warm-started dual simplex). Namespace: `pluss::subsp::math::milp` (users alias it locally; there is no `pluss::` root alias).

## What is verified, independently of the solver
* **Differential campaign** (10 seeds x 122 problems x 2 LP backends = 2,440 solves, 0 failures): knapsack, general integer with
  equalities, mixed integer/continuous, set cover, assignment, parity-infeasible, LP-relaxation-infeasible, unbounded relaxation,
  tiny binary. Oracle: `scipy.optimize.milp` (HiGHS, presolve off, `mip_rel_gap = 0`). Every returned point is re-checked in numpy for
  feasibility, integrality and `c^T x = objective`.
* **Exhaustive enumeration** for the tiny binary problems (HiGHS is itself checked against brute force in the reference generator).
* **Permanent test** `test/milp_test.cpp` (52 problems, both backends) with the same checks in C++, plus a mutation check
  (integer tolerance 0.45 makes it fail with "inequality violated").

## Findings
* HiGHS' default `mip_rel_gap = 1e-4` made it stop 4e-4 short of the optimum on one mixed problem; the oracle now uses gap 0.
* A bound-only LP (no constraint rows) crashed `linprog`; it now gets an inert row.
* **Soundness**: a node LP that the interior-point method fails to finish must never be pruned as infeasible. Such nodes are
  re-solved exactly with the simplex before use (found when an all-zero constraint column made the scaling overflow).
* With `lp.method = Simplex` every node is warm-started from its parent's optimal basis (a branching bound change is a dual-simplex repair).

## Limits
Integer variables need finite bounds; no cuts, presolve or strong branching (big-M models such as the loop-law MILP are slow:
~45-70 s per reaction range on e_coli_core); `NodeLimit` returns the incumbent and bound.
Regenerate: `generate_reference.py <out.h>` (numpy, scipy).
