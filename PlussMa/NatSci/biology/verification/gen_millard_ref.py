# Millard quintic-Bezier curve oracle: an independent numpy/scipy re-implementation of OpenSim's SmoothSegmentedFunctionFactory construction (control points
# from corners, scaled curviness) with u(x) by brentq and analytic Bezier derivatives; plus isometric muscle-tendon equilibria with the Millard curves.
# Run from NatSci/biology:  python verification/gen_millard_ref.py
import numpy as np
from math import comb
from scipy.optimize import brentq
def corner(x0, y0, d0, x1, y1, d1, curv):
    xc = (y1 - y0 - x1 * d1 + x0 * d0) / (d0 - d1) if abs(d0 - d1) > np.sqrt(np.finfo(float).eps) else 0.5 * (x0 + x1)
    yc = (xc - x1) * d1 + y1
    a = (xc - x0) ** 2 + (yc - y0) ** 2; b = (xc - x1) ** 2 + (yc - y1) ** 2; c = (x1 - x0) ** 2 + (y1 - y0) ** 2
    assert c > a and c > b
    xm0 = x0 + curv * (xc - x0); ym0 = y0 + curv * (yc - y0); xm1 = x1 + curv * (xc - x1); ym1 = y1 + curv * (yc - y1)
    return np.array([x0, xm0, xm0, xm1, xm1, x1]), np.array([y0, ym0, ym0, ym1, ym1, y1])
def bern(P, u, k=0):
    # k-th derivative of the quintic Bezier with control values P at u
    Q = np.array(P, float)
    for _ in range(k): Q = (len(Q) - 1) * np.diff(Q)
    n = len(Q) - 1
    return sum(comb(n, i) * u ** i * (1 - u) ** (n - i) * Q[i] for i in range(n + 1))
class Curve:
    def __init__(self, segs, sb, sa): self.s, self.sb, self.sa = segs, sb, sa
    def __call__(self, x, order=0):
        x0, x1 = self.s[0][0][0], self.s[-1][0][-1]
        if x < x0: return [self.s[0][1][0] + self.sb * (x - x0), self.sb, 0.0][order]
        if x > x1: return [self.s[-1][1][-1] + self.sa * (x - x1), self.sa, 0.0][order]
        for k, (X, Y) in enumerate(self.s):
            if X[0] <= x <= X[-1] or k == len(self.s) - 1 and x <= X[-1]:
                u = brentq(lambda u: bern(X, u) - x, 0.0, 1.0, xtol=1e-15, rtol=1e-15)
                xu, yu = bern(X, u, 1), bern(Y, u, 1)
                if order == 0: return bern(Y, u)
                if order == 1: return yu / xu
                xuu, yuu = bern(X, u, 2), bern(Y, u, 2)
                return (yuu * xu - yu * xuu) / xu ** 3
def sc(c): return 0.1 + 0.8 * c
def active(x0=0.47 - 0.0259, x1=0.73, x3=1.8123, dydx=0.8616, ylow=0.1, curv=1.0):
    c = sc(curv); x2 = 1.0; d = 0.05 * x2; xs = x2 - d; y0 = 0.0; y1 = 1 - dydx * (xs - x1); d01 = 1.25 * (y1 - y0) / (x1 - x0)
    x01 = x0 + 0.5 * (x1 - x0); y01 = y0 + 0.5 * (y1 - y0); x1s = x1 + 0.5 * (xs - x1); y1s = y1 + 0.5 * (1 - y1)
    x23 = (x2 + d) + 0.5 * (x3 - (x2 + d)); y23 = 0.5; d23 = (0 - 1) / ((x3 - d) - (x2 + d))
    return Curve([corner(x0, ylow, 0, x01, y01, d01, c), corner(x01, y01, d01, x1s, y1s, dydx, c), corner(x1s, y1s, dydx, x2, 1, 0, c), corner(x2, 1, 0, x23, y23, d23, c), corner(x23, y23, d23, x3, ylow, 0, c)], 0.0, 0.0)
