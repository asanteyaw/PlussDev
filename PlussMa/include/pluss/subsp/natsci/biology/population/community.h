#pragma once

// Community ecology and adaptive dynamics, batched over lanes.
//
//  * Random communities (May 1972 setting, in the form of Allesina & Tang, Nature 483 (2012) 205, arXiv:1105.2071, read): a community matrix
//    M = -d I + (off-diagonal entries of variance sigma^2, connectance C, pair correlation tau = E[M_ij M_ji] / sigma^2). For large S the
//    eigenvalues fill an ellipse centred at -d with real semi-axis sigma sqrt(S C) (1 + tau); stability needs sigma sqrt(S C) (1 + tau) < d
//    (May: tau = 0, sigma sqrt(S C) < d; predator-prey tau = -2/pi: d pi / (pi - 2) = 2.75 d; competition/mutualism mix tau = 2/pi: 0.61 d).
//    `spectral_abscissa` = max Re lambda = ln rho(exp(A)) computed with batched matrix exponentials and Gelfand's formula (no general eigensolver).
//  * Generalised Lotka-Volterra invasion analysis: invasion growth rate r_i + sum_j A_ij n_j* of a species absent from the resident
//    community whose equilibrium solves A_RR n_R = -r_R.
//  * Chemostat with Monod uptake (Tilman-style resource competition for one limiting resource): dR/dt = D (S0 - R) - sum_i q_i mu_i(R) N_i,
//    dN_i/dt = N_i (mu_i(R) - m_i), mu_i(R) = mu_max,i R / (K_i + R); R*_i = m_i K_i / (mu_max,i - m_i); the species with the lowest R* excludes the others.
//  * Adaptive dynamics (invasion fitness f(x, y) of a rare mutant y in a resident x; canonical equation dx/dt = M(x) df/dy |_{y=x}; singular
//    point x*: df/dy = 0; convergence stable iff d^2f/dxdy + d^2f/dy^2 < 0; evolutionarily stable iff d^2f/dy^2 < 0; convergence stable but
//    invadable (d^2f/dy^2 > 0) = evolutionary branching point). Formulas as stated in arXiv:1005.3862 (read) eqs. (1)-(4); the Gaussian
//    logistic-competition example there gives d^2f/dy^2 = 1/sigma_alpha^2 - 1/sigma_K^2 at x* = 0, so branching iff sigma_alpha < sigma_K.

#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/natsci/biology/population/demography.h"
#include "pluss/tils/kit/rng_seed.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::biology::population::community {

namespace demography = ::pluss::subsp::natsci::biology::population::demography;

// ---- random matrices and stability --------------------------------------------------------------------------------------------
inline Series spectral_abscissa(const Series &a, int squarings = 40) {
  return demography::spectral_radius(pluss::subsp::linearalg::matrix_exp(a), squarings).log();
}

// -d I + off-diagonal entries N(0, sigma^2) with probability `connectance` (symmetric pattern), pair correlation tau; `lanes` independent matrices
inline Series random_community(std::int64_t species, std::int64_t lanes, double sigma, double connectance, double tau, double d, std::uint64_t seed = 0) {
  dynamics::require(species >= 2 && lanes >= 1 && sigma >= 0.0 && connectance > 0.0 && connectance <= 1.0 && std::abs(tau) <= 1.0, "random_community: invalid arguments");
  const Config cfg = Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::uint64_t base = pluss::tils::seed_or_next(seed);
  const Series z1 = pluss::series::randn({lanes, species, species}, cfg.dtype, base + 1), z2 = pluss::series::randn({lanes, species, species}, cfg.dtype, base + 2);
  const Series u = pluss::series::rand({lanes, species, species}, cfg.dtype, base + 3);
  const Series idx = pluss::series::arange(0, species, 1, cfg.dtype);
  const Series upper = idx.unsqueeze(-1).lt(idx.unsqueeze(0)).to(cfg.dtype);     // i < j
  const Series lower = upper.transpose(0, 1);
  const Series keep_u = u.lt(connectance).to(cfg.dtype).mul(upper);                // sampled on the upper triangle, mirrored below
  const Series keep = keep_u.add(keep_u.transpose(-1, -2));
  const Series upper_vals = z1.mul(sigma);                                          // M_ij, i < j
  const Series lower_vals = z1.transpose(-1, -2).mul(tau * sigma).add(z2.mul(sigma * std::sqrt(1.0 - tau * tau))); // M_ji given M_ij
  const Series off = upper_vals.mul(upper).add(lower_vals.mul(lower)).mul(keep);
  return off.sub(pluss::series::eye(species, cfg).mul(d));
}
// edge of the stability region for the elliptic law: real semi-axis sigma sqrt(S C) (1 + tau) - d (large S)
inline double predicted_abscissa(std::int64_t species, double sigma, double connectance, double tau, double d) {
  return sigma * std::sqrt(static_cast<double>(species) * connectance) * (1.0 + tau) - d;
}

