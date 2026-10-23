"""Reference values for biology/systems/enzymes.h (writes systems/test/enzymes_reference.h).

Independent oracles:
  * Lambert W: mpmath (50 digits) evaluated at W(exp(z)) so the huge arguments of progress curves are exact;
  * mechanism engine: sympy EXACT rational steady states (symbolic King-Altman by solving the linear system), and scipy null_space;
  * Morrison: numerical solution of the binding equilibrium (brentq);
  * progress curves: full mass-action E + S <-> C -> E + P integrated with scipy Radau at tight tolerance.
"""
import numpy as np, mpmath as mp, sympy as sp
from scipy import integrate, optimize, linalg

out = []
def arr(name, v):
    out.append("inline const std::vector<double> %s = {%s};" % (name, ",".join(repr(float(x)) for x in np.ravel(v))))

# ---- Lambert W
mp.mp.dps = 60
zs = [-40.0, -5.0, -1.0, 0.0, 0.5, 1.0, 2.0, 5.0, 20.0, 100.0, 700.0]
arr("lw_z", zs); arr("lw_w", [float(mp.lambertw(mp.e ** mp.mpf(z))) for z in zs])

# ---- mechanism engine: exact steady states with sympy
def exact_flux(n_states, steps, conc, E_T=1):
    """steps: list of (from, to, k, species_index or None, product_sign); conc: dict index->value (rational)."""
    Q = sp.zeros(n_states, n_states)
    for fr, to, k, sp_i, ps in steps:
        rate = sp.Rational(k) * (sp.Rational(conc[sp_i]) if sp_i is not None else 1)
        Q[to, fr] += rate; Q[fr, fr] -= rate
    M = Q.copy()
    for j in range(n_states): M[n_states - 1, j] = 1
    rhs = sp.zeros(n_states, 1); rhs[n_states - 1] = 1
    pi = M.LUsolve(rhs)
    v = 0
    for fr, to, k, sp_i, ps in steps:
        if ps:
            rate = sp.Rational(k) * (sp.Rational(conc[sp_i]) if sp_i is not None else 1)
            v += ps * rate * pi[fr]
    return float(v), [float(x) for x in pi]

R = sp.Rational
# (a) Michaelis-Menten: E + S <-> ES -> E + P
mm_steps = [(0, 1, R(10), 0, 0), (1, 0, R(1), None, 0), (1, 0, R(1, 2), None, 1)]
# NOTE: the catalytic return ES -> E carries the product (+1) and is a separate step from the unbinding
mm_cases = [R(1, 10), R(1, 2), 2, 10]
arr("mm_S", [float(c) for c in mm_cases]); arr("mm_v", [exact_flux(2, mm_steps, {0: c})[0] for c in mm_cases])

# (b) ordered bi-bi with all reverse steps: states E, EA, EAB, EPQ, EQ ; concentrations [A, B, P, Q]
ob = [(0, 1, R(20), 0, 0), (1, 0, R(3), None, 0),
      (1, 2, R(15), 1, 0), (2, 1, R(2), None, 0),
      (2, 3, R(8), None, 0), (3, 2, R(1), None, 0),
      (3, 4, R(6), None, 1), (4, 3, R(4), 2, -1),
      (4, 0, R(9), None, 0), (0, 4, R(2), 3, 0)]
ob_conc = [{0: R(1, 10), 1: R(1, 5), 2: R(0), 3: R(0)}, {0: R(1, 2), 1: R(2), 2: R(1, 20), 3: R(1, 10)}, {0: R(3), 1: R(1, 3), 2: R(1), 3: R(1, 2)}, {0: R(1, 20), 1: R(5), 2: R(0), 3: R(0)}]
arr("ob_conc", [[float(c[i]) for i in range(4)] for c in ob_conc]); arr("ob_v", [exact_flux(5, ob, c)[0] for c in ob_conc])
arr("ob_pi_first", exact_flux(5, ob, ob_conc[0])[1])

# (c) ping-pong with reverse steps: states E, EA, F, FB ; concentrations [A, B, P, Q]
pp = [(0, 1, R(12), 0, 0), (1, 0, R(2), None, 0),
      (1, 2, R(7), None, 1), (2, 1, R(3), 2, -1),
      (2, 3, R(10), 1, 0), (3, 2, R(4), None, 0),
      (3, 0, R(5), None, 0), (0, 3, R(1), 3, 0)]
