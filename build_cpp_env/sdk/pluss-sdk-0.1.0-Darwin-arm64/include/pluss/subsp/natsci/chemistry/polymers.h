#pragma once

// Polymer physics and polymerization kinetics, batched over lanes (every function treats leading axes as independent lanes).
//
//  * Chain statistics. Freely-jointed chain <R^2> = N b^2; freely-rotating chain (Flory, Statistical Mechanics of Chain Molecules, 1969,
//    eq. for <R^2> with fixed bond angle): <R^2> = N b^2 (1+c)/(1-c) - 2 b^2 c (1-c^N)/(1-c)^2, c = cos(theta);
//    worm-like chain (Kratky & Porod, Rec. Trav. Chim. 68 (1949) 1106): <R^2> = 2 P L [1 - (P/L)(1 - e^{-L/P})] and
//    (Benoit & Doty, J. Phys. Chem. 57 (1953) 958): <Rg^2> = P L/3 - P^2 + 2 P^3/L - 2 (P^4/L^2)(1 - e^{-L/P});
//    force-extension: freely-jointed chain x/L = coth(a) - 1/a, a = f b / kT (Kuhn & Grun 1942); worm-like chain
//    Marko & Siggia, Macromolecules 28 (1995) 8759: f P/kT = z + 1/(4 (1-z)^2) - 1/4, z = x/L (inverted by bisection).
//  * Molecular-weight distributions. Flory (1936) most probable distribution n_x = (1-p) p^{x-1}, w_x = x (1-p)^2 p^{x-1} with
//    Carothers' X_n = (1+r)/(1+r-2rp); Poisson distribution of living polymerization n_x = e^{-nu} nu^{x-1}/(x-1)!
//    (Flory 1940); averages Mn, Mw, Mz and dispersity from any discrete distribution.
//  * Free-radical polymerization by the method of moments (Odian, Principles of Polymerization, 4th ed., ch. 3; Kiparissides,
//    Chem. Eng. Sci. 51 (1996) 1637): initiator decomposition, propagation, termination by combination and disproportionation and
//    transfer to monomer; state y = [I, M, lambda_0..2 (live chains), mu_0..2 (dead chains)] with exact closed moment equations,
//    plus the quasi-steady-state results.
//  * Copolymerization: Mayo & Lewis, J. Am. Chem. Soc. 66 (1944) 1594 composition equation, azeotrope, and the composition drift with
//    conversion (Skeist 1946 ODE and its closed form, Meyer & Lowry, J. Polym. Sci. A 3 (1965) 2843).
//  * Flory-Huggins solutions (Flory 1942, Huggins 1941): free energy of mixing, chemical potentials, spinodal, critical point and the
//    binodal (coexistence) by a batched Newton solve on the two equal-chemical-potential conditions.
// Lengths in consistent units; energies in units of kT where a function says so.

#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::chemistry::polymers {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace roots = ::pluss::subsp::math::roots;
using dynamics::Coeff;

// ---- chain statistics ---------------------------------------------------------------------------------
inline Series fjc_mean_square_end_to_end(const Series &n_bonds, const Coeff &bond_length) {
  return n_bonds.mul(bond_length.on(n_bonds).square());
}

inline Series freely_rotating_mean_square_end_to_end(const Series &n_bonds, const Coeff &bond_length, const Coeff &cos_theta) {
  const Series b2 = bond_length.on(n_bonds).square();
  const Series c = cos_theta.on(n_bonds);
  const Series one_minus = c.mul(-1.0).add(1.0);
  const Series cn = c.pow(n_bonds);
  return n_bonds.mul(b2).mul(c.add(1.0)).div(one_minus).sub(b2.mul(c).mul(2.0).mul(cn.mul(-1.0).add(1.0)).div(one_minus.square()));
}

inline Series wlc_mean_square_end_to_end(const Series &contour_length, const Coeff &persistence_length) {
  const Series big_l = contour_length;
  const Series p = persistence_length.on(big_l);
  const Series x = big_l.div(p);
  // 2 P L (1 - (1 - e^-x)/x); for small x the difference cancels: L^2 (1 - x/3 + x^2/12 - x^3/60 + ...)
  const Series term = x.mul(-1.0).expm1().mul(-1.0).div(x); // (1 - e^-x)/x
  const Series exact = p.mul(big_l).mul(2.0).mul(term.mul(-1.0).add(1.0));
  const Series series = big_l.square().mul(x.mul(-1.0 / 3.0).add(1.0).add(x.square().mul(1.0 / 12.0)).sub(x.pow(3.0).mul(1.0 / 60.0)));
  return x.lt(1e-3).where(series, exact);
}

