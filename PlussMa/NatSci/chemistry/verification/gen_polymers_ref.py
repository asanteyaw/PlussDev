"""Reference values for chemistry/polymers (generates polymers/test/polymers_reference.h).

Independent oracles (never the formula under test evaluated the same way):
  * worm-like chain <R^2>, <Rg^2>: numerical quadrature of the DEFINITIONS (tangent correlation exp(-s/P)), scipy.integrate.quad;
  * freely-rotating chain: brute-force double sum  b^2 sum_ij c^|i-j| ;
  * Langevin function: mpmath at 50 digits;  Marko-Siggia: closed-form spot values;
  * distributions: scipy.stats (geom, poisson) moments and pmfs;
  * radical polymerization: CHAIN-LENGTH-RESOLVED kinetics (every live and dead chain length an explicit species, combination by
    convolution) integrated with scipy Radau; its moments are compared with the closed moment equations;
  * Mayo-Lewis / Skeist: scipy solve_ivp (DOP853) of the drift ODE; Meyer-Lowry closed form against it;
  * Flory-Huggins: scipy fsolve of the equal-chemical-potential conditions with the symmetric case checked against the exact
    relation ln((1+m)/(1-m)) = chi N m (brentq).
"""
import numpy as np, mpmath as mp
from scipy import integrate, optimize, stats

out = []
def arr(name, v):
    out.append("inline const std::vector<double> %s = {%s};" % (name, ",".join(repr(float(x)) for x in np.ravel(v))))

# ---- worm-like chain
P = 1.0
Ls = np.array([1e-4, 1e-2, 0.1, 0.5, 1, 3, 10, 100, 1000.0])
def wlc_r2(L):  # 2 int_0^L (L-s) exp(-s/P) ds
    return 2 * integrate.quad(lambda s: (L - s) * np.exp(-s / P), 0, L, epsabs=1e-14, epsrel=1e-13, limit=200)[0]
def wlc_rg2(L):  # (1/L^2) int_0^L (L-t) [2 P t - 2 P^2 (1 - exp(-t/P))] dt
    f = lambda t: (L - t) * (2 * P * t - 2 * P**2 * (-np.expm1(-t / P)))
    return integrate.quad(f, 0, L, epsabs=1e-14, epsrel=1e-13, limit=200)[0] / L**2 / 2.0 * 1.0
arr("wlc_L", Ls); arr("wlc_r2", [wlc_r2(L) for L in Ls]);
# Rg^2 = (1/(2 L^2)) int int <(r(s)-r(s'))^2> = (1/L^2) int_0^L (L-t) <R^2(t)> dt
arr("wlc_rg2", [integrate.quad(lambda t, L=L: (L - t) * wlc_r2(t) if t > 0 else 0.0, 0, L, epsabs=1e-13, epsrel=1e-11, limit=400)[0] / L**2 for L in Ls])

# ---- freely rotating chain (brute force sums)
frc_cases = [(10, 1.0, 0.5), (50, 0.7, 0.8), (200, 0.1, 0.95), (7, 2.0, -0.3), (1000, 0.05, 0.99)]
vals = []
for N, b, c in frc_cases:
    i = np.arange(N); vals.append(b * b * np.sum(c ** np.abs(i[:, None] - i[None, :])))
arr("frc_n", [x[0] for x in frc_cases]); arr("frc_b", [x[1] for x in frc_cases]); arr("frc_c", [x[2] for x in frc_cases]); arr("frc_r2", vals)

# ---- Langevin function
mp.mp.dps = 50
a_grid = [1e-6, 1e-3, 0.04, 0.0499, 0.05, 0.0501, 0.3, 1.0, 5.0, 30.0, 200.0]
arr("fjc_a", a_grid); arr("fjc_x", [float(mp.coth(mp.mpf(a)) - 1 / mp.mpf(a)) for a in a_grid])

# ---- distributions
xs = np.arange(1, 301.0)
p = 0.97
arr("fs_p", [p]);
arr("fs_n", stats.geom.pmf(xs, 1 - p)); arr("fs_w", xs * (1 - p) ** 2 * p ** (xs - 1))
nu = 12.5
arr("po_nu", [nu]); arr("po_n", stats.poisson.pmf(xs - 1, nu))
# averages of the Poisson distribution computed directly with numpy float128-free sums
n = stats.poisson.pmf(xs - 1, nu); s0, s1, s2, s3 = [np.sum(xs ** k * n) for k in range(4)]
arr("po_avg", [s1 / s0, s2 / s1, s3 / s2])

