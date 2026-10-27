# Muscle-driven two-link arm oracle: sympy Lagrangian M(q), h(q, qd) for the chain, the De Groote-Fregly muscle equations (constants parsed from the OpenSim
# header) with polynomial moment arms, coupled and integrated with scipy DOP853. Run from NatSci/biology:
#   python verification/gen_msk_ref.py <path to DeGrooteFregly2016Muscle.h>
import re, sys, numpy as np, sympy as sp
from scipy.integrate import solve_ivp
from scipy.optimize import brentq
src = open(sys.argv[1]).read()
def const(name): return float(re.search(r"constexpr static double %s\s*=\s*([-0-9.eE+]+)" % name, src).group(1))
b = {k: const(k) for k in ["b11", "b21", "b31", "b41", "b12", "b22", "b32", "b42", "b13", "b23", "b33", "b43"]}
d = {k: const(k) for k in ["d1", "d2", "d3", "d4"]}
c1, c2, c3, kPE, LMIN = const("c1"), const("c2"), const("c3"), const("kPE"), const("m_minNormFiberLength")
def gauss(x, b1, b2, b3, b4): return b1 * np.exp(-0.5 * (x - b2) ** 2 / (b3 + b4 * x) ** 2)
def fal(l): return sum(gauss(l, b["b1%d" % k], b["b2%d" % k], b["b3%d" % k], b["b4%d" % k]) for k in (1, 2, 3))
def fpe(l, e0=0.6): off = np.exp(kPE * (LMIN - 1) / e0); return (np.exp(kPE * (l - 1) / e0) - off) / (np.exp(kPE) - off)
def fvinv(f): return (np.sinh((f - d["d4"]) / d["d1"]) - d["d3"]) / d["d2"]
def kT(s1): return np.log((1 + c3) / c1) / (1 + s1 - c2)
def ltinv(f, s1=0.049): return np.log((f + c3) / c1) / kT(s1) + c2
def dft(lt, s1=0.049): return c1 * kT(s1) * np.exp(kT(s1) * (lt - c2))
# --- chain (2 links) via sympy
n = 2
q = sp.symbols("q1:3"); qd = sp.symbols("d1:3"); qdd = sp.symbols("dd1:3")
m = sp.symbols("m1:3", positive=True); l = sp.symbols("l1:3", positive=True); c = sp.symbols("c1:3", positive=True); I = sp.symbols("I1:3", positive=True); g = sp.Symbol("g", positive=True)
th = [sum(q[: i + 1]) for i in range(n)]; thd = [sum(qd[: i + 1]) for i in range(n)]
jx = [0]; jy = [0]; cx = []; cy = []
for i in range(n):
    cx.append(jx[-1] + c[i] * sp.cos(th[i])); cy.append(jy[-1] + c[i] * sp.sin(th[i])); jx.append(jx[-1] + l[i] * sp.cos(th[i])); jy.append(jy[-1] + l[i] * sp.sin(th[i]))
def tdot(e): return sum(sp.diff(e, q[k]) * qd[k] for k in range(n))
T = sum(sp.Rational(1, 2) * m[i] * (tdot(cx[i]) ** 2 + tdot(cy[i]) ** 2) + sp.Rational(1, 2) * I[i] * thd[i] ** 2 for i in range(n))
V = sum(m[i] * g * cy[i] for i in range(n)); Lg = T - V
Msym = sp.hessian(T, qd)
tau = []
for k in range(n):
    p = sp.diff(Lg, qd[k]); tau.append(sum(sp.diff(p, q[j]) * qd[j] + sp.diff(p, qd[j]) * qdd[j] for j in range(n)) - sp.diff(Lg, q[k]))
h = sp.Matrix(tau).subs({s: 0 for s in qdd})
Pc = dict(m=[1.9, 1.2], l=[0.30, 0.27], c=[0.13, 0.12], I=[0.020, 0.010], g=9.80665)
fM = sp.lambdify(list(q) + list(m) + list(l) + list(c) + list(I), Msym, "numpy"); fh = sp.lambdify(list(q) + list(qd) + list(m) + list(l) + list(c) + list(I) + [g], h, "numpy")
# --- muscles: F, lopt, lts, moment-arm polynomial c0 + c1 q (per joint), reference length
mus = [  # F, lopt, lts, [r_j1 coeffs], [r_j2 coeffs]
    (1200.0, 0.10, 0.18, ([0.045, 0.0], [0.0, 0.0])),
    (900.0, 0.10, 0.20, ([-0.040, 0.0], [0.0, 0.0])),
    (800.0, 0.08, 0.22, ([0.0, 0.0], [0.030, 0.012])),   # elbow flexor with a posture-dependent moment arm
    (700.0, 0.09, 0.15, ([0.0, 0.0], [-0.025, 0.0])),
    (500.0, 0.11, 0.24, ([0.035, 0.0], [0.025, 0.0])),   # biarticular flexor
]
M = len(mus); J = 2
P = dict(vmax=10.0, ta=0.015, td=0.060, s1=0.049, e0=0.6, smooth=0.1, alpha=0.2)
q0 = np.array([0.35, 0.9]); qref = q0.copy()
def arm(i, j, qq): co = mus[i][3][j]; return co[0] + co[1] * qq
def mtu_len(i, qq):
    L = L0[i]
    for j in range(J):
        co = mus[i][3][j]
        L -= co[0] * (qq[j] - qref[j]) + 0.5 * co[1] * (qq[j] ** 2 - qref[j] ** 2)
    return L