inline Series wlc_radius_of_gyration_squared(const Series &contour_length, const Coeff &persistence_length) {
  const Series big_l = contour_length;
  const Series p = persistence_length.on(big_l);
  const Series x = big_l.div(p);
  // Rg^2 / L^2 = 1/(3x) - 1/x^2 + 2/x^3 - (2/x^4)(1 - e^-x)
  const Series inv = x.reciprocal();
  const Series direct = inv.mul(1.0 / 3.0).sub(inv.square()).add(inv.pow(3.0).mul(2.0)).sub(inv.pow(4.0).mul(2.0).mul(x.mul(-1.0).expm1().mul(-1.0)));
  // small x: 1/12 + 2 sum_{m>=5} (-1)^m x^{m-4}/m!
  Series series = x.mul(0.0).add(1.0 / 12.0);
  double fact = 120.0; // 5!
  for (int m = 5; m <= 14; ++m) {
    series = series.add(x.pow(static_cast<double>(m - 4)).mul(2.0 * ((m % 2) ? -1.0 : 1.0) / fact));
    fact *= static_cast<double>(m + 1);
  }
  return x.lt(0.2).where(series, direct).mul(big_l.square());
}

// fractional extension x/L of a freely-jointed chain under force f: Langevin function of a = f b / kT
inline Series fjc_extension(const Series &force, const Coeff &kuhn_length, const Coeff &kt) {
  const Series a = force.mul(kuhn_length.on(force)).div(kt.on(force));
  const Series aa = a.abs();
  const Series sinh_a = aa.clamp(1e-12, 300.0).sinh();
  const Series cosh_a = aa.clamp(1e-12, 300.0).cosh();
  const Series large = cosh_a.div(sinh_a).sub(aa.clamp(1e-12, 300.0).reciprocal());
  const Series small = aa.mul(1.0 / 3.0).sub(aa.pow(3.0).mul(1.0 / 45.0)).add(aa.pow(5.0).mul(2.0 / 945.0)).sub(aa.pow(7.0).mul(1.0 / 4725.0));
  const Series magnitude = aa.lt(0.05).where(small, large);
  return a.lt(0.0).where(magnitude.mul(-1.0), magnitude);
}

// Marko-Siggia interpolation: force at fractional extension z in [0, 1)
inline Series wlc_force(const Series &z, const Coeff &persistence_length, const Coeff &kt) {
  const Series one_minus = z.mul(-1.0).add(1.0);
  return z.add(one_minus.square().mul(4.0).reciprocal()).sub(0.25).mul(kt.on(z)).div(persistence_length.on(z));
}

// fractional extension at force f (>= 0): root of wlc_force(z) = f on [0, 1) by bisection (monotone), all lanes together
inline Series wlc_extension(const Series &force, const Coeff &persistence_length, const Coeff &kt) {
  const Series lo = force.mul(0.0);
  const Series hi = force.mul(0.0).add(1.0 - 1e-14);
  const auto residual = [&](const Series &z) { return wlc_force(z, persistence_length, kt).sub(force); };
  roots::Options opt;
  opt.max_iterations = 80;
  return roots::bisect(residual, lo, hi, opt).root;
}

// ---- molecular-weight distributions --------------------------------------------------------------------
// x: degrees of polymerization [..., X] (broadcast against p [..., 1] or scalar)
inline Series flory_number_fraction(const Series &x, const Coeff &p) {
  const Series pp = p.on(x);
  return pp.mul(-1.0).add(1.0).mul(pp.pow(x.sub(1.0)));
}
inline Series flory_weight_fraction(const Series &x, const Coeff &p) {
  const Series pp = p.on(x);
  return x.mul(pp.mul(-1.0).add(1.0).square()).mul(pp.pow(x.sub(1.0)));
}
// Carothers equation with stoichiometric ratio r <= 1: X_n = (1 + r)/(1 + r - 2 r p)
inline Series carothers_degree_of_polymerization(const Series &conversion, const Coeff &r = 1.0) {
  const Series rr = r.on(conversion);
  return rr.add(1.0).div(rr.add(1.0).sub(rr.mul(conversion).mul(2.0)));
}
// Poisson distribution of a living polymerization with nu monomer units added per initiator (x = 1, 2, ...)
inline Series poisson_number_fraction(const Series &x, const Coeff &nu) {
  const Series v = nu.on(x);
  return v.mul(-1.0).add(x.sub(1.0).mul(v.log())).sub(x.lgamma()).exp();
}

