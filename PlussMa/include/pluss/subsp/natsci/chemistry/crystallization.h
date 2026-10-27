#pragma once

// Crystallization: nucleation theory, solid-state transformation kinetics and the continuous (MSMPR) crystallizer, batched over lanes.
//
//  * Classical nucleation theory (Volmer & Weber 1926; Becker & Doring 1935; Gibbs): with supersaturation ratio S, interfacial energy gamma, molecular volume v,
//        critical radius  r* = 2 gamma v / (k T ln S),   barrier  dG* = 16 pi gamma^3 v^2 / (3 (k T ln S)^2),   rate  J = A exp(-dG* / k T).
//  * Johnson-Mehl-Avrami-Kolmogorov kinetics: X(t) = 1 - exp(-k t^n); for homogeneous nucleation at constant rate I with 3-D interface-controlled growth at constant
//    speed G the exponent is n = 4 and k = pi I G^3 / 3 (derived from the extended volume and checked by a Monte Carlo simulation); the Avrami plot ln(-ln(1 - X)) vs ln t has
//    slope n.
//  * MSMPR crystallizer (mixed-suspension mixed-product removal, Randolph & Larson): population balance dn/dt + d(G n)/dL = -n/tau with nucleation flux B = G n(0) and solute balance
//        dc/dt = (c_in - c)/tau - 3 rho_c k_v G mu_2,        mu_j = int L^j n dL,
//    kinetics G = k_g (c - c_sat)^g, B = k_b (c - c_sat)^b mu_3^j. Size-independent growth closes the moment equations
//        dmu_0/dt = B - mu_0/tau,    dmu_j/dt = j G mu_{j-1} - mu_j/tau  (j = 1, 2, 3),
//    and the steady state is exponential, n(L) = (B/G) exp(-L/(G tau)), mu_j = j! n0 (G tau)^{j+1}. For j = 1 the supersaturation is explicit:
//        (c - c_sat) = (6 k_b k_g^3 tau^4)^{-1/(b + 3 g)},   B = (c_in - c) / (6 rho_c k_v G^3 tau^4).
//    The finite-volume population-balance solver of common::population_balance (MUSCL + SSP-RK3) integrates the full distribution with the same kinetics.
// Randolph & Larson's monograph and the original nucleation papers were not obtained; everything is verified against exact steady states, an independent moment-ODE
// integration, a closed-form nucleation sum and a Monte Carlo transformation simulation.

#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"
#include "pluss/subsp/natsci/common/population_balance.h"

#include <cmath>