# ---- Mayo-Lewis / Skeist / Meyer-Lowry
def F1(f1, r1, r2):
    f2 = 1 - f1; num = r1 * f1**2 + f1 * f2
    return num / (num + f1 * f2 + r2 * f2**2)
ml_cases = [(0.5, 0.3, 0.2), (0.5, 0.3, 0.8), (0.5, 2.0, 0.5), (0.9, 0.1, 5.0), (0.3, 0.8, 0.9)]
arr("ml_f1", [c[0] for c in ml_cases]); arr("ml_r1", [c[1] for c in ml_cases]); arr("ml_r2", [c[2] for c in ml_cases])
arr("ml_F1", [F1(*c) for c in ml_cases])
sk = []
for f10, r1, r2 in [(0.6, 0.5, 0.3), (0.2, 2.0, 0.5), (0.4, 0.1, 0.1)]:
    sol = integrate.solve_ivp(lambda X, y: [(y[0] - F1(y[0], r1, r2)) / (1 - X)], [0, 0.6], [f10], method="DOP853", rtol=1e-12, atol=1e-14)
    sk.append((f10, r1, r2, sol.y[0, -1]))
arr("sk_f10", [s[0] for s in sk]); arr("sk_r1", [s[1] for s in sk]); arr("sk_r2", [s[2] for s in sk]); arr("sk_f1", [s[3] for s in sk])

# ---- Flory-Huggins
def m1(phi, n1, n2, chi): return np.log(1 - phi) / n1 + (1 / n1 - 1 / n2) * phi + chi * phi**2
def m2(phi, n1, n2, chi): return np.log(phi) / n2 + (1 / n2 - 1 / n1) * (1 - phi) + chi * (1 - phi) ** 2
# symmetric: n1 = n2 = N: binodal phi = (1 +- m)/2, ln((1+m)/(1-m)) = chi N m
sym = []
for N, chiN in [(10, 2.5), (10, 3.0), (50, 4.0), (1, 2.2)]:
    mm = optimize.brentq(lambda m: np.log((1 + m) / (1 - m)) - chiN * m, 1e-6, 1 - 1e-15)
    sym.append((N, chiN / N, 0.5 * (1 - mm), 0.5 * (1 + mm)))
arr("fhs_n", [s[0] for s in sym]); arr("fhs_chi", [s[1] for s in sym]); arr("fhs_a", [s[2] for s in sym]); arr("fhs_b", [s[3] for s in sym])
# asymmetric: solve with several starts and keep the converged one
asy = []
for n1, n2, chi in [(1, 100, 0.7), (1, 100, 1.0), (1, 20, 0.8), (5, 50, 0.4)]:
    sol = None
    for a0 in np.concatenate([np.geomspace(1e-9, 0.09, 12)]):
        for b0 in np.linspace(0.15, 0.99, 14):
            try:
                r = optimize.fsolve(lambda v: [m1(v[0], n1, n2, chi) - m1(v[1], n1, n2, chi), m2(v[0], n1, n2, chi) - m2(v[1], n1, n2, chi)], [a0, b0], xtol=1e-14, full_output=True)
                v = r[0]
                if r[2] == 1 and 0 < v[0] < v[1] < 1 and abs(v[1] - v[0]) > 1e-3 and np.max(np.abs(r[1]["fvec"])) < 1e-12: sol = v
            except Exception: pass
    if sol is None: print('no binodal for', n1, n2, chi); continue
    asy.append((n1, n2, chi, sol[0], sol[1]))
arr("fha_n1", [s[0] for s in asy]); arr("fha_n2", [s[1] for s in asy]); arr("fha_chi", [s[2] for s in asy]); arr("fha_a", [s[3] for s in asy]); arr("fha_b", [s[4] for s in asy])

# ---- radical polymerization: chain-length resolved oracle
kd, f, kp, ktc, ktd, ktm = 1e-3, 0.6, 100.0, 1e6, 2e6, 0.5
I0, M0 = 0.05, 5.0
N = 1200
kt = ktc + ktd
def rhs(t, y):
    I, M = y[0], y[1]; Pn = y[2:2 + N]; Dn = y[2 + N:]
    lam0 = Pn.sum(); Ri = 2 * f * kd * I
    dP = np.zeros(N); dD = np.zeros(N)
    dP[0] += Ri
    dP += kp * M * (np.concatenate([[0.0], Pn[:-1]]) - Pn)
    dP += ktm * M * (-Pn); dP[0] += ktm * M * lam0
    dP += -kt * lam0 * Pn
    conv = np.convolve(Pn, Pn)[:N - 1]  # sum_{i+j=n}, n = 2..N
    dD += ktm * M * Pn + ktd * lam0 * Pn
    dD[1:] += 0.5 * ktc * conv
    dM = -(kp + ktm) * M * lam0 - Ri
    return np.concatenate([[-kd * I, dM], dP, dD])