pp_conc = [{0: R(1, 4), 1: R(1, 2), 2: R(0), 3: R(0)}, {0: R(2), 1: R(1, 5), 2: R(1, 10), 3: R(1, 10)}, {0: R(5), 1: R(5), 2: R(0), 3: R(0)}]
arr("pp_conc", [[float(c[i]) for i in range(4)] for c in pp_conc]); arr("pp_v", [exact_flux(4, pp, c)[0] for c in pp_conc])
# initial-rate Cleland form check parameters (no products): k's of the irreversible steps
arr("pp_irrev", [12.0, 2.0, 7.0, 10.0, 4.0, 5.0])

# (d) dead-end complexes: uncompetitive / mixed / substrate inhibition from the full steady state
k1, km1, kcat = 10.0, 1.0, 0.5
dead = []
for kon, koff in [(4.0, 2.0), (30.0, 6.0)]:
    for S in [0.05, 0.4, 3.0]:
        for I in [0.0, 0.7, 5.0]:
            # states E, ES, ESI : ESI dead end off ES
            steps = [(0, 1, R(k1).limit_denominator(10**6), 0, 0), (1, 0, R(km1), None, 0), (1, 0, R(kcat).limit_denominator(10**6), None, 1),
                     (1, 2, R(kon), 1, 0), (2, 1, R(koff), None, 0)]
            dead.append((kon, koff, S, I, exact_flux(3, steps, {0: R(S).limit_denominator(10**6), 1: R(I).limit_denominator(10**6)})[0]))
arr("dead_params", [[d[0], d[1], d[2], d[3]] for d in dead]); arr("dead_v", [d[4] for d in dead])
subs = []
for S in [0.05, 0.3, 1.0, 4.0, 20.0]:
    steps = [(0, 1, R(k1), 0, 0), (1, 0, R(km1), None, 0), (1, 0, R(1, 2), None, 1), (1, 2, R(6), 0, 0), (2, 1, R(18), None, 0)]  # ESS dead end, Ksi = 18/6 = 3
    subs.append((S, exact_flux(3, steps, {0: R(S).limit_denominator(10**6)})[0]))
arr("subinh_S", [x[0] for x in subs]); arr("subinh_v", [x[1] for x in subs])

# ---- Morrison tight binding: solve E + I <-> EI numerically
def morrison_numeric(E, I, Ki):
    f = lambda ei: (E - ei) * (I - ei) - Ki * ei
    ei = optimize.brentq(f, 0.0, min(E, I), xtol=1e-16, rtol=1e-15)
    return 1 - ei / E
mo = [(1.0, 0.5, 0.1), (1.0, 2.0, 0.01), (0.1, 0.1, 1e-3), (5.0, 1.0, 100.0), (2.0, 10.0, 1e-6)]
arr("mor_E", [m[0] for m in mo]); arr("mor_I", [m[1] for m in mo]); arr("mor_K", [m[2] for m in mo]); arr("mor_a", [morrison_numeric(*m) for m in mo])

# ---- progress curves: full mass action vs Schnell-Mendoza / tQSSA
k1, km1, kcat = 10.0, 1.0, 0.5
Km = (km1 + kcat) / k1
def full(E0, S0, times):
    f = lambda t, y: [-k1 * y[0] * y[1] + km1 * y[2], -k1 * y[0] * y[1] + (km1 + kcat) * y[2], k1 * y[0] * y[1] - (km1 + kcat) * y[2], kcat * y[2]]
    sol = integrate.solve_ivp(f, [0, times[-1]], [S0, E0, 0.0, 0.0], method="Radau", t_eval=times, rtol=1e-12, atol=1e-16)
    return sol.y
times = [0.0, 1.0, 5.0, 20.0, 60.0, 150.0, 400.0]
for tag, E0, S0 in [("pcA", 0.002, 1.0), ("pcB", 0.8, 1.0)]:
    y = full(E0, S0, times)
    arr(tag + "_p", [k1, km1, kcat, E0, S0]); arr(tag + "_t", times)
    arr(tag + "_S", y[0]); arr(tag + "_ST", y[0] + y[2])
with open("systems/test/enzymes_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/biology/verification/gen_enzymes_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace enzymes_reference {\n" + "\n".join(out) + "\n}\n")
print("ok", len(out), "arrays")

# ======================= biochemical thermodynamics (transformed Gibbs energies) =======================
Rgas = 8.314462618
def species_ladder(dgf_prot, n_h, pkas, T):
    """all species of a group: [(dGf0, N_H)] from the fully protonated form by explicit stepwise deprotonation."""
    out = [(dgf_prot, n_h)]; cum = 0.0
    for j, pk in enumerate(pkas, start=1):
        cum += pk
        out.append((dgf_prot + Rgas * T * np.log(10) * cum, n_h - j))
    return out
