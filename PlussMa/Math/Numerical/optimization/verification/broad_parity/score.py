#!/usr/bin/env python3
"""Scores Optima against reference results. Usage: score.py matlab_out.txt ours_out.txt [-v]

Two yardsticks. "strict": the lowest objective among reference runs that report success (flag > 0) with
violation <= 1e-5. An Optima run matches when it exits Converged (flag 1), is feasible (violation <= 1e-5)
and its objective is within 1e-4 (relative) of the reference or lower. "matched": the same test
against the reference run of the corresponding algorithm (interior-point, sqp, active-set), which is what a
nonconvex problem with several local minima fairly demands; a run passes "matched" if it passes either.
The merit method has no counterpart and is scored on the strict yardstick only.
"""
import sys
def load(path):
    d = {}
    for l in open(path):
        p = l.rstrip("\n").split("|")
        if len(p) < 7: continue
        try: d[(p[0], p[1])] = (p[2], int(p[3]), float(p[5]), float(p[6]), p[7] if len(p) > 7 else "")
        except ValueError: d[(p[0], p[1])] = ("ERR", 0, float("nan"), float("nan"), p[-1])
    return d
m, o = load(sys.argv[1]), load(sys.argv[2])
verbose = "-v" in sys.argv
probs = sorted({k[0] for k in o}, key=lambda s: [k[0] for k in o].index(s) if False else 0)
order = []
for k in o:
    if k[0] not in order: order.append(k[0])
ref = {}
for p in order:
    best = None
    for a in ("interior-point", "sqp", "active-set"):
        r = m.get((p, a))
        if r and r[0] not in ("ERR",) and int(r[0]) > 0 and r[3] <= 1e-5 and r[2] == r[2]:
            best = r[2] if best is None else min(best, r[2])
    ref[p] = best
algs = ["interior-point", "sqp", "active-set", "merit"]
def good(r, target):
    return r and r[0] == "1" and r[3] <= 1e-5 and r[2] <= target + 1e-4 * (1 + abs(target))
strict = {a: [0, 0] for a in algs}
matched = {a: [0, 0] for a in algs}
fails = {a: [] for a in algs}
for p in order:
    if ref[p] is None: continue
    for a in algs:
        r = o.get((p, a))
        s_ok = good(r, ref[p])
        mref = m.get((p, a)) if a != "merit" else None
        m_ok = s_ok or (mref is not None and mref[0] != "ERR" and int(mref[0]) > 0 and mref[3] <= 1e-5 and good(r, mref[2]))
        strict[a][1] += 1; matched[a][1] += 1
        strict[a][0] += bool(s_ok); matched[a][0] += bool(m_ok)
        if not m_ok: fails[a].append((p, r, ref[p]))
nref = sum(1 for p in order if ref[p] is not None)
print(f"problems with a reference: {nref} of {len(order)}")
print(f"{'algorithm':15} {'strict':>14} {'matched':>14}")
for a in algs:
    print(f"{a:15} {strict[a][0]:3}/{strict[a][1]} {100.0*strict[a][0]/strict[a][1]:5.1f}%   {matched[a][0]:3}/{matched[a][1]} {100.0*matched[a][0]/matched[a][1]:5.1f}%")
core = algs[:3]
for name, d in (("strict", strict), ("matched", matched)):
    ok = sum(d[a][0] for a in core); n = sum(d[a][1] for a in core)
    print(f"three MATLAB-counterpart algorithms, {name}: {ok}/{n} = {100.0*ok/n:.1f}%")
if verbose:
    for a in algs:
        for p, r, rf in fails[a]:
            print(f"  FAIL {a:14} {p:9} ref={rf:.8g} ours={r[:4] if r else None} {r[4][:50] if r else ''}")
