#pragma once

// Adsorption and heterogeneous catalysis, batched over lanes (pressures, temperatures, parameter sets).
//
//  * Isotherms (loading q or coverage theta as a function of pressure P):
//      Langmuir                  theta = K P / (1 + K P);   dissociative  theta = sqrt(K P) / (1 + sqrt(K P))
//      competitive Langmuir      theta_i = K_i P_i / (1 + sum_j K_j P_j)
//      Freundlich                q = K P^{1/n};   Sips  q = q_m (K P)^{1/n} / (1 + (K P)^{1/n});   Temkin  theta = (R T / b) ln(A P)
//      BET (n layers)            q / q_m = c x [1 - (n+1) x^n + n x^{n+1}] / ((1 - x) [1 + (c - 1) x - c x^{n+1}]),  x = P / P_sat   (Brunauer, Emmett
//                                & Teller 1938; n -> infinity: c x / ((1 - x)(1 - x + c x))); the linear form x / (q (1 - x)) = 1/(q_m c) + (c - 1) x /(q_m c)
//                                gives q_m and c by regression (`bet_fit`) and the specific surface area q_m N_A sigma.
//  * Isosteric heat of adsorption q_st = R T^2 (d ln P / d T)_theta (Clausius-Clapeyron); for K = K0 exp(Q / R T) it equals Q.
//  * Langmuir-Hinshelwood and Eley-Rideal rates: r_LH = k theta_A theta_B with competitive adsorption (optimal pressure ratio K_A P_A = 1 + K_B P_B),
//    r_ER = k theta_A P_B.
//  * Microkinetics of a surface mechanism by mass action with site conservation: `site_balance_network` helpers feed common::reaction (rate integration and the
//    conservation-closed Newton steady state).
// The 1938 BET paper was not obtained; every formula is verified against direct summation of the multilayer model, an independent regression, exact limits and
// numerical differentiation / optimisation.

#include "pluss/subsp/natsci/chemistry/thermo.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::chemistry::surface {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
using dynamics::Coeff;

inline Series langmuir(const Series &pressure, const Coeff &k) { const Series kp = pressure.mul(k.on(pressure)); return kp.div(kp.add(1.0)); }
inline Series langmuir_dissociative(const Series &pressure, const Coeff &k) { const Series s = pressure.mul(k.on(pressure)).sqrt(); return s.div(s.add(1.0)); }
// pressures [..., S] and constants k [S] (or [..., S]); returns theta_i [..., S]
inline Series langmuir_competitive(const Series &pressures, const Series &k) {
  const Series kp = pressures.mul(k);
  return kp.div(kp.sum(-1, true).add(1.0));
}
inline Series freundlich(const Series &pressure, const Coeff &k, const Coeff &n) { return pressure.pow(n.on(pressure).reciprocal()).mul(k.on(pressure)); }
inline Series sips(const Series &pressure, const Coeff &q_m, const Coeff &k, const Coeff &n) {
  const Series t = pressure.mul(k.on(pressure)).pow(n.on(pressure).reciprocal());
  return t.div(t.add(1.0)).mul(q_m.on(pressure));
}
inline Series temkin(const Series &pressure, const Coeff &a, const Coeff &b, const Coeff &temperature) {
  return pressure.mul(a.on(pressure)).log().mul(temperature.on(pressure)).mul(thermo::gas_constant()).div(b.on(pressure));
}
// BET relative loading q / q_m at relative pressure x for c and a finite number of layers; layers <= 0 selects the infinite-layer form
inline Series bet(const Series &x, const Coeff &c, double layers = 0.0) {
  const Series cc = c.on(x);
  if (layers <= 0.0)
    return cc.mul(x).div(x.neg().add(1.0)).div(x.neg().add(1.0).add(cc.mul(x)));
  const Series xn = x.pow(layers);
  const Series numerator = cc.mul(x).mul(xn.mul(layers + 1.0).neg().add(1.0).add(xn.mul(x).mul(layers)));
  const Series denominator = x.neg().add(1.0).mul(x.mul(cc.sub(1.0)).add(1.0).sub(cc.mul(xn).mul(x)));
  return numerator.div(denominator);
}