groups = {"ATP": (-2768100.0, 1, [6.5]), "ADP": (-1906100.0, 1, [6.4]), "Pi": (-1137300.0, 3, [2.15, 7.20, 12.35]), "H2O": (-237190.0, 2, [])}
thermo = []
for T in [298.15, 310.15]:
    for pH in [5.0, 6.0, 7.0, 7.4, 8.0, 9.0]:
        # species-level equilibrium between the MOST DEPROTONATED species of each group, then total-concentration ratio by explicit fractions
        lad = {k: species_ladder(v[0], v[1], v[2], T) for k, v in groups.items()}
        def frac_total_over_species(name, idx):
            # [sum of all species] / [species idx] at this pH, from explicit stepwise equilibria  [H_{n-j}A] / [H_{n-j+1}A] = 10^{pH - pKa_j}
            pk = groups[name][2]; ratios = [1.0]
            for j in range(1, len(pk) + 1): ratios.append(ratios[-1] * 10 ** (pH - pk[j - 1]))
            return sum(ratios) / ratios[idx]
        a = lad["ATP"][-1]; d = lad["ADP"][-1]; p = lad["Pi"][-1]; w = lad["H2O"][-1]
        dgr_species = d[0] + p[0] - a[0] - w[0]
        nuH = a[1] + w[1] - d[1] - p[1]                       # protons released by A + w -> D + P + nuH H+
        lnK_species = -dgr_species / (Rgas * T)
        # [D][P]/[A] (species) = K_s [H+]^(-nuH)
        lnRatio = lnK_species - nuH * np.log(10) * (-pH) * 1.0
        # careful: [D][P]/([A]) * [H+]^{nuH} = K_s  ->  [D][P]/[A] = K_s * [H+]^{-nuH} = K_s * 10^{pH nuH}
        lnRatio = lnK_species + nuH * np.log(10) * pH
        tot = np.log(frac_total_over_species("ADP", len(groups["ADP"][2]))) + np.log(frac_total_over_species("Pi", len(groups["Pi"][2]))) - np.log(frac_total_over_species("ATP", len(groups["ATP"][2])))
        thermo.append((T, pH, lnRatio + tot))   # ln K' = ln([sumD][sumP]/[sumA]) with water at unit activity
arr("th_T", [t[0] for t in thermo]); arr("th_pH", [t[1] for t in thermo]); arr("th_lnK", [t[2] for t in thermo])
arr("th_dgf0", [groups[k][0] for k in ["ATP", "ADP", "Pi", "H2O"]]); arr("th_nh", [groups[k][1] for k in ["ATP", "ADP", "Pi", "H2O"]])
with open("systems/test/enzymes_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/biology/verification/gen_enzymes_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace enzymes_reference {\n" + "\n".join(out) + "\n}\n")
print("ok", len(out), "arrays (with thermodynamics)")

# ======================= transcription of eQuilibrator's functions (equilibrator-cache, thermodynamic_constants.py) =======================
R_EQ = 8.314462618e-3  # eQuilibrator itself uses 8.31e-3; the CODATA value is used here and in the C++ (formulas otherwise identical)
def debye_hueckel_eq(sqrt_I, T):
    a1, a2, a3, B = 9.20483e-3, 1.284668e-5, 4.95199e-8, 1.6
    alpha = a1 * T - a2 * T**2 + a3 * T**3
    return alpha / (1.0 / sqrt_I + B)
def legendre_eq(pH, pMg, I, T, charge, nH, nMg):
    RT = R_EQ * T; LOG10 = np.log(10)
    proton = nH * RT * LOG10 * pH
    dgmg = (T / 298.15) * -455.3 + (1.0 - T / 298.15) * -467.00
    mg = nMg * (RT * LOG10 * pMg - dgmg)
    isterm = debye_hueckel_eq(I ** 0.5, T) * (charge**2 - nH - 4 * nMg) if I > 0 else 0.0
    return (proton + mg - isterm) / RT
def group_transform_eq(ms, pH, pMg, I, T):
    # ms: list of (charge, nH, nMg, ddg_over_rt298); eQuilibrator: -RT * logaddexp-reduce of -(ddg*298.15/T + legendre)
    vals = [-(m[3] * 298.15 / T + legendre_eq(pH, pMg, I, T, m[0], m[1], m[2])) for m in ms]
    return -R_EQ * T * np.logaddexp.reduce(vals) * 1000.0   # J/mol