def fvel(fmaxE=1.4, dC=0.0, dNC=0.25, dIso=5.0, dE=0.0, dNE=0.15, cc=0.6, ce=0.9):
    cC, cE = sc(cc), sc(ce); xnc = -0.9; ync = 0 + 0.5 * dNC * (xnc + 1) + 0.5 * dC * (xnc + 1); xne = 0.9; yne = fmaxE + 0.5 * dNE * (xne - 1) + 0.5 * dE * (xne - 1)
    return Curve([corner(-1, 0, dC, xnc, ync, dNC, cC), corner(xnc, ync, dNC, 0, 1, dIso, cC), corner(0, 1, dIso, xne, yne, dNE, cE), corner(xne, yne, dNE, 1, fmaxE, dE, cE)], dC, dE)
def passive(e0=0.0, e1=0.7, kl=0.2, ki=None, curv=0.75):
    ki = 2.0 / (e1 - e0) if ki is None else ki; c = sc(curv); xz = 1 + e0; xi = 1 + e1; dx = min(0.1 / ki, 0.1 * (xi - xz)); xl = xz + dx; xf = xz + 0.5 * (xl - xz); yl = kl * (xl - xf)
    return Curve([corner(xz, 0, 0, xl, yl, kl, c), corner(xl, yl, kl, xi, 1, ki, c)], 0.0, ki)
def tendon(e1=0.049, ki=None, ft=2 / 3, curv=0.5):
    ki = 1.375 / e1 if ki is None else ki; c = sc(curv); xi = 1 + e1; xt = (ft - 1) / ki + xi; xf = 1 + (xt - 1) / 10; ytm = 0.5 * ft; xtm = (ytm - 1) / ki + xi
    sm = (ytm - 0) / (xtm - xf); xc = xf + 0.5 * (xtm - xf); yc = sm * (xc - xf)
    return Curve([corner(1, 0, 0, xc, yc, sm, c), corner(xc, yc, sm, xt, ft, ki, c)], 0.0, ki)
A, V, P, T = active(), fvel(), passive(), tendon()
grids = {"act": (A, np.linspace(0.2, 2.0, 37)), "fv": (V, np.linspace(-1.2, 1.2, 33)), "pas": (P, np.linspace(0.9, 1.9, 33)), "ten": (T, np.linspace(0.95, 1.07, 33))}
out = {}
for k, (c, g) in grids.items():
    out[k + "_x"] = g
    out[k + "_y"] = [c(x, 0) for x in g]; out[k + "_d1"] = [c(x, 1) for x in g]; out[k + "_d2"] = [c(x, 2) for x in g]
# muscle-tendon isometric equilibria with the Millard curves (same geometry as the De Groote test)
Fm, lopt, lts, alpha = 800.0, 0.09, 0.24, 0.35
width = lopt * np.sin(alpha)
def tinv(f): return brentq(lambda x: T(x, 0) - f, 1.0, 1.2, xtol=1e-15) if f > 1e-12 else 1.0
def geom(L, f):
    lt = tinv(f) * lts; al = L - lt; lf = np.hypot(al, width); return lf / lopt, al / lf
eq = []
for a in [0.2, 0.6, 1.0]:
    for L in [0.30, 0.33, 0.35]:
        g = lambda f: f - geom(L, f)[1] * (a * A(geom(L, f)[0]) + P(geom(L, f)[0]))
        f = brentq(g, 0.0, 3.0, xtol=1e-14); ln, cs = geom(L, f); eq += [a, L, f, ln]
out["eq"] = eq; out["muscle"] = [Fm, lopt, lts, alpha]
def arr(name, a): return "inline const std::vector<double> %s = {%s};" % (name, ",".join(repr(float(t)) for t in np.asarray(a, float).ravel()))
with open("biomechanics/test/millard_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/biology/verification/gen_millard_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace millard_reference {\n" + "\n".join(arr(k, v) for k, v in out.items()) + "\n}\n")
print("keys", A(1.0), A(0.4441, 0), V(0), V(-1), V(1), P(1.7), T(1.049), "ok")