namespace pluss::subsp::natsci::chemistry::crystallization {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace pbe = ::pluss::subsp::natsci::common::population_balance;
using dynamics::Coeff;

inline double boltzmann() { return ::pluss::subsp::natsci::common::constants::codata2022::boltzmann_constant.measurement.value().si_value(); }

// ---- classical nucleation theory ---------------------------------------------------------------------------------------------------
inline Series critical_radius(const Series &supersaturation, const Coeff &gamma, const Coeff &molecular_volume, const Coeff &temperature) {
  return gamma.on(supersaturation).mul(molecular_volume.on(supersaturation)).mul(2.0).div(temperature.on(supersaturation).mul(boltzmann()).mul(supersaturation.log()));
}
inline Series nucleation_barrier(const Series &supersaturation, const Coeff &gamma, const Coeff &molecular_volume, const Coeff &temperature) {
  const Series kt_ln_s = temperature.on(supersaturation).mul(boltzmann()).mul(supersaturation.log());
  return gamma.on(supersaturation).pow(3.0).mul(molecular_volume.on(supersaturation).square()).mul(16.0 * M_PI / 3.0).div(kt_ln_s.square());
}
inline Series nucleation_rate(const Series &supersaturation, const Coeff &prefactor, const Coeff &gamma, const Coeff &molecular_volume, const Coeff &temperature) {
  return nucleation_barrier(supersaturation, gamma, molecular_volume, temperature).div(temperature.on(supersaturation).mul(boltzmann())).neg().exp().mul(prefactor.on(supersaturation));
}

// ---- JMAK -----------------------------------------------------------------------------------------------------------------------------
inline Series jmak_fraction(const Series &t, const Coeff &k, const Coeff &n) { return t.pow(n.on(t)).mul(k.on(t)).neg().exp().neg().add(1.0); }
inline double jmak_constant_nucleation_3d(double nucleation_rate, double growth_speed) { return M_PI * nucleation_rate * growth_speed * growth_speed * growth_speed / 3.0; }
struct AvramiFit final {
  Series exponent = pluss::series::undefined(), rate = pluss::series::undefined();
};
// regression of ln(-ln(1 - X)) on ln t over the last axis: slope = n, intercept = ln k
inline AvramiFit avrami_fit(const Series &t, const Series &x) {
  const Series lx = t.log(), ly = x.neg().add(1.0).log().neg().log();
  const Series mx = lx.mean(-1, true), my = ly.mean(-1, true);
  const Series slope = lx.sub(mx).mul(ly.sub(my)).sum(-1, false).div(lx.sub(mx).square().sum(-1, false));
  return {slope, my.squeeze(-1).sub(slope.mul(mx.squeeze(-1))).exp()};
}

// ---- MSMPR ----------------------------------------------------------------------------------------------------------------------------
struct Msmpr final {
  Coeff residence_time{1.0};     // tau
  Coeff feed_concentration{1.0}; // c_in
  Coeff saturation{0.0};         // c_sat
  Coeff growth_constant{1.0};    // k_g
  Coeff growth_order{1.0};       // g
  Coeff nucleation_constant{1.0}; // k_b
  Coeff nucleation_order{1.0};   // b
  double magma_order{1.0};       // j (exponent of the third moment in the nucleation rate)
  Coeff crystal_density{1.0};    // rho_c
  Coeff shape_factor{1.0};       // k_v (volume = k_v L^3)
};
inline Series driving_force(const Series &c, const Msmpr &p) { return c.sub(p.saturation.on(c)).clamp(0.0, 1e300); }
inline Series growth_rate(const Series &c, const Msmpr &p) { return driving_force(c, p).pow(p.growth_order.on(c)).mul(p.growth_constant.on(c)); }
inline Series birth_rate(const Series &c, const Series &mu3, const Msmpr &p) {
  return driving_force(c, p).pow(p.nucleation_order.on(c)).mul(p.nucleation_constant.on(c)).mul(mu3.clamp(0.0, 1e300).pow(p.magma_order));
}
// moment-equation state [c, mu0, mu1, mu2, mu3]  ->  rates
inline Series moment_rate(const Series &state, const Msmpr &p) {
  const Series c = state.select(-1, 0), mu0 = state.select(-1, 1), mu1 = state.select(-1, 2), mu2 = state.select(-1, 3), mu3 = state.select(-1, 4);
  const Series tau = p.residence_time.on(c), g = growth_rate(c, p), b = birth_rate(c, mu3, p);
  const Series dc = p.feed_concentration.on(c).sub(c).div(tau).sub(p.crystal_density.on(c).mul(p.shape_factor.on(c)).mul(g).mul(mu2).mul(3.0));
  const Series d0 = b.sub(mu0.div(tau)), d1 = g.mul(mu0).sub(mu1.div(tau)), d2 = g.mul(mu1).mul(2.0).sub(mu2.div(tau)), d3 = g.mul(mu2).mul(3.0).sub(mu3.div(tau));
  return dc.unsqueeze(-1).unite(std::vector<Series>{d0.unsqueeze(-1), d1.unsqueeze(-1), d2.unsqueeze(-1), d3.unsqueeze(-1)}, state.dim() - 1);
}
struct SteadyState final {
  Series concentration = pluss::series::undefined();
  Series growth = pluss::series::undefined();
  Series nucleation = pluss::series::undefined();
  Series n0 = pluss::series::undefined(); // zero-size density B / G
};
// closed-form steady state for magma_order = 1
inline SteadyState steady_state(const Series &like, const Msmpr &p) {
  dynamics::require(std::abs(p.magma_order - 1.0) < 1e-14, "msmpr::steady_state: the closed form needs magma_order = 1");
  const Series tau = p.residence_time.on(like), kb = p.nucleation_constant.on(like), kg = p.growth_constant.on(like), b = p.nucleation_order.on(like), g = p.growth_order.on(like);
  const Series delta = kb.mul(kg.pow(3.0)).mul(tau.pow(4.0)).mul(6.0).pow(b.add(g.mul(3.0)).reciprocal().neg());
  const Series c = p.saturation.on(like).add(delta);
  const Series growth = kg.mul(delta.pow(g));
  const Series birth = p.feed_concentration.on(like).sub(c).div(p.crystal_density.on(like).mul(p.shape_factor.on(like)).mul(growth.pow(3.0)).mul(tau.pow(4.0)).mul(6.0));
  return {c, growth, birth, birth.div(growth)};
}
inline Series exponential_density(const Series &size, const Series &n0, const Series &growth, const Coeff &tau) { return n0.mul(size.div(growth.mul(tau.on(growth))).neg().exp()); }
// analytic moments of the steady state: mu_j = j! n0 (G tau)^{j+1}
inline Series steady_moment(const Series &n0, const Series &growth, const Coeff &tau, int j) {
  double factorial = 1.0;
  for (int i = 2; i <= j; ++i) factorial *= i;
  return n0.mul(growth.mul(tau.on(growth)).pow(static_cast<double>(j + 1))).mul(factorial);
}
// full population-balance right-hand side: state [c, n_1 .. n_B] on `grid` (size-independent growth G(c), nucleation flux B at L = 0, outflow n / tau)
inline Series pbe_rate(const Series &state, const pbe::Grid &grid, const Msmpr &p) {
  const std::int64_t bins = grid.bins();
  const Series c = state.select(-1, 0), n = state.slice(-1, 1, bins + 1, 1);
  const Series g = growth_rate(c, p), tau = p.residence_time.on(c);
  const Series mu2 = pbe::moment(n, grid, 2.0), mu3 = pbe::moment(n, grid, 3.0);
  const Series b = birth_rate(c, mu3, p);
  const Series edges = g.unsqueeze(-1).mul(pbe::on(std::vector<double>(static_cast<std::size_t>(bins + 1), 1.0), n));
  const Series dn = pbe::growth_rhs(n, grid, edges, b).sub(n.div(tau.unsqueeze(-1)));
  const Series dc = p.feed_concentration.on(c).sub(c).div(tau).sub(p.crystal_density.on(c).mul(p.shape_factor.on(c)).mul(g).mul(mu2).mul(3.0));
  return dc.unsqueeze(-1).unite(std::vector<Series>{dn}, state.dim() - 1);
}

} // namespace pluss::subsp::natsci::chemistry::crystallization
