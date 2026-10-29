#pragma once

// Step-growth polymerization: conversion kinetics, molecular-weight distributions and gelation (Flory-Stockmayer), batched over lanes.
//
//  * Linear step growth, equal reactivity (Flory, J. Am. Chem. Soc. 58 (1936) 1877; Odian ch. 2): second-order dp/dt = k c0 (1-p)^2,
//    i.e. X_n = 1 + k c0 t, and the self-catalysed (third-order) case dp/dt = k c0^2 (1-p)^3, X_n^2 = 1 + 2 k c0^2 t; stoichiometric
//    imbalance r = N_A/N_B <= 1: ln[(1 - r p)/(1 - p)] = (1 - r) k c_A0 t. The most probable distribution n_x = (1-p) p^{x-1}
//    (polymers.h) is the exact solution of the Smoluchowski equations with a constant kernel; the closed moment equations are
//    dM0/dt = -k M0^2, dM2/dt = 2 k M1^2, dM3/dt = 6 k M1 M2.
//  * Branched polymers AB_{f-1} (one A and f-1 B groups per monomer): X_n = 1/(1 - a(f-1)), X_w = (1 - a^2 (f-1))/(1 - a (f-1))^2 with
//    a the fraction of reacted B groups; gel point a_c = 1/(f-1) (Flory, Principles of Polymer Chemistry, 1953, ch. IX).
//  * A_f + B_g (r = N_A / N_B <= 1): critical conversion p_A,c = 1/sqrt(r (f-1)(g-1)) and, beyond it, the sol fraction from the
//    branching-process generating functions of Stockmayer (J. Chem. Phys. 11 (1943) 45, 12 (1944) 125) / Macosko & Miller (Macromolecules
//    9 (1976) 199): F_A = 1 - p_A + p_A F_B^{g-1}, F_B = 1 - p_B + p_B F_A^{f-1}, smallest root in [0, 1]; the monomer-weighted sol
//    fraction is (n_A F_A^f + n_B F_B^g)/(n_A + n_B) for equal monomer masses.

#include "pluss/subsp/natsci/chemistry/polymers.h"

#include <cmath>