L0 = [mus[i][2] + mus[i][1] * np.cos(P["alpha"]) * 1.0 for i in range(M)]  # reference MTU length at q_ref: tendon slack + optimal fibre along tendon
def geom(i, L, f):
    F, lopt, lts, _ = mus[i]; wid = lopt * np.sin(P["alpha"]); lt = ltinv(f, P["s1"]) * lts; al = L - lt; lf = np.hypot(al, wid); return lf / lopt, al / lf
def iso(i, a, L):
    def gg(f): ln, cs = geom(i, L, f); return f - cs * (a * fal(ln) + fpe(ln, P["e0"]))
    return brentq(gg, 0.0, 5.0, xtol=1e-14)
BASE = np.array([0.45, 0.50, 0.45, 0.50, 0.30])
def exc(t): return BASE + np.array([0.25 * 0.5 * (1 + np.tanh((t - 0.1) / 0.03)), 0.0, 0.2 * 0.5 * (1 + np.tanh((t - 0.2) / 0.04)), -0.1 * 0.5 * (1 + np.tanh((t - 0.2) / 0.04)), 0.1 * 0.5 * (1 + np.tanh((t - 0.15) / 0.05))])
a0 = BASE.copy()
f0 = np.array([iso(i, a0[i], mtu_len(i, q0)) for i in range(M)])
def arate(a, e):
    z = 0.5 + 1.5 * a; f = 0.5 * np.tanh(P["smooth"] * (e - a)); return ((f + 0.5) / (P["ta"] * z) + (-f + 0.5) * z / P["td"]) * (e - a)
def rhs(t, y):
    qq = y[:J]; dq = y[J:2 * J]; a = y[2 * J:2 * J + M]; f = y[2 * J + M:]
    F = np.array([mus[i][0] for i in range(M)]) * f
    R = np.array([[arm(i, j, qq[j]) for j in range(J)] for i in range(M)])
    tq = R.T @ F
    Mn = np.array(fM(*(list(qq) + Pc["m"] + Pc["l"] + Pc["c"] + Pc["I"])), float); hn = np.array(fh(*(list(qq) + list(dq) + Pc["m"] + Pc["l"] + Pc["c"] + Pc["I"] + [Pc["g"]])), float).ravel()
    qdd = np.linalg.solve(Mn, tq - hn)
    e = exc(t); da = arate(a, e); df = np.zeros(M)
    for i in range(M):
        Fm, lopt, lts, _ = mus[i]
        L = mtu_len(i, qq); Ld = -sum(arm(i, j, qq[j]) * dq[j] for j in range(J))
        ln, cs = geom(i, L, f[i]); fvm = (f[i] / cs - fpe(ln, P["e0"])) / (a[i] * fal(ln)); vn = fvinv(fvm); vf = vn * P["vmax"] * lopt
        vt = Ld - vf / cs; df[i] = vt / lts * dft(ltinv(f[i], P["s1"]), P["s1"])
    return np.r_[dq, qdd, da, df]
y0 = np.r_[q0, [0.0, 0.0], a0, f0]
sol = solve_ivp(rhs, (0, 0.5), y0, method="DOP853", rtol=1e-11, atol=1e-12, dense_output=True)
ts = [0.1, 0.2, 0.3, 0.4, 0.5]
traj = [v for t in ts for v in sol.sol(t)]
coef = np.zeros((M, J, 2))
for i in range(M):
    for j in range(J): coef[i, j] = mus[i][3][j]
def arr(name, a): return "inline const std::vector<double> %s = {%s};" % (name, ",".join(repr(float(t)) for t in np.asarray(a, float).ravel()))
with open("biomechanics/test/msk_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/biology/verification/gen_msk_ref.py; do not edit.\n#pragma once\n#include <cmath>\n#include <vector>\nnamespace msk_reference {\n" + "\n".join([
        arr("chain_m", Pc["m"]), arr("chain_l", Pc["l"]), arr("chain_c", Pc["c"]), arr("chain_I", Pc["I"]), arr("muscle_f", [x[0] for x in mus]), arr("muscle_lopt", [x[1] for x in mus]), arr("muscle_lts", [x[2] for x in mus]),
        arr("coef", coef), arr("l0", L0), arr("q0", q0), arr("a0", a0), arr("f0", f0), "inline const double alpha = %r;" % P["alpha"], arr("traj_t", ts), arr("traj", traj)]) + "\n}\n")
print("final", sol.y[:, -1])
tt = np.linspace(0, 0.5, 51); Y = sol.sol(tt)
print("min fT", Y[2 * J + M:].min(), "q range", Y[:J].min(), Y[:J].max(), "max |qd|", np.abs(Y[J:2 * J]).max())
lns = [[geom(i, mtu_len(i, Y[:J, k]), Y[2 * J + M + i, k])[0] for k in range(len(tt))] for i in range(M)]
print("norm fibre length range", np.min(lns), np.max(lns))