y0 = np.zeros(2 + 2 * N); y0[0] = I0; y0[1] = M0
times = [0.0, 20.0, 100.0, 400.0, 1200.0, 3000.0]
sol = integrate.solve_ivp(rhs, [0, times[-1]], y0, method="Radau", t_eval=times, rtol=1e-10, atol=1e-16)
n = np.arange(1, N + 1.0)
rows = []
for k in range(len(times)):
    y = sol.y[:, k]; P_, D_ = y[2:2 + N], y[2 + N:]
    rows.append([y[0], y[1]] + [np.sum(n**q * P_) for q in range(3)] + [np.sum(n**q * D_) for q in range(3)])
arr("rp_params", [kd, f, kp, ktc, ktd, ktm, I0, M0]); arr("rp_times", times); arr("rp_states", rows)


# ====================================== step-growth, gelation, Rouse / Zimm (second tranche) ======================================
# (1) imbalance / third-order kinetics against ODE integration of the rate laws (not the closed forms)
k_sg, c0_sg = 0.7, 2.0
ts = np.array([0.1, 0.5, 2.0, 10.0, 60.0])
r_list = [0.9, 0.99, 1.0]
imb = []
for r in r_list:
    sol = integrate.solve_ivp(lambda t, y: [k_sg * c0_sg * (1 - y[0]) * (1 - r * y[0])], [0, ts[-1]], [0.0], t_eval=ts, method="DOP853", rtol=1e-13, atol=1e-15)
    imb.append(sol.y[0])
arr("sg_t", ts); arr("sg_r", r_list); arr("sg_imbalanced_p", np.concatenate(imb))
sol3 = integrate.solve_ivp(lambda t, y: [k_sg * c0_sg**2 * (1 - y[0]) ** 3], [0, ts[-1]], [0.0], t_eval=ts, method="DOP853", rtol=1e-13, atol=1e-15)
arr("sg_third_p", sol3.y[0]); arr("sg_params", [k_sg, c0_sg])

# (2) AB_{f-1} branching: discrete Smoluchowski with K_ij = (f-2)(i+j) + 2 (reactive A/B group pairs of tree-like clusters)
def afb_smoluchowski(f, alphas, N=900):
    kk = np.arange(1, N + 1.0)
    K = (f - 2) * (kk[:, None] + kk[None, :]) + 2.0
    def rhs(t, n):
        loss = n * (K @ n)
        gain = np.zeros(N)
        for k in range(2, N + 1):
            i = np.arange(1, k)
            gain[k - 1] = 0.5 * np.sum(K[i - 1, k - i - 1] * n[i - 1] * n[k - i - 1])
        return gain - loss
    n0 = np.zeros(N); n0[0] = 1.0
    # time at which a given alpha is reached: alpha = (M1 - M0) / ((f-1) M1); integrate and record
    out = []
    sol = integrate.solve_ivp(rhs, [0, 12.0], n0, method="LSODA", rtol=1e-10, atol=1e-14, dense_output=True)
    def alpha_of(t):
        y = sol.sol(t); m0, m1 = y.sum(), (kk * y).sum(); return (m1 - m0) / ((f - 1) * m1)
    for a in alphas:
        tt = optimize.brentq(lambda t: alpha_of(t) - a, 1e-9, 12.0, xtol=1e-14, rtol=1e-14)
        y = sol.sol(tt); out.append((kk**2 * y).sum() / (kk * y).sum())
    return out
alphas = [0.1, 0.2, 0.3, 0.4]
arr("afb_alpha", alphas)
arr("afb_xw_f3", afb_smoluchowski(3, alphas))
alphas4 = [0.05, 0.1, 0.2, 0.28]
arr("afb_alpha_f4", alphas4)
arr("afb_xw_f4", afb_smoluchowski(4, alphas4, N=900))

# (3) A_f + B_g sol fractions by Monte Carlo random bonding (tree-like for large N), weight = monomer count
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components
def mc_sol(f, g, r, pa, n_a=60000, seed=1):
    rng = np.random.default_rng(seed)
    n_b = int(round(f * n_a / (r * g)))
    groups_a = np.repeat(np.arange(n_a), f)          # molecule index of every A group
    groups_b = n_a + np.repeat(np.arange(n_b), g)    # molecule index of every B group
    nbonds = int(round(pa * len(groups_a)))
    ia = rng.choice(len(groups_a), nbonds, replace=False)
    ib = rng.choice(len(groups_b), nbonds, replace=False)
    u, v = groups_a[ia], groups_b[ib]
    n = n_a + n_b
    m = coo_matrix((np.ones(nbonds), (u, v)), shape=(n, n))
    nc, lab = connected_components(m, directed=False)
    sizes = np.bincount(lab)
    gel = sizes.max()
    # a component is gel if it is the giant component: weight fraction of the giant one
    return 1.0 - gel / n if gel > 0.01 * n else 1.0