struct MolecularWeights final {
  Series mn = pluss::series::undefined();
  Series mw = pluss::series::undefined();
  Series mz = pluss::series::undefined();
  Series dispersity = pluss::series::undefined();
};
// averages of a discrete number distribution n [..., X] over degrees x [..., X] (or [X]); repeat-unit mass m0
inline MolecularWeights molecular_weights(const Series &x, const Series &n, const Coeff &m0 = 1.0) {
  const Series s0 = n.sum(-1, false);
  const Series s1 = x.mul(n).sum(-1, false);
  const Series s2 = x.square().mul(n).sum(-1, false);
  const Series s3 = x.pow(3.0).mul(n).sum(-1, false);
  MolecularWeights out;
  const Series mass = m0.on(s0);
  out.mn = s1.div(s0).mul(mass);
  out.mw = s2.div(s1).mul(mass);
  out.mz = s3.div(s2).mul(mass);
  out.dispersity = out.mw.div(out.mn);
  return out;
}

// ---- free-radical polymerization (method of moments) ----------------------------------------------------
struct RadicalKinetics final {
  Coeff kd = 0.0;   // initiator decomposition, 1/s
  Coeff f = 0.5;    // initiator efficiency
  Coeff kp = 0.0;   // propagation
  Coeff ktc = 0.0;  // termination by combination (radical-loss rate coefficient: loss rate = ktc lambda_0^2)
  Coeff ktd = 0.0;  // termination by disproportionation
  Coeff ktm = 0.0;  // transfer to monomer
};

// y [..., 8] = [I, M, lam0, lam1, lam2, mu0, mu1, mu2]; lam_k = sum n^k [P_n.], mu_k = sum n^k [D_n]
inline dynamics::RateFn free_radical_rates(const RadicalKinetics &k) {
  return [k](const Series &y) {
    const Series i = y.select(-1, 0), m = y.select(-1, 1), l0 = y.select(-1, 2), l1 = y.select(-1, 3), l2 = y.select(-1, 4);
    const Series kd = k.kd.on(i), f = k.f.on(i), kp = k.kp.on(i), ktc = k.ktc.on(i), ktd = k.ktd.on(i), ktm = k.ktm.on(i);
    const Series kt = ktc.add(ktd);
    const Series ri = kd.mul(f).mul(i).mul(2.0); // radicals formed per time; each starts a chain of length 1
    const Series pm = kp.mul(m), tm = ktm.mul(m);
    const Series di = kd.mul(i).mul(-1.0);
    const Series dm = pm.add(tm).mul(l0).add(ri).mul(-1.0);
    const Series dl0 = ri.sub(kt.mul(l0).mul(l0));
    const Series dl1 = ri.add(pm.mul(l0)).add(tm.mul(l0.sub(l1))).sub(kt.mul(l0).mul(l1));
    const Series dl2 = ri.add(pm.mul(l1.mul(2.0).add(l0))).add(tm.mul(l0.sub(l2))).sub(kt.mul(l0).mul(l2));
    const Series dm0 = tm.mul(l0).add(ktd.mul(l0).mul(l0)).add(ktc.mul(l0).mul(l0).mul(0.5));
    const Series dm1 = tm.mul(l1).add(kt.mul(l0).mul(l1));
    const Series dm2 = tm.mul(l2).add(ktd.mul(l0).mul(l2)).add(ktc.mul(l0.mul(l2).add(l1.square())));
    const std::vector<Series> tail{dm.unsqueeze(-1), dl0.unsqueeze(-1), dl1.unsqueeze(-1), dl2.unsqueeze(-1), dm0.unsqueeze(-1), dm1.unsqueeze(-1), dm2.unsqueeze(-1)};
    return di.unsqueeze(-1).unite(tail, -1);
  };
}