# an ATP-like group: microspecies (charge, nH, nMg, ddG/RT relative to the major one at 298.15 K)
ms_atp = [(-4, 1, 0, 0.0), (-3, 2, 0, 3.0), (-2, 3, 0, 8.5), (-2, 1, 1, -1.5), (-1, 2, 1, 2.0)]
cases = [(7.0, 3.0, 0.25, 298.15), (6.5, 2.0, 0.10, 310.15), (8.0, 4.0, 0.0, 298.15), (7.4, 10.0, 0.05, 303.15), (5.5, 3.0, 0.35, 293.15)]
arr("eq_ms", [[m[0], m[1], m[2], m[3]] for m in ms_atp]); arr("eq_cases", [list(c) for c in cases])
arr("eq_group_gibbs", [group_transform_eq(ms_atp, pH, pMg, I, T) for pH, pMg, I, T in cases])
arr("eq_dh", [debye_hueckel_eq(np.sqrt(c[2]), c[3]) * 1000.0 for c in cases])
with open("systems/test/enzymes_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/biology/verification/gen_enzymes_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace enzymes_reference {\n" + "\n".join(out) + "\n}\n")
print("ok", len(out), "arrays (with eQuilibrator transcription)")

# ======================= MWC (Einav & Phillips 2017, arXiv:1701.06122): independent statistical-mechanics enumeration =======================
import itertools
def mwc_enumerate(c, m, KA, KI, beta_eps):
    """sum over ALL 2 * 2^m microstates (conformation x occupancy of each distinguishable site) with Boltzmann weights."""
    wa = sum((c / KA) ** sum(s) for s in itertools.product([0, 1], repeat=m))
    wi = np.exp(-beta_eps) * sum((c / KI) ** sum(s) for s in itertools.product([0, 1], repeat=m))
    return wa / (wa + wi)
def mwc_velocity_enumerate(c, m, KA, KI, beta_eps, b):
    """mean number of occupied sites in the active (and b * inactive) ensemble, per site."""
    wa = [(c / KA) ** sum(s) for s in itertools.product([0, 1], repeat=m)]; occ = [sum(s) for s in itertools.product([0, 1], repeat=m)]
    wi = [np.exp(-beta_eps) * (c / KI) ** sum(s) for s in itertools.product([0, 1], repeat=m)]
    num = sum(w * o for w, o in zip(wa, occ)) + b * sum(w * o for w, o in zip(wi, occ)); den = sum(wa) + sum(wi)
    return num / den / m
mw_cases = [(4, 1e-4, 1e-3, -6.0, 0.0), (2, 0.1e-9, 60e-6, -14.6, 0.0), (3, 2.0, 50.0, -3.0, 0.05), (1, 1.0, 5.0, -1.0, 0.0), (5, 0.5, 20.0, -9.0, 0.1)]
cgrid = np.logspace(-12, 2, 29)
arr("mwc_cases", [list(c) for c in mw_cases]); arr("mwc_c", cgrid)
arr("mwc_p", [[mwc_enumerate(c, m, KA, KI, be) for c in cgrid] for m, KA, KI, be, b in mw_cases])
arr("mwc_v", [[mwc_velocity_enumerate(c, m, KA, KI, be, b) for c in cgrid] for m, KA, KI, be, b in mw_cases])
chars = []
for m, KA, KI, be, b in mw_cases:
    pmin = 1 / (1 + np.exp(-be)); pmax = 1 / (1 + np.exp(-be) * (KA / KI) ** m)
    f = lambda lc: (mwc_enumerate(np.exp(lc), m, KA, KI, be) - pmin) / (pmax - pmin) - 0.5
    lc50 = optimize.brentq(f, np.log(KA) - 40, np.log(KA) + 40, xtol=1e-14)
    norm = lambda lc: (mwc_enumerate(np.exp(lc), m, KA, KI, be) - pmin) / (pmax - pmin)
    hstep = 1e-5
    hill = 2 * (np.log(norm(lc50 + hstep)) - np.log(norm(lc50 - hstep))) / (2 * hstep)
    chars.append((pmin, pmax, np.exp(lc50), hill))
arr("mwc_chars", [list(c) for c in chars])
# rapid-equilibrium limit of the full mass-action mechanism through the engine: lumped states (conformation, occupancy), m = 3
# kinetic constants: binding of the j-th site (m - j) kon c, unbinding j koff; conformational exchange R_j <-> T_j; catalysis j kcat from R_j
with open("systems/test/enzymes_reference.h", "w") as fh:
    fh.write("// Generated by NatSci/biology/verification/gen_enzymes_ref.py; do not edit.\n#pragma once\n#include <vector>\nnamespace enzymes_reference {\n" + "\n".join(out) + "\n}\n")
print("ok", len(out), "arrays (with MWC)")