namespace pluss::subsp::natsci::chemistry::step_growth {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
using dynamics::Coeff;

// ---- linear step growth -------------------------------------------------------------------------------
// conversion p(t) of a second-order (uncatalysed-by-acid) polyesterification-type reaction with c0 the initial group concentration
inline Series conversion_second_order(const Series &t, const Coeff &k, const Coeff &c0) {
  const Series kc = k.on(t).mul(c0.on(t)).mul(t);
  return kc.div(kc.add(1.0));
}
// self-catalysed (third-order) reaction: (1 - p)^-2 = 1 + 2 k c0^2 t
inline Series conversion_third_order(const Series &t, const Coeff &k, const Coeff &c0) {
  const Series c = c0.on(t);
  return t.mul(k.on(t)).mul(c.square()).mul(2.0).add(1.0).rsqrt().mul(-1.0).add(1.0);
}
// stoichiometric imbalance r < 1 (A-A + B-B with B in excess), p = conversion of the deficient group A:
//   ln[(1 - r p)/(1 - p)] = (1 - r) k c_A0 t  =>  p = (E - 1)/(E - r),  E = exp((1 - r) k c_A0 t)
inline Series conversion_imbalanced(const Series &t, const Coeff &k, const Coeff &ca0, const Coeff &r) {
  const Series rr = r.on(t);
  const Series kct = k.on(t).mul(ca0.on(t)).mul(t);
  const Series e = rr.mul(-1.0).add(1.0).mul(kct).exp();
  const Series generic = e.sub(1.0).div(e.sub(rr));
  const Series equimolar = kct.div(kct.add(1.0)); // r -> 1
  return rr.sub(1.0).abs().lt(1e-12).where(equimolar, generic);
}
// closed moment equations of the equal-reactivity linear system: y = [M0, M1, M2, M3] (moments of the chain-length distribution)
inline dynamics::RateFn linear_moment_rates(const Coeff &k) {
  return [k](const Series &y) {
    const Series m0 = y.select(-1, 0), m1 = y.select(-1, 1), m2 = y.select(-1, 2);
    const Series kk = k.on(m0);
    const Series d0 = kk.mul(m0).mul(m0).mul(-1.0);
    const Series d1 = m0.mul(0.0);
    const Series d2 = kk.mul(m1).mul(m1).mul(2.0);
    const Series d3 = kk.mul(m1).mul(m2).mul(6.0);
    return d0.unsqueeze(-1).unite(std::vector<Series>{d1.unsqueeze(-1), d2.unsqueeze(-1), d3.unsqueeze(-1)}, -1);
  };
}

// ---- reversible (equilibrium-limited) step growth -----------------------------------------------------------------------------
// Condensation of equimolar functional groups A + B <-> C + W (W = the condensate, e.g. water) with equilibrium constant
// K = [C][W]/([A][B]) = k / k_r (MIT OpenCourseWare 10.569 Synthesis of Polymers, Fall 2006, Prof. P. Hammond, lecture 3, read from the
// PDF, SHA-256 6f8c4879...; Odian ch. 2). With p the conversion and c0 the initial group concentration:
//     closed system ([W] = c0 p):   K = p^2/(1-p)^2,  p_eq = sqrt(K)/(1 + sqrt(K)),  X_n = 1 + sqrt(K)
//     condensate held at [W]:       K = X_n (X_n - 1) [W]/c0   =>   [W]_eq = K c0/(X_n (X_n - 1)),  X_n = (1 + sqrt(1 + 4 K c0/[W]))/2
// and the kinetics dp/dt = k c0 (1-p)^2 - k_r p [W]. The molecular-weight distribution stays the most probable one (random scission is the
// reverse of random coupling) with p(t).
inline Series equilibrium_conversion_closed(const Series &k_eq) { const Series r = k_eq.sqrt(); return r.div(r.add(1.0)); }
inline Series equilibrium_dp_closed(const Series &k_eq) { return k_eq.sqrt().add(1.0); }
inline Series equilibrium_condensate(const Series &dp, const Series &k_eq, const Coeff &c0) { return k_eq.mul(c0.on(dp)).div(dp.mul(dp.sub(1.0))); }
inline Series equilibrium_dp_open(const Series &k_eq, const Series &condensate, const Coeff &c0) {
  return k_eq.mul(c0.on(k_eq)).div(condensate).mul(4.0).add(1.0).sqrt().add(1.0).mul(0.5);
}
// state y = [p, w]: p conversion, w condensate concentration. Condensate is produced with the bonds (c0 dp/dt) and removed by a
// first-order mass-transfer step `removal` (1/s; 0 = closed system).
inline dynamics::RateFn reversible_rates(const Coeff &k_forward, const Coeff &k_eq, const Coeff &c0, const Coeff &removal = 0.0) {
  return [=](const Series &y) {
    const Series p = y.select(-1, 0), w = y.select(-1, 1);
    const Series kf = k_forward.on(p), cc = c0.on(p);
    const Series kr = kf.div(k_eq.on(p));
    const Series dp = kf.mul(cc).mul(p.mul(-1.0).add(1.0).square()).sub(kr.mul(p).mul(w));
    const Series dw = cc.mul(dp).sub(removal.on(p).mul(w));
    return dp.unsqueeze(-1).unite(std::vector<Series>{dw.unsqueeze(-1)}, -1);
  };
}

// ---- AB_{f-1} branching (a = fraction of reacted B groups, gel at a = 1/(f-1)) ----------------------------
inline Series afb_number_average(const Series &a, const Coeff &f) {
  return a.mul(f.on(a).sub(1.0)).mul(-1.0).add(1.0).reciprocal();
}
inline Series afb_weight_average(const Series &a, const Coeff &f) {
  const Series fm = f.on(a).sub(1.0);
  return a.square().mul(fm).mul(-1.0).add(1.0).div(a.mul(fm).mul(-1.0).add(1.0).square());
}
inline Series afb_critical_conversion(const Coeff &f, const Series &like) { return f.on(like).sub(1.0).reciprocal(); }

// ---- A_f + B_g gelation -------------------------------------------------------------------------------
// critical conversion of the A groups, 1 / sqrt(r (f-1)(g-1))
inline Series critical_conversion(const Coeff &f, const Coeff &g, const Series &r) {
  return r.mul(f.on(r).sub(1.0)).mul(g.on(r).sub(1.0)).rsqrt();
}
struct SolGel final {
  Series sol_fraction = pluss::series::undefined(); // weight fraction of the sol (finite molecules); 1 below the gel point
  Series gel_fraction = pluss::series::undefined();
  Series f_a = pluss::series::undefined();          // probability that an A bond leads to a finite branch
  Series f_b = pluss::series::undefined();
};
// pa = conversion of A groups (0..1), r = N_A/N_B (<= 1 so that pb = r pa <= 1), functionalities f (A_f) and g (B_g). Equal monomer
// masses. The smallest root of F = G(F) in [0, 1) is found by batched bisection (G is convex, so there is at most one root below 1).
inline SolGel sol_gel(const Series &pa, const Series &r, const Coeff &f, const Coeff &g, int iterations = 80) {
  const double fm = 1.0, gm = 1.0;
  (void)fm; (void)gm;
  const Series pb = pa.mul(r);
  const Series fa1 = f.on(pa).sub(1.0), gb1 = g.on(pa).sub(1.0);
  const auto fb_of = [&](const Series &fa) { return pb.mul(-1.0).add(1.0).add(pb.mul(fa.pow(fa1))); };
  const auto h = [&](const Series &fa) { return pa.mul(-1.0).add(1.0).add(pa.mul(fb_of(fa).pow(gb1))).sub(fa); };
  const Series gel = pa.mul(pb).mul(fa1).mul(gb1).gt(1.0);
  Series lo = pa.mul(0.0), hi = pa.mul(0.0).add(1.0 - 1e-12);
  for (int it = 0; it < iterations; ++it) {
    const Series mid = lo.add(hi).mul(0.5);
    const Series positive = h(mid).gt(0.0); // h(0) > 0: the root lies above `mid`
    lo = positive.where(mid, lo);
    hi = positive.where(hi, mid);
  }
  const Series root = lo.add(hi).mul(0.5);
  SolGel out;
  out.f_a = gel.where(root, pa.mul(0.0).add(1.0));
  out.f_b = fb_of(out.f_a);
  // numbers of molecules: N_A = f n1, N_B = g n2 = N_A / r  =>  n2 / n1 = f / (r g)
  const Series n1 = pa.mul(0.0).add(1.0);
  const Series n2 = f.on(pa).div(r.mul(g.on(pa)));
  out.sol_fraction = n1.mul(out.f_a.pow(f.on(pa))).add(n2.mul(out.f_b.pow(g.on(pa)))).div(n1.add(n2));
  out.gel_fraction = out.sol_fraction.mul(-1.0).add(1.0);
  return out;
}

} // namespace pluss::subsp::natsci::chemistry::step_growth
