#!/usr/bin/env python3
"""Generate test/linprog_reference.h for linprog_test.cpp.

References: SciPy/HiGHS (`scipy.optimize.linprog`, method 'highs') for the random families and NETLIB LP problems
(https://www.netlib.org/lp/data/, expanded with David Gay's emps.c) whose optimal values are published.
HiGHS presolve reports some feasible-but-unbounded LPs as 'infeasible', so the reference status is decided by a
zero-objective feasibility solve plus a presolve-off rerun (see README.md).

usage: generate_reference.py <out.h> [netlib_dir_with_*.expanded.mps  (needs highspy)]
"""
import sys, numpy as np
from scipy.optimize import linprog
INF = np.inf
rng = np.random.default_rng(20261014)
groups = []

def fmt(a):
    return ",".join("INF" if v == INF else "-INF" if v == -INF else repr(float(v)) for v in np.ravel(a))

def reference(c, Ai, bi, Ae, be, lb, ub):
    n = len(c)
    kw = dict(A_ub=Ai if len(bi) else None, b_ub=bi if len(bi) else None, A_eq=Ae if len(be) else None,
              b_eq=be if len(be) else None, bounds=list(zip(lb, ub)), method='highs')
    r = linprog(c, **kw)
    if r.status == 0:
        return 0, r.fun
    if linprog(np.zeros(n), **kw).status == 2:
        return 1, float('nan')
    r2 = linprog(c, options={'presolve': False}, **kw)
    return (0, r2.fun) if r2.status == 0 else (2, float('nan'))

def add(name, n, Ai, Ae, lanes, published=None):
    out = []
    for (c, bi, be, lb, ub) in lanes:
        st, obj = reference(c, Ai, bi, Ae, be, lb, ub)
        out.append((c, bi, be, lb, ub, st, obj))
    groups.append((name, n, Ai, Ae, out, published))

for t in range(40):   # feasible: box / free variables / negative lower bounds / no upper bound
    n = int(rng.integers(3, 30)); mi = int(rng.integers(0, 10)); me = int(rng.integers(0, min(5, n - 1)))
    if mi + me == 0: mi = 2
    kind = t % 4
    lb = np.zeros(n); ub = np.full(n, 5.0)
    if kind == 1: lb = np.where(rng.random(n) < 0.4, -INF, lb); ub = np.where(rng.random(n) < 0.4, INF, ub)
    if kind == 2: lb = -rng.uniform(0, 3, n)
    if kind == 3: ub = np.full(n, INF)
    x0 = np.clip(rng.uniform(-1, 2, n), np.where(np.isfinite(lb), lb + .1, -1), np.where(np.isfinite(ub), ub - .1, 3))
    Ai = rng.normal(size=(mi, n)); Ae = rng.normal(size=(me, n))
    c = rng.normal(size=n)
    if kind == 3: c = np.abs(c)
    add('random%d' % t, n, Ai, Ae, [(c, Ai @ x0 + rng.uniform(0, 1, mi), Ae @ x0, lb, ub)])
for t in range(6):   # infeasible
    n = int(rng.integers(3, 12)); Ai = rng.normal(size=(2, n)); Ai = np.vstack([Ai, -Ai[:1]])
    add('infeasible%d' % t, n, Ai, np.zeros((0, n)), [(rng.normal(size=n), np.array([1., 1., -2. - rng.uniform(0, 1)]),
                                                     np.zeros(0), np.full(n, -INF), np.full(n, INF))])
for t in range(4):   # infeasible with a redundant equality row
    n = int(rng.integers(3, 12)); Ae = rng.normal(size=(2, n)); Ae = np.vstack([Ae, Ae[:1]])
    add('infeasible_redundant%d' % t, n, np.zeros((0, n)), Ae, [(rng.normal(size=n), np.zeros(0), np.array([1., 2., 5.]), np.zeros(n), np.full(n, INF))])
for t in range(6):   # unbounded (strict recession direction)
    n = int(rng.integers(3, 12)); d = np.abs(rng.normal(size=n)); Ai = np.abs(rng.normal(size=(3, n)))
    Ai = Ai - np.outer(Ai @ d / (d @ d), d) - 0.05 * np.outer(np.ones(3), d)
    add('unbounded%d' % t, n, Ai, np.zeros((0, n)), [(-np.abs(rng.normal(size=n)), np.abs(rng.normal(size=3)) + 1, np.zeros(0), np.zeros(n), np.full(n, INF))])
