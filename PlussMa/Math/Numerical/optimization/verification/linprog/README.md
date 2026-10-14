# linprog verification

`include/pluss/subsp/math/linprog.h` is a batched homogeneous self-dual interior-point LP solver (shared by FBA and the
planned integer-programming layer). Correctness is established three ways that do not rely on the solver agreeing with itself.

1. **Independent oracles.** SciPy `linprog` (HiGHS) on random problem families: feasible (box / free variables / negative
   lower bounds / no upper bounds), infeasible, infeasible with a redundant equality row, unbounded, degenerate (dependent
   equality rows, duplicated tight inequalities) and batched (one call, 150-200 lanes sharing the matrices).
   NETLIB afiro, sc50a, sc50b, sc105, kb2, adlittle, blend, stocfor1, scagr7, share2b, israel were run during development
   (all match HiGHS to <=1e-9 relative; published optima matched except that scagr7's value I recalled was wrong - HiGHS and
   this solver agree with each other to 1e-10); afiro, sc50a, sc50b, kb2, adlittle, blend, share2b are embedded in the test.
2. **Certificates checked against the original data, in C++** (`test/linprog_test.cpp`), not trusting any solver:
   optimal -> primal feasibility + duality gap (objective vs dual objective) + objective vs oracle;
   infeasible -> Farkas ray in the original constraint space (b^T y > 0, column conditions); unbounded -> recession ray.
3. **Mutation check.** Loosening the solver tolerance to 1e-2 makes the test fail ("primal infeasibility 0.0195"), so the
   checks can fail.

A 30-seed differential campaign (about 33,000 problems, seeds 0-29, same families, certificates verified in numpy) had
0 failures (re-run after the implicit-bound / Cholesky rewrite: 20 seeds, 0 failures); the permanent test embeds a fixed-seed subset (373 problems: 348 optimal + 4 NearOptimal-certified, 10 infeasible, 11 unbounded).

## Solver design notes (what the verification forced)
* Finite upper bounds are handled **implicitly** (second complementarity pair), so the normal equations stay the size of the
  equality block. The first version added one explicit row per bound and was ~3x slower on e_coli_core.
* The normal equations are factorised with **Cholesky** (an explicit batched inverse loses ~1e-4 on the ill-conditioned matrices
  of late iterations) with a retry that grows the diagonal shift if rounding makes a lane indefinite.
* Lanes that stall near the end are **frozen at their best iterate** (`stall_patience`, only once the merit is < 1e-4): they
  report `Status::NearOptimal` when the best merit is < `accept_tolerance` (1e-6) and are otherwise `IterationLimit`.
  In the 20-seed campaign ~3.5% of lanes ended NearOptimal; their worst objective / feasibility / gap errors were 2.3e-8 /
  4.9e-8 / 3.9e-8 (still certified by the checks above). `linprog::success(status)` accepts Optimal and NearOptimal.
* Finished lanes are frozen with `where`, never by multiplying a step by 0 (NaN * 0 = NaN).
* The Farkas multipliers are returned in the ORIGINAL row space `(y_eq, y_ub)`; the certificate is checked against the box
  bounds directly, independent of the solver's internal standard form.

## Simplex backend, warm start and scaling (added later)
* `Options::method = Simplex`: bounded-variable revised simplex (phase I with artificials, Harris two-pass ratio test, Bland fallback,
  dense basis inverse refactored every 60 pivots; matrix work in Series ops, pivot selection on the host). Returns EXACT vertices
  (constraint violations ~1e-12 instead of ~1e-7), Farkas multipliers from the phase-I duals and recession rays. One lane at a time.
* **Warm start**: `Problem::warm_basis / warm_at_upper` (from `Result::basis / at_upper`) skip phase I; bound/rhs changes are repaired
  by dual simplex pivots, cost changes by primal pivots; an unusable basis silently falls back to a cold start.
  Differential test vs HiGHS: 11 seeds x 320 variants, 0 failures, ~10x fewer pivots (e.g. 784 vs 7,939).
* **Ruiz scaling** (rows/columns driven to unit max-norm) was needed for big-M / stoichiometric matrices; empty rows and columns must keep scale 1
  (an all-zero column overflowed to 1e150 and produced NaNs - found by the MILP campaign).
* **Sparse assembly** of `A D A^T` from the non-zeros when the matrix is sparser than `sparse_density` (m >= 128).
* **Lane chunking** (`Options::max_lanes`) bounds the `[B, m, m]` workspace.
* Why the simplex exists: with a pinned objective floor (FVA at fraction 1.0, loop-law MILP relaxations) the feasible set has almost no
  interior and interior-point accuracy collapses (genome-scale FBA: NearOptimal with a 0.46% objective error); the simplex solves these exactly.
* Engine footgun found on the way: `Series::where` takes `cond.where(a, b)` only when the receiver is **Bool**; a float 0/1 receiver is read as the
  legacy `value.where(cond, other)` and silently returns garbage.

## Findings during verification
* HiGHS presolve (through SciPy) reports some feasible-but-unbounded LPs as *infeasible*; the reference status is therefore
  decided by a zero-objective feasibility solve plus a presolve-off rerun.
* A recession ray proves only dual infeasibility; the solver confirms unboundedness with a zero-cost batched solve
  (`Options::confirm_unbounded`) and returns a Farkas ray if the problem is in fact infeasible.
* Numerical stall at degenerate vertices (primal residual stuck ~4e-8) was cured by full-system iterative refinement
  against the exact Newton equations with a primal-proximal preconditioner (`primal_regularization`).
* `-inf * 0 = NaN` in the bound shift (non-finite entries must be zeroed before arithmetic) broke every problem with an
  infinite bound until the campaign found it.

## Regenerate
`generate_reference.py <out.h> [dir of netlib *.expanded.mps]` (needs numpy, scipy; highspy for NETLIB).
NETLIB files: https://www.netlib.org/lp/data/<name>, expanded with David Gay's `emps.c`
(https://www.netlib.org/lp/data/emps.c, sha256 fee41f544f6873a5e12bc598947828dc9964ef0676162e4df55e915760e2be22).
Known limitations: dense normal equations (fine to a few thousand rows), finite upper bounds add rows, rows must be
independent up to the regularisation (dependent equality rows are handled, but are not eliminated), no warm start.