// ---- generalised Lotka-Volterra ----------------------------------------------------------------------------------------------
// resident mask [S] (1 = present); returns n* of the resident subsystem (zero elsewhere) [..., S] solving (A n + r)_R = 0
inline Series resident_equilibrium(const Series &r, const Series &a, const Series &resident) {
  const std::int64_t s = r.size(-1);
  const Config cfg = Config{}.with_dtype(r.scalar_type()).with_platform(r.platform());
  const Series m = resident.to(r.scalar_type()).to(r.platform());
  const Series eye = pluss::series::eye(s, cfg);
  // absent rows become the identity (n_j = 0 equation), present rows are A n = -r restricted to present columns
  const Series mat = m.unsqueeze(-1).mul(a.mul(m.unsqueeze(-2))).add(eye.mul(m.neg().add(1.0)));
  return dynamics::solve(mat, r.neg().mul(m));
}
inline Series invasion_growth_rate(const Series &r, const Series &a, const Series &resident) {
  const Series n = resident_equilibrium(r, a, resident);
  return r.add(dynamics::matvec(a, n));
}

// ---- chemostat ----------------------------------------------------------------------------------------------------------------
inline Series monod(const Series &resource, const Series &mu_max, const Series &half) { return resource.mul(mu_max).div(resource.add(half)); }
// state [..., 1 + n]: resource then n consumer densities; parameters are [n] (or lane-broadcastable)
inline Series chemostat_rate(const Series &state, double dilution, double supply, const Series &mu_max, const Series &half, const Series &mortality, const Series &yield) {
  const Series resource = state.select(-1, 0).unsqueeze(-1);
  const Series n = state.slice(-1, 1, state.size(-1), 1);
  const Series growth = monod(resource, mu_max, half);                                   // [..., n]
  const Series dr = resource.neg().add(supply).mul(dilution).sub(growth.mul(n).div(yield).sum(-1, true));
  const Series dn = growth.sub(mortality).mul(n);
  return dr.unite(std::vector<Series>{dn}, state.dim() - 1);
}
inline Series r_star(const Series &mu_max, const Series &half, const Series &mortality) { return mortality.mul(half).div(mu_max.sub(mortality)); }
// single-species equilibrium consumer density N* = yield D (S0 - R*) / m  (with the mortality m including dilution, as D = m in a chemostat)
inline Series single_species_density(double dilution, double supply, const Series &rstar, const Series &mortality, const Series &yield) {
  return rstar.neg().add(supply).mul(dilution).mul(yield).div(mortality);
}

// ---- adaptive dynamics --------------------------------------------------------------------------------------------------------
using Fitness = std::function<Series(const Series &mutant, const Series &resident)>; // elementwise over lanes

// df/dy at y = x
inline Series selection_gradient(const Fitness &f, const Series &x, double h = 1e-5) { return f(x.add(h), x).sub(f(x.sub(h), x)).div(2.0 * h); }
struct SecondDerivatives final {
  Series fyy = pluss::series::undefined(), fxy = pluss::series::undefined();
};
inline SecondDerivatives second_derivatives(const Fitness &f, const Series &x, double h = 1e-3) {
  SecondDerivatives out;
  out.fyy = f(x.add(h), x).sub(f(x, x).mul(2.0)).add(f(x.sub(h), x)).div(h * h);
  out.fxy = f(x.add(h), x.add(h)).sub(f(x.add(h), x.sub(h))).sub(f(x.sub(h), x.add(h))).add(f(x.sub(h), x.sub(h))).div(4.0 * h * h);
  return out;
}
struct SingularClassification final {
  Series convergence_stable = pluss::series::undefined(); // Bool: fxy + fyy < 0
  Series evolutionarily_stable = pluss::series::undefined(); // Bool: fyy < 0
  Series branching = pluss::series::undefined();          // Bool: convergence stable and invadable
  Series css = pluss::series::undefined();                // Bool: convergence stable and evolutionarily stable
};
inline SingularClassification classify(const Fitness &f, const Series &x_star, double h = 1e-3) {
  const SecondDerivatives d = second_derivatives(f, x_star, h);
  SingularClassification out;
  out.convergence_stable = d.fxy.add(d.fyy).lt(0.0);
  out.evolutionarily_stable = d.fyy.lt(0.0);
  out.branching = out.convergence_stable.logical_and(d.fyy.gt(0.0));
  out.css = out.convergence_stable.logical_and(out.evolutionarily_stable);
  return out;
}
// Newton for a singular strategy (selection gradient zero) from x0, derivative of the gradient by central differences
inline Series singular_strategy(const Fitness &f, const Series &x0, double tolerance = 1e-10, std::int64_t max_iterations = 100, double h = 1e-4) {
  Series x = x0;
  for (std::int64_t i = 0; i < max_iterations; ++i) {
    const Series g = selection_gradient(f, x);
    const Series dg = selection_gradient(f, x.add(h)).sub(selection_gradient(f, x.sub(h))).div(2.0 * h);
    x = x.sub(g.div(dg));
    if (g.abs().max().item<double>() < tolerance)
      break;
  }
  return x;
}
// logistic competition: f(x, y) = 1 - alpha(y, x) K(x) / K(y)
inline Series logistic_competition_fitness(const Series &mutant, const Series &resident, const std::function<Series(const Series &)> &carrying,
                                           const std::function<Series(const Series &mutant, const Series &resident)> &kernel) {
  return kernel(mutant, resident).mul(carrying(resident)).div(carrying(mutant)).neg().add(1.0);
}

} // namespace pluss::subsp::natsci::biology::population::community