for t in range(6):   # degenerate: dependent equality rows, duplicated tight inequalities
    n = int(rng.integers(4, 16)); Ae = rng.normal(size=(3, n)); Ae = np.vstack([Ae, Ae[:1] + Ae[1:2]])
    x0 = rng.uniform(.2, 2, n); Ai = rng.normal(size=(3, n)); Ai = np.vstack([Ai, Ai[:1]])
    add('degenerate%d' % t, n, Ai, Ae, [(rng.normal(size=n), Ai @ x0, Ae @ x0, np.zeros(n), np.full(n, 6.))])
for t in range(2):   # one batched call: 150 lanes sharing A (c, rhs and bounds differ per lane)
    n = 14; Ai = rng.normal(size=(6, n)); Ae = rng.normal(size=(3, n)); lanes = []
    for l in range(150):
        x0 = rng.uniform(.2, 2, n); lanes.append((rng.normal(size=n), Ai @ x0 + rng.uniform(0, 1, 6), Ae @ x0, np.zeros(n), np.full(n, 5.)))
    add('batch%d' % t, n, Ai, Ae, lanes)

for t in range(4):   # empty columns / rows (a variable in no constraint, a constraint row of zeros): scaling regression
    n = int(rng.integers(5, 12)); Ai = rng.normal(size=(4, n)); Ai[:, rng.integers(0, n)] = 0.0; Ai[rng.integers(0, 4), :] = 0.0
    x0 = rng.uniform(.2, 2, n); c = rng.normal(size=n)
    add('empty_row_col%d' % t, n, Ai, np.zeros((0, n)), [(c, Ai @ x0 + rng.uniform(.1, 1, 4), np.zeros(0), np.zeros(n), np.full(n, 4.))])
published = {'afiro': -4.6475314286e+02, 'sc50a': -6.4575077059e+01, 'sc50b': -7.0e+01, 'kb2': -1.7499001299e+03,
             'adlittle': 2.2549496316e+05, 'blend': -3.0812149846e+01, 'share2b': -4.1573224074e+02}
if len(sys.argv) > 2:
    import highspy
    for nm, pub in published.items():
        h = highspy.Highs(); h.setOptionValue('output_flag', False); h.readModel('%s/%s.expanded.mps' % (sys.argv[2], nm))
        lp = h.getLp(); n, m = lp.num_col_, lp.num_row_
        A = np.zeros((m, n)); st, idx, val = np.array(lp.a_matrix_.start_), np.array(lp.a_matrix_.index_), np.array(lp.a_matrix_.value_)
        for j in range(n):
            for k in range(st[j], st[j + 1]): A[idx[k], j] = val[k]
        c = np.array(lp.col_cost_); lb = np.array(lp.col_lower_); ub = np.array(lp.col_upper_)
        rl = np.array(lp.row_lower_); ru = np.array(lp.row_upper_)
        lb = np.where(lb <= -1e20, -INF, lb); ub = np.where(ub >= 1e20, INF, ub); rl = np.where(rl <= -1e20, -INF, rl); ru = np.where(ru >= 1e20, INF, ru)
        eq = [i for i in range(m) if rl[i] == ru[i]]; rows = []; rhs = []
        for i in range(m):
            if i in eq: continue
            if np.isfinite(ru[i]): rows.append(A[i]); rhs.append(ru[i])
            if np.isfinite(rl[i]): rows.append(-A[i]); rhs.append(-rl[i])
        add('netlib_' + nm, n, np.array(rows).reshape(-1, n), A[eq], [(c, np.array(rhs), rl[eq], lb, ub)], (pub - lp.offset_))

with open(sys.argv[1], 'w') as f:
    f.write("// Generated by verification/linprog/generate_reference.py (SciPy/HiGHS + NETLIB); do not edit.\n#pragma once\n#include <cmath>\n#include <vector>\n#define INF INFINITY\nnamespace linprog_reference {\n")
    f.write("struct Lane { std::vector<double> c, bi, be, lb, ub; int status; double objective; };\n")
    f.write("struct Group { const char *name; int n, mi, me; std::vector<double> ai, ae; std::vector<Lane> lanes; double published; };\n")
    f.write("inline const std::vector<Group> &groups() {\n  static const std::vector<Group> g = {\n")
    for name, n, Ai, Ae, lanes, pub in groups:
        f.write('    {"%s", %d, %d, %d, {%s}, {%s}, {\n' % (name, n, Ai.shape[0], Ae.shape[0], fmt(Ai), fmt(Ae)))
        for c, bi, be, lb, ub, st, obj in lanes:
            f.write('      {{%s}, {%s}, {%s}, {%s}, {%s}, %d, %s},\n' % (fmt(c), fmt(bi), fmt(be), fmt(lb), fmt(ub), st, "NAN" if st else repr(float(obj))))
        f.write('    }, %s},\n' % ("NAN" if pub is None else repr(float(pub))))
    f.write("  };\n  return g;\n}\n} // namespace linprog_reference\n")
print("groups:", len(groups))