struct BetFit final {
  Series monolayer = pluss::series::undefined();  // q_m
  Series c = pluss::series::undefined();
  Series r_squared = pluss::series::undefined();
};
// regression of x / (q (1 - x)) on x over the last axis (data [..., N]); use points in the linear range (typically 0.05 < x < 0.35)
inline BetFit bet_fit(const Series &x, const Series &q) {
  const Series y = x.div(q.mul(x.neg().add(1.0)));
  const Series xm = x.mean(-1, true), ym = y.mean(-1, true);
  const Series sxx = x.sub(xm).square().sum(-1, false), sxy = x.sub(xm).mul(y.sub(ym)).sum(-1, false);
  const Series slope = sxy.div(sxx), intercept = ym.squeeze(-1).sub(slope.mul(xm.squeeze(-1)));
  const Series qm = slope.add(intercept).reciprocal();
  const Series c = slope.div(intercept).add(1.0);
  const Series ss_res = y.sub(slope.unsqueeze(-1).mul(x).add(intercept.unsqueeze(-1))).square().sum(-1, false), ss_tot = y.sub(ym).square().sum(-1, false);
  return {qm, c, ss_res.div(ss_tot).neg().add(1.0)};
}
// specific surface area [m^2 / kg] from the monolayer loading q_m [mol / kg] and the molecular cross-section sigma [m^2]
inline Series specific_surface_area(const Series &monolayer_mol_per_kg, double cross_section_m2) {
  return monolayer_mol_per_kg.mul(cross_section_m2 * ::pluss::subsp::natsci::common::constants::codata2022::avogadro_constant.measurement.value().si_value());
}

// Isosteric heat from a pressure-at-fixed-coverage relation P(theta, T): q_st = R T^2 d ln P / dT, central difference in T
inline Series isosteric_heat(const std::function<Series(const Series &theta, const Series &temperature)> &pressure, const Series &theta, const Series &temperature, double relative_step = 1e-5) {
  const Series h = temperature.mul(relative_step);
  const Series dlnp = pressure(theta, temperature.add(h)).log().sub(pressure(theta, temperature.sub(h)).log()).div(h.mul(2.0));
  return dlnp.mul(temperature.square()).mul(thermo::gas_constant());
}
// Langmuir pressure for a given coverage: P = theta / (K (1 - theta)), with K = K0 exp(Q / (R T))
inline Series langmuir_pressure(const Series &theta, const Series &temperature, const Coeff &k0, const Coeff &heat) {
  const Series k = temperature.mul(thermo::gas_constant()).reciprocal().mul(heat.on(temperature)).exp().mul(k0.on(temperature));
  return theta.div(k.mul(theta.neg().add(1.0)));
}

// ---- bimolecular surface reactions -----------------------------------------------------------------------------------------------
// Langmuir-Hinshelwood: r = k theta_A theta_B with competitive adsorption of A and B
inline Series langmuir_hinshelwood_rate(const Series &p_a, const Series &p_b, const Coeff &k_a, const Coeff &k_b, const Coeff &k_rate) {
  const Series ka = p_a.mul(k_a.on(p_a)), kb = p_b.mul(k_b.on(p_b));
  return ka.mul(kb).div(ka.add(kb).add(1.0).square()).mul(k_rate.on(p_a));
}
// Eley-Rideal: r = k theta_A P_B
inline Series eley_rideal_rate(const Series &p_a, const Series &p_b, const Coeff &k_a, const Coeff &k_rate) {
  const Series ka = p_a.mul(k_a.on(p_a));
  return ka.div(ka.add(1.0)).mul(p_b).mul(k_rate.on(p_a));
}

} // namespace pluss::subsp::natsci::chemistry::surface