// number- and weight-average molar mass of the polymer made so far (live + dead chains), repeat-unit mass m0
inline MolecularWeights radical_molecular_weights(const Series &y, const Coeff &m0 = 1.0) {
  const Series n0 = y.select(-1, 2).add(y.select(-1, 5));
  const Series n1 = y.select(-1, 3).add(y.select(-1, 6));
  const Series n2 = y.select(-1, 4).add(y.select(-1, 7));
  MolecularWeights out;
  const Series mass = m0.on(n0);
  out.mn = n1.div(n0).mul(mass);
  out.mw = n2.div(n1).mul(mass);
  out.mz = out.mw; // third moment is not tracked by this closure
  out.dispersity = out.mw.div(out.mn);
  return out;
}

// quasi-steady-state radical concentration  lam0 = sqrt(R_i / k_t),  R_i = 2 f kd [I]
inline Series qssa_radical_concentration(const Series &initiator, const RadicalKinetics &k) {
  const Series ri = k.kd.on(initiator).mul(k.f.on(initiator)).mul(initiator).mul(2.0);
  return ri.div(k.ktc.on(initiator).add(k.ktd.on(initiator))).sqrt();
}
// instantaneous polymerization rate  R_p = kp [M] lam0
inline Series qssa_polymerization_rate(const Series &monomer, const Series &initiator, const RadicalKinetics &k) {
  return k.kp.on(monomer).mul(monomer).mul(qssa_radical_concentration(initiator, k));
}
// instantaneous kinetic chain length  nu = kp [M] lam0 / R_i  (propagations per radical formed)
inline Series qssa_kinetic_chain_length(const Series &monomer, const Series &initiator, const RadicalKinetics &k) {
  const Series ri = k.kd.on(initiator).mul(k.f.on(initiator)).mul(initiator).mul(2.0);
  return qssa_polymerization_rate(monomer, initiator, k).div(ri);
}

// ---- copolymerization --------------------------------------------------------------------------------
// instantaneous copolymer composition F1 from the monomer feed fraction f1 (Mayo-Lewis)
inline Series mayo_lewis(const Series &f1, const Coeff &r1, const Coeff &r2) {
  const Series rr1 = r1.on(f1), rr2 = r2.on(f1);
  const Series f2 = f1.mul(-1.0).add(1.0);
  const Series num = rr1.mul(f1.square()).add(f1.mul(f2));
  return num.div(num.add(f1.mul(f2)).add(rr2.mul(f2.square())));
}
// azeotropic feed (F1 = f1), meaningful when both r < 1 or both r > 1
inline Series azeotrope(const Coeff &r1, const Coeff &r2, const Series &like) {
  return r2.on(like).mul(-1.0).add(1.0).div(r1.on(like).mul(-1.0).add(2.0).sub(r2.on(like)));
}
// Skeist drift: feed fraction f1 versus conversion X (batched RK4 over conversion), starting from f10
inline Series skeist_feed_fraction(const Series &f10, const Coeff &r1, const Coeff &r2, double conversion, std::int64_t steps = 400) {
  const dynamics::RateFn rate = [&](const Series &y) {
    // y = [f1, X]; d f1/dX = (f1 - F1)/(1 - X); X is carried as a state so the autonomous RK4 helper can be reused
    const Series f1 = y.select(-1, 0), x = y.select(-1, 1);
    const Series d = f1.sub(mayo_lewis(f1, r1, r2)).div(x.mul(-1.0).add(1.0));
    return d.unsqueeze(-1).unite(std::vector<Series>{d.mul(0.0).add(1.0).unsqueeze(-1)}, -1);
  };
  const Series y0 = f10.unsqueeze(-1).unite(std::vector<Series>{f10.mul(0.0).unsqueeze(-1)}, -1);
  return dynamics::simulate(rate, y0, 0.0, conversion, steps).select(-1, 0);
}
// Meyer-Lowry closed form: conversion at which the feed has drifted from f10 to f1 (r1, r2 != 1, r1 r2 < 1 branch-free formula)
inline Series meyer_lowry_conversion(const Series &f1, const Series &f10, const Coeff &r1, const Coeff &r2) {
  const Series rr1 = r1.on(f1), rr2 = r2.on(f1);
  const Series one = f1.mul(0.0).add(1.0);
  const Series alpha = rr2.div(one.sub(rr2)), beta = rr1.div(one.sub(rr1));
  const Series gamma = one.sub(rr1.mul(rr2)).div(one.sub(rr1).mul(one.sub(rr2)));
  const Series delta = one.sub(rr2).div(one.mul(2.0).sub(rr1).sub(rr2));
  const Series f2 = one.sub(f1), f20 = one.sub(f10);
  const Series ratio = f1.div(f10).pow(alpha).mul(f2.div(f20).pow(beta)).mul(f10.sub(delta).div(f1.sub(delta)).pow(gamma));
  return one.sub(ratio);
}