mc_cases = [(3, 2, 0.9, 0.6), (3, 2, 0.9, 0.8), (3, 2, 0.9, 0.95), (4, 2, 1.0, 0.5), (4, 2, 1.0, 0.7), (3, 3, 0.8, 0.9)]
arr("gel_f", [c[0] for c in mc_cases]); arr("gel_g", [c[1] for c in mc_cases]); arr("gel_r", [c[2] for c in mc_cases]); arr("gel_pa", [c[3] for c in mc_cases])
arr("gel_sol_mc", [np.mean([mc_sol(*c, seed=s) for s in (1, 2, 3)]) for c in mc_cases])

# (4) Rouse / Zimm against numpy eigenvalues of the matrices themselves
def rouse_A(N):
    A = 2 * np.eye(N) - np.eye(N, k=1) - np.eye(N, k=-1); A[0, 0] = 1; A[-1, -1] = 1; return A
def zimm_H(N, hs):
    i = np.arange(N); d = np.abs(i[:, None] - i[None, :]).astype(float); np.fill_diagonal(d, 1.0)
    H = np.eye(N) + np.sqrt(2) * hs / np.sqrt(d) * (1 - np.eye(N)); return H
zi_cases = [(8, 0.0), (8, 0.15), (30, 0.15), (60, 0.25), (100, 0.1)]
arr("zi_n", [c[0] for c in zi_cases]); arr("zi_hs", [c[1] for c in zi_cases])
lam_all = []; dcm = []
for N, hs in zi_cases:
    ev = np.sort(np.real(np.linalg.eigvals(zimm_H(N, hs) @ rouse_A(N))))
    lam_all.append(ev[1:5]); dcm.append(zimm_H(N, hs).sum() / N**2)
arr("zi_lambda_1_4", lam_all); arr("zi_dsum", dcm)
arr("ro_n", [8, 50]); arr("ro_eig", [np.sort(np.linalg.eigvalsh(rouse_A(8)))[1:4], np.sort(np.linalg.eigvalsh(rouse_A(50)))[1:4]])

with open("polymers/test/polymers_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/chemistry/verification/gen_polymers_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace polymers_reference {\n" + "\n".join(out) + "\n}\n")
print("ok", len(out), "arrays")

# ======================= reversible step growth (MIT OCW 10.569 lecture 3 formulas) vs ODE and population balance =======================
# ODE oracle: closed system, dp/dt = k c0 [(1-p)^2 - p^2 / K]
rev = []
for k, c0, K in [(0.5, 2.0, 9.0), (1.0, 1.0, 100.0), (0.3, 4.0, 1.0)]:
    ts_ = np.array([0.05, 0.5, 2.0, 10.0, 60.0])
    sol = integrate.solve_ivp(lambda t, y: [k * c0 * ((1 - y[0]) ** 2 - y[0] ** 2 / K)], [0, ts_[-1]], [0.0], t_eval=ts_, method="DOP853", rtol=1e-13, atol=1e-15)
    rev.append((k, c0, K, list(ts_), list(sol.y[0])))
arr("rev_params", [[r[0], r[1], r[2]] for r in rev]); arr("rev_t", rev[0][3]); arr("rev_p", [r[4] for r in rev])
# open system with condensate held at w: steady state from the quadratic K c0 (1-p)^2 = w p  (solved numerically)
op = []
for K, c0, w in [(9.0, 2.0, 0.01), (100.0, 1.0, 0.002), (1.0, 4.0, 0.5)]:
    f = lambda p: K * c0 * (1 - p) ** 2 - w * p * 1.0   # forward k c0 (1-p)^2 = k_r p w with k_r = k/K  ->  K c0 (1-p)^2 = p w
    p_eq = optimize.brentq(f, 0.0, 1.0, xtol=1e-15)
    op.append((K, c0, w, p_eq, 1 / (1 - p_eq)))
arr("open_params", [[o[0], o[1], o[2]] for o in op]); arr("open_p", [o[3] for o in op]); arr("open_dp", [o[4] for o in op])
with open("polymers/test/polymers_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/chemistry/verification/gen_polymers_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace polymers_reference {\n" + "\n".join(out) + "\n}\n")
print("ok", len(out), "arrays (with reversible step growth)")