// ---- Flory-Huggins solutions --------------------------------------------------------------------------
// phi: volume fraction of component 2 (chain length n2); component 1 has length n1. Free energy per lattice site in kT.
inline Series fh_free_energy(const Series &phi, const Coeff &n1, const Coeff &n2, const Coeff &chi) {
  const Series one_minus = phi.mul(-1.0).add(1.0);
  return one_minus.mul(one_minus.log()).div(n1.on(phi)).add(phi.mul(phi.log()).div(n2.on(phi))).add(chi.on(phi).mul(phi).mul(one_minus));
}
// exchange chemical potentials per lattice site: m1 = mu1 / n1, m2 = mu2 / n2 (in kT)
inline Series fh_mu1_per_site(const Series &phi, const Coeff &n1, const Coeff &n2, const Coeff &chi) {
  const Series a = n1.on(phi), b = n2.on(phi);
  return phi.mul(-1.0).add(1.0).log().div(a).add(a.reciprocal().sub(b.reciprocal()).mul(phi)).add(chi.on(phi).mul(phi.square()));
}
inline Series fh_mu2_per_site(const Series &phi, const Coeff &n1, const Coeff &n2, const Coeff &chi) {
  const Series a = n1.on(phi), b = n2.on(phi);
  const Series om = phi.mul(-1.0).add(1.0);
  return phi.log().div(b).add(b.reciprocal().sub(a.reciprocal()).mul(om)).add(chi.on(phi).mul(om.square()));
}
// spinodal interaction parameter chi_s(phi) = 1/2 (1/(n1 (1-phi)) + 1/(n2 phi))
inline Series fh_spinodal_chi(const Series &phi, const Coeff &n1, const Coeff &n2) {
  return phi.mul(-1.0).add(1.0).mul(n1.on(phi)).reciprocal().add(phi.mul(n2.on(phi)).reciprocal()).mul(0.5);
}
struct FloryHugginsCritical final {
  Series phi = pluss::series::undefined();
  Series chi = pluss::series::undefined();
};
inline FloryHugginsCritical fh_critical(const Series &n1, const Series &n2) {
  const Series s1 = n1.sqrt(), s2 = n2.sqrt();
  FloryHugginsCritical out;
  out.phi = s1.div(s1.add(s2)); // volume fraction of component 2: sqrt(n1)/(sqrt(n1)+sqrt(n2))
  out.chi = s1.reciprocal().add(s2.reciprocal()).square().mul(0.5);
  return out;
}
// The two spinodal compositions for chi above critical: roots of chi = 1/2 (1/(n1 (1-phi)) + 1/(n2 phi)), i.e. the quadratic
//   2 chi n1 n2 phi (1 - phi) = n2 phi + n1 (1 - phi)
struct FloryHugginsPair final {
  Series low = pluss::series::undefined();
  Series high = pluss::series::undefined();
};
inline FloryHugginsPair fh_spinodal_compositions(const Series &chi, const Series &n1, const Series &n2) {
  // -2 chi n1 n2 phi^2 + (2 chi n1 n2 - n2 + n1) phi - n1 = 0
  const Series aq = chi.mul(n1).mul(n2).mul(-2.0);
  const Series bq = chi.mul(n1).mul(n2).mul(2.0).sub(n2).add(n1);
  const Series cq = n1.mul(-1.0);
  const Series disc = bq.square().sub(aq.mul(cq).mul(4.0)).clamp(0.0, 1e300).sqrt();
  const Series r1 = bq.mul(-1.0).add(disc).div(aq.mul(2.0)), r2 = bq.mul(-1.0).sub(disc).div(aq.mul(2.0));
  FloryHugginsPair out;
  out.low = r1.lt(r2).where(r1, r2);
  out.high = r1.lt(r2).where(r2, r1);
  return out;
}
// Binodal (coexisting) compositions for chi above critical: damped batched Newton on mu1(a) = mu1(b), mu2(a) = mu2(b),
// started outside the spinodal and continued in chi from just above the critical point in `stages` steps for robustness.
inline FloryHugginsPair fh_binodal(const Series &chi, const Series &n1, const Series &n2, int stages = 24, int newton_iterations = 40) {
  const auto crit = fh_critical(n1, n2);
  const Series one = chi.mul(0.0).add(1.0);
  const auto lo_clip = [&](const Series &v) { return v.clamp(1e-14, 1.0 - 1e-14); };
  // first stage: Landau expansion about the critical point, f = -(chi - chi_c) d^2 + f4 d^4 / 24, so d^2 = 12 (chi - chi_c) / f4
  const Series phic = crit.phi;
  const Series f4 = n1.mul(phic.mul(-1.0).add(1.0).pow(3.0)).reciprocal().add(n2.mul(phic.pow(3.0)).reciprocal()).mul(2.0);
  const double first = 1.0 / (static_cast<double>(stages) * static_cast<double>(stages));
  Series delta = chi.sub(crit.chi).mul(first).mul(12.0).div(f4).clamp(0.0, 1e300).sqrt();
  // unknowns: centre c and half-gap g (a = c - g, b = c + g). Dividing the equal-potential conditions by the gap,
  //   F_k = (m_k(b) - m_k(a)) / (2 g) = 0,
  // removes the trivial root a = b of the original equations, so Newton cannot collapse onto it.
  Series c = phic, g = delta.clamp(1e-9, 0.45);
  const Series half = one.mul(0.5);
  for (int s = 1; s <= stages; ++s) {
    const double t = static_cast<double>(s) / stages;
    const Series chi_s = crit.chi.add(chi.sub(crit.chi).mul(t * t)); // quadratic ramp: fine steps near the critical point
    for (int it = 0; it < newton_iterations; ++it) {
      const Series a = c.sub(g), b = c.add(g);
      const Series g2 = g.mul(2.0);
      const Series f1 = fh_mu1_per_site(b, n1, n2, chi_s).sub(fh_mu1_per_site(a, n1, n2, chi_s)).div(g2);
      const Series f2 = fh_mu2_per_site(b, n1, n2, chi_s).sub(fh_mu2_per_site(a, n1, n2, chi_s)).div(g2);
      const auto d1 = [&](const Series &p) { return p.mul(-1.0).add(1.0).mul(n1).reciprocal().mul(-1.0).add(n1.reciprocal().sub(n2.reciprocal())).add(chi_s.mul(p).mul(2.0)); };
      const auto d2 = [&](const Series &p) { return p.mul(n2).reciprocal().sub(n2.reciprocal().sub(n1.reciprocal())).sub(chi_s.mul(p.mul(-1.0).add(1.0)).mul(2.0)); };
      const Series d1a = d1(a), d1b = d1(b), d2a = d2(a), d2b = d2(b);
      const Series j11 = d1b.sub(d1a).div(g2), j21 = d2b.sub(d2a).div(g2);          // dF/dc
      const Series j12 = d1a.add(d1b).div(g2).sub(f1.div(g)), j22 = d2a.add(d2b).div(g2).sub(f2.div(g)); // dF/dg
      const Series det0 = j11.mul(j22).sub(j12.mul(j21));
      const Series det = det0.abs().lt(1e-300).where(det0.mul(0.0).add(1e-300), det0);
      const Series dc = f1.mul(j22).sub(f2.mul(j12)).div(det);
      const Series dg = j11.mul(f2).sub(j21.mul(f1)).div(det);
      // damped step: keep g > 0 and both compositions inside (0, 1)
      Series nc = c.sub(dc).clamp(1e-12, 1.0 - 1e-12), ng = g.sub(dg);
      const Series low_bound = g.mul(0.2), high_bound = g.mul(5.0); // relative step control on the gap
      ng = ng.lt(low_bound).where(low_bound, ng);
      ng = ng.gt(high_bound).where(high_bound, ng);
      const Series room = nc.lt(half).where(nc, nc.mul(-1.0).add(1.0)).mul(1.0 - 1e-9); // both compositions stay inside (0, 1)
      ng = ng.gt(room).where(room, ng);
      c = nc;
      g = ng;
    }
  }
  FloryHugginsPair out;
  out.low = c.sub(g);
  out.high = c.add(g);
  return out;
}

} // namespace pluss::subsp::natsci::chemistry::polymers
