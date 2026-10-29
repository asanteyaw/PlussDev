#pragma once

// Statistical rate theories, batched over lanes (temperatures, molecules, pressures): partition functions, transition-state theory (Eyring) with its tunnelling
// correction and Eyring analysis, hard-sphere collision theory, and RRKM microcanonical rates with Lindemann-Hinshelwood (strong-collision) pressure fall-off.
//
//   Q_trans / V = (2 pi m k T / h^2)^{3/2},  Q_rot(linear) = k T / (sigma h c B),  Q_rot(non-linear) = sqrt(pi) / sigma (k T / h c)^{3/2} / sqrt(A B C),
//   Q_vib = prod_j 1 / (1 - exp(-h c nu_j / k T))                                   (wavenumbers nu in cm^-1, zero of energy at the zero-point level)
//   TST     k = kappa (k T / h) (Q^‡ / Q_R) exp(-E0 / R T) (c0)^{1 - m};  Eyring: ln(k / T) = ln(k/h) + dS^‡/R - dH^‡/(R T);  Wigner kappa = 1 + (h c nu^‡ / k T)^2 / 24
//   collision theory   k = P sigma sqrt(8 k T / (pi mu)) N_A exp(-Ea / R T)         (P = steric factor)
//   Beyer-Swinehart    exact harmonic density of states rho(E) on an integer-wavenumber grid: for each oscillator rho_new(E) = sum_{k >= 0} rho(E - k nu); since the
//                      recursion is a cumulative sum over a stride, it is one reshape + cumsum on the Series backend (no loop over energy bins)
//   RRKM               k(E) = N^‡(E - E0) / (h rho(E)),  N^‡ = cumulative count of transition-state states;  thermal population f(E) = rho(E) e^{-E/kT} / Q
//   strong collision   k_uni(omega) = sum_E f(E) k(E) omega / (omega + k(E))          (omega = collision frequency); k_inf = sum f k = (kT/h)(Q^‡/Q)e^{-E0/kT} x
//                      [(bin/kT) / (1 - e^{-bin/kT})] exactly, the TST rate up to the bin discretisation
// Eyring (1935), Wigner (1932), Rice-Ramsperger-Kassel-Marcus, Beyer & Swinehart (1973, Commun. ACM 16:379) and Lindemann-Hinshelwood were not obtained; everything is
// verified against brute-force state enumeration, closed forms and exact identities.

#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"
#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/stats/regression.h"

#include <cmath>
#include <numbers>

namespace pluss::subsp::natsci::chemistry::rate_theory {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace constants = ::pluss::subsp::natsci::common::constants::codata2022;
using dynamics::Coeff;

[[nodiscard]] inline double planck() { return constants::planck_constant.measurement.value().si_value(); }
[[nodiscard]] inline double boltzmann() { return constants::boltzmann_constant.measurement.value().si_value(); }
[[nodiscard]] inline double avogadro() { return constants::avogadro_constant.measurement.value().si_value(); }
[[nodiscard]] inline double gas_constant() { return constants::molar_gas_constant.measurement.value().si_value(); }
[[nodiscard]] inline double light_speed_cm() { return constants::speed_of_light.measurement.value().si_value() * 100.0; } // cm / s
[[nodiscard]] inline double wavenumber_to_joule() { return planck() * light_speed_cm(); }                                 // J per cm^-1

// ---- partition functions ------------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline Series translational_per_volume(const Coeff &mass_kg, const Series &temperature) {
  return temperature.mul(mass_kg.on(temperature)).mul(2.0 * std::numbers::pi * boltzmann() / (planck() * planck())).pow(1.5);
}
[[nodiscard]] inline Series rotational_linear(const Coeff &rotational_constant_cm, const Coeff &symmetry, const Series &temperature) {
  return temperature.mul(boltzmann()).div(rotational_constant_cm.on(temperature).mul(symmetry.on(temperature)).mul(wavenumber_to_joule()));
}
[[nodiscard]] inline Series rotational_nonlinear(const Coeff &a_cm, const Coeff &b_cm, const Coeff &c_cm, const Coeff &symmetry, const Series &temperature) {
  const Series kt_cm = temperature.mul(boltzmann() / wavenumber_to_joule());
  return kt_cm.pow(1.5).mul(std::sqrt(std::numbers::pi)).div(symmetry.on(temperature).mul(a_cm.on(temperature).mul(b_cm.on(temperature)).mul(c_cm.on(temperature)).sqrt()));
}
// wavenumbers [..., n] (cm^-1), temperature [...] -> [...]
[[nodiscard]] inline Series vibrational(const Series &wavenumbers, const Series &temperature) {
  const Series x = wavenumbers.mul(wavenumber_to_joule() / boltzmann()).div(temperature.unsqueeze(-1));
  return x.neg().exp().neg().add(1.0).log().neg().sum(-1, false).exp();
}

// ---- transition-state theory -------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline Series eyring_prefactor(const Series &temperature) { return temperature.mul(boltzmann() / planck()); } // k T / h  [1/s]
// k = kappa (kT/h) exp(-dG^‡ / RT) (c0)^(1 - molecularity); dG [J/mol], standard concentration c0 [mol/m^3] (default 1 mol/L)
[[nodiscard]] inline Series eyring_rate(const Series &delta_g, const Series &temperature, const Coeff &kappa = 1.0, double molecularity = 1.0, double standard_concentration = 1000.0) {
  return eyring_prefactor(temperature).mul(kappa.on(temperature)).mul(delta_g.div(temperature.mul(gas_constant())).neg().exp()).mul(std::pow(standard_concentration, 1.0 - molecularity));
}
// k = kappa (kT/h) (Q^‡/Q_R) exp(-E0/RT): partition functions already include volume factors consistently (per-volume translational parts); E0 [J/mol]
[[nodiscard]] inline Series tst_rate(const Series &q_transition, const Series &q_reactants, const Series &e0, const Series &temperature, const Coeff &kappa = 1.0) {
  return eyring_prefactor(temperature).mul(kappa.on(temperature)).mul(q_transition.div(q_reactants)).mul(e0.div(temperature.mul(gas_constant())).neg().exp());
}
[[nodiscard]] inline Series wigner(const Series &imaginary_wavenumber_cm, const Series &temperature) {
  return imaginary_wavenumber_cm.mul(wavenumber_to_joule() / boltzmann()).div(temperature).square().div(24.0).add(1.0);
}
// Eckart barrier: forward/reverse barrier heights v1, v2 [J/mol] (reactant/product zero-point energies as reference) and the imaginary
// wavenumber |nu^‡| [cm^-1] fixing the curvature. Transmission probability T(E) at energy E [J/mol] above the reactant asymptote, shape [...].
[[nodiscard]] inline Series eckart_transmission(const Series &energy, const Series &v1, const Series &v2, const Series &imaginary_wavenumber_cm) {
  const double pi = std::numbers::pi;
  const Series hnu = imaginary_wavenumber_cm.mul(wavenumber_to_joule() * avogadro());
  const Series a1 = v1.mul(2.0 * pi).div(hnu), a2 = v2.mul(2.0 * pi).div(hnu);
  const Series denom = a1.rsqrt().add(a2.rsqrt());
  const Series open = energy.ge(v1.sub(v2).clamp_min(0.0)); // classically allowed on both asymptotes
  const Series x1 = energy.mul(2.0 * pi).div(hnu), x2 = x1.add(a2).sub(a1); // 2 pi (kinetic energy) / h nu on each side
  const Series a = x1.clamp_min(0.0).sqrt().mul(2.0).div(denom), b = x2.clamp_min(0.0).sqrt().mul(2.0).div(denom);
  const Series g = a1.mul(a2).sub(pi * pi / 4.0);
  const Series root = g.abs().sqrt().mul(2.0);
  const Series cd = g.gt(0.0).where(root.clamp_max(700.0).cosh(), root.cos());
  const Series cp = a.add(b).clamp_max(700.0).cosh(), cm = a.sub(b).clamp_max(700.0).cosh();
  return open.where(cp.sub(cm).div(cp.add(cd)), cp.mul(0.0));
}
// Thermal tunnelling correction kappa(T) = exp(v1/RT) int T(E) exp(-E/RT) dE / RT. T(E) opens like sqrt(E - threshold), so the integral is taken in
// s = sqrt((E - E0)/RT) (E0 = max(0, v1 - v2)), where the integrand is analytic, with composite Simpson on s in [0, sqrt(u_max)].
[[nodiscard]] inline Series eckart(const Series &v1, const Series &v2, const Series &imaginary_wavenumber_cm, const Series &temperature, int points = 801, double u_max = 80.0) {
  const std::int64_t n = (points % 2 == 0) ? points + 1 : points;
  const double s_max = std::sqrt(u_max), h = s_max / static_cast<double>(n - 1);
  const Series s = Series::linspace(0.0, s_max, n, pluss::ScalarType::Float64);
  std::vector<double> w(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i)
    w[i] = (i == 0 || i == n - 1) ? 1.0 : (i % 2 ? 4.0 : 2.0);
  const Series weights = pluss::make_series(w, {n}).to(pluss::ScalarType::Float64).mul(h / 3.0).mul(s).mul(2.0); // Simpson weight * dE/ds / RT
  const Series rt_ = temperature.mul(gas_constant()).unsqueeze(-1); // [..., 1]
  const Series e0 = v1.sub(v2).clamp_min(0.0).unsqueeze(-1);       // energetic threshold (endothermic reactions)
  const Series u = s.square();
  const Series e = e0.add(u.mul(rt_));
  const Series prob = eckart_transmission(e, v1.unsqueeze(-1), v2.unsqueeze(-1), imaginary_wavenumber_cm.unsqueeze(-1));
  const Series boltz = v1.unsqueeze(-1).sub(e0).div(rt_).sub(u).exp();
  return prob.mul(boltz).mul(weights).sum(-1, false);
}
struct EyringFit final {
  Series enthalpy = pluss::series::undefined(); // dH^‡ [J/mol]
  Series entropy = pluss::series::undefined();  // dS^‡ [J/(mol K)] (at the standard concentration used for k)
  Series enthalpy_error = pluss::series::undefined();
  Series entropy_error = pluss::series::undefined();
};
// Eyring analysis of rate constants k(T): regression of ln(k/T) on 1/T
[[nodiscard]] inline EyringFit eyring_fit(const Series &temperature, const Series &rate_constant) {
  namespace reg = ::pluss::subsp::stats::regression;
  const auto fit = reg::ols_fit(rate_constant.div(temperature).log(), temperature.reciprocal().unsqueeze(-1));
  const auto b = fit.coefficients.contiguous().host<double>(), se = fit.std_errors.contiguous().host<double>();
  const double r = gas_constant(), ln_kh = std::log(boltzmann() / planck());
  const auto make = [&](double v) { return pluss::make_series(std::vector<double>{v}, {1}).to(temperature.scalar_type()).select(0, 0); };
  return {.enthalpy = make(-b[1] * r), .entropy = make((b[0] - ln_kh) * r), .enthalpy_error = make(se[1] * r), .entropy_error = make(se[0] * r)};
}

// ---- collision theory ---------------------------------------------------------------------------------------------------------------------
// reduced mass mu [kg], cross-section sigma [m^2], activation energy [J/mol]; result in m^3 / (mol s)
[[nodiscard]] inline Series collision_rate(const Coeff &sigma, const Coeff &reduced_mass, const Coeff &activation_energy, const Series &temperature, const Coeff &steric = 1.0) {
  const Series speed = temperature.mul(8.0 * boltzmann() / std::numbers::pi).div(reduced_mass.on(temperature)).sqrt();
  return speed.mul(sigma.on(temperature)).mul(steric.on(temperature)).mul(avogadro()).mul(activation_energy.on(temperature).div(temperature.mul(gas_constant())).neg().exp());
}

// ---- densities of states and RRKM ------------------------------------------------------------------------------------------------------------
// exact harmonic densities of states on the grid E_i = i cm^-1 (i = 0 .. bins - 1) for integer wavenumbers [..., n]: counts per 1 cm^-1 bin, lanes batched.
// The per-oscillator update rho(E) += rho(E - nu) (cumulative along the stride nu) is a reshape + cumsum.
[[nodiscard]] inline Series beyer_swinehart(const std::vector<std::int64_t> &wavenumbers, std::int64_t bins, const Config &cfg = Config{}.with_dtype(pluss::ScalarType::Float64)) {
  dynamics::require(bins >= 2, "beyer_swinehart: need at least two bins");
  std::vector<double> delta(static_cast<std::size_t>(bins), 0.0);
  delta[0] = 1.0;
  Series rho = pluss::make_series(delta, {bins}).to(cfg.dtype);
  for (const std::int64_t nu : wavenumbers) {
    dynamics::require(nu >= 1, "beyer_swinehart: wavenumbers must be positive integers (cm^-1)");
    const std::int64_t rows = (bins + nu - 1) / nu, padded = rows * nu;
    Series x = padded > bins ? rho.unite(std::vector<Series>{pluss::series::zeros({padded - bins}, cfg)}, 0) : rho;
    x = x.reshape({rows, nu}).cumsum(0, false).reshape({padded});                // rho_new(E) = sum_k rho(E - k nu)
    rho = x.slice(0, 0, bins, 1);
  }
  return rho;
}
struct Rrkm final {
  Series energy_cm = pluss::series::undefined();  // [B]
  Series rate = pluss::series::undefined();       // k(E) [1/s], zero below the threshold
  Series population = pluss::series::undefined(); // f(E) at the given temperature
  Series partition = pluss::series::undefined();  // Q (vibrational, reactant)
  double threshold_bins{0.0};
};
// reactant and transition-state densities of states (counts per 1 cm^-1 bin, same grid), threshold E0 in whole cm^-1; temperature [...]
[[nodiscard]] inline Rrkm rrkm(const Series &rho_reactant, const Series &rho_transition, std::int64_t threshold_cm, const Series &temperature) {
  const std::int64_t bins = rho_reactant.size(-1);
  dynamics::require(threshold_cm >= 0 && threshold_cm < bins && rho_transition.size(-1) == bins, "rrkm: the threshold must lie inside the grid and both densities share it");
  const Series energy = pluss::series::arange(0, bins, 1, rho_reactant.scalar_type()).to(rho_reactant.platform());
  const Config zc = Config{}.with_dtype(rho_reactant.scalar_type()).with_platform(rho_reactant.platform());
  const Series cumulative = rho_transition.cumsum(-1, false);                                              // N^‡(eps), eps = i bins above the TS zero
  const Series count = threshold_cm == 0 ? cumulative : pluss::series::zeros({threshold_cm}, zc).unite(std::vector<Series>{cumulative.slice(-1, 0, bins - threshold_cm, 1)}, 0); // N^‡(E - E0), 0 below E0
  const Series safe_rho = rho_reactant.gt(0.0).where(rho_reactant, rho_reactant.mul(0.0).add(1.0));
  const double bin_joule = wavenumber_to_joule();
  const Series rate = count.div(safe_rho).mul(bin_joule / planck()).mul(rho_reactant.gt(0.0).to(rho_reactant.scalar_type()));  // N^‡ / (h rho/dE)
  const Series boltz = energy.unsqueeze(0).mul(-bin_joule / boltzmann()).div(temperature.unsqueeze(-1)).exp();               // [..., B]
  const Series weight = rho_reactant.mul(boltz);
  const Series q = weight.sum(-1, true);
  return {.energy_cm = energy, .rate = rate, .population = weight.div(q), .partition = q.squeeze(-1), .threshold_bins = static_cast<double>(threshold_cm)};
}
// high-pressure limit k_inf = sum f k
[[nodiscard]] inline Series high_pressure_limit(const Rrkm &r) { return r.population.mul(r.rate).sum(-1, false); }
// strong-collision Lindemann-Hinshelwood-RRKM: k_uni(omega) = sum f k omega / (omega + k), omega [...] collision frequency [1/s]
[[nodiscard]] inline Series strong_collision(const Rrkm &r, const Series &omega) {
  const Series w = omega.unsqueeze(-1);
  return r.population.mul(r.rate).mul(w).div(w.add(r.rate)).sum(-1, false);
}

// ---- hindered internal rotor ---------------------------------------------------------------------------------------------------------------
// H = -(hbar^2 / 2 I) d^2/dphi^2 + (V0 / 2)(1 - cos(N phi)) on the full circle in the plane-wave basis e^{i m phi}, |m| <= M:
// diagonal hbar^2 m^2 / (2 I) + V0/2, couplings -V0/4 between m and m +- N. Levels (energies from the potential minimum, J) from one batched symmetric eigensolve;
// partition function Q = sum_n exp(-E_n / kT) (divided by N, the internal symmetry number, for the usual per-well convention).
// Limits that fix it: V0 -> 0 gives the free rotor sum_m exp(-hbar^2 m^2 / 2 I kT); a high barrier gives N identical harmonic wells with
// omega = N sqrt(V0 / 2 I) and levels hbar omega (n + 1/2) + O(1/V0).
// moment of inertia I [kg m^2], barrier V0 [J], returns the 2M+1 sorted levels [J]
[[nodiscard]] inline Series hindered_rotor_levels(double moment_of_inertia, double barrier, std::int64_t symmetry, std::int64_t cutoff = 60) {
  dynamics::require(moment_of_inertia > 0.0 && barrier >= 0.0 && symmetry >= 1 && cutoff > symmetry, "hindered_rotor_levels: I > 0, V0 >= 0, N >= 1 and a plane-wave cutoff above N");
  const std::int64_t n = 2 * cutoff + 1;
  const double hbar = planck() / (2.0 * std::numbers::pi), kinetic = hbar * hbar / (2.0 * moment_of_inertia);
  std::vector<double> h(static_cast<std::size_t>(n * n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    const double m = static_cast<double>(i - cutoff);
    h[static_cast<std::size_t>(i * n + i)] = kinetic * m * m + 0.5 * barrier;
    if (i + symmetry < n) h[static_cast<std::size_t>(i * n + i + symmetry)] = h[static_cast<std::size_t>((i + symmetry) * n + i)] = -0.25 * barrier;
  }
  return pluss::subsp::linearalg::eigvalsh(pluss::make_series(h, {n, n}).to(pluss::ScalarType::Float64));
}
// Q(T) for temperatures [...]; per_well divides by the symmetry number
[[nodiscard]] inline Series hindered_rotor_partition(const Series &levels, const Series &temperature, std::int64_t symmetry = 1) {
  const Series ground = levels.select(0, 0);
  // exp(-(E - E0)/kT) x exp(-E0/kT): kept in two factors so very high barriers do not underflow the sum
  const Series x = levels.sub(ground).unsqueeze(0).div(temperature.unsqueeze(-1).mul(boltzmann()));
  return x.neg().exp().sum(-1, false).mul(ground.div(temperature.mul(boltzmann())).neg().exp()).div(static_cast<double>(symmetry));
}

// ---- weak-collision master equation --------------------------------------------------------------------------------------------------------
// Grain-averaged master equation  dn_j/dt = omega sum_i (P_ji n_i - P_ij ... ) - k_j n_j  with an exponential-down collision kernel of mean energy transfer alpha:
// the symmetric collision weight Phi_ij = d_i d_j exp(-|E_i - E_j| / alpha) is scaled (symmetric Sinkhorn iteration) so that sum_i Phi_ij = f_j, i.e. P_ij = Phi_ij / f_j is
// a stochastic matrix that satisfies detailed balance with the Boltzmann population f exactly. With N_j = n_j / sqrt(f_j) the generator
//     M = omega (F^{-1/2} Phi F^{-1/2} - I) - diag(k)
// is symmetric, so the thermal unimolecular rate constant is  k_uni = -lambda_max(M)  from one batched symmetric eigensolve (omega lanes). omega -> infinity gives
// k_inf = sum f k exactly (the populations stay Boltzmann); small alpha / low omega gives the weak-collision low-pressure behaviour.
struct MasterEquation final {
  Series grain_energy = pluss::series::undefined(); // [G] grain centre energies (cm^-1)
  Series rate = pluss::series::undefined();         // [G] grain-averaged k(E)
  Series population = pluss::series::undefined();   // [G] grain Boltzmann populations (sum 1)
  Series coupling = pluss::series::undefined();     // [G, G] F^{-1/2} Phi F^{-1/2}
};
// coarse-grain an Rrkm object (single temperature) into grains of `grain_cm` bins; alpha_cm = mean energy transferred per down collision
[[nodiscard]] inline MasterEquation master_equation(const Rrkm &r, std::int64_t grain_cm, double alpha_cm, int /*unused: kept for source compatibility*/ sinkhorn_iterations = 0) {
  const std::int64_t bins = r.population.size(-1), g = bins / grain_cm;
  dynamics::require(r.population.numel() == bins && g >= 2 && grain_cm >= 1 && alpha_cm > 0.0, "master_equation: one temperature, at least two grains, grain >= 1 cm^-1, alpha > 0");
  const Series population = r.population.reshape({bins});
  const Series pop = population.slice(0, 0, g * grain_cm, 1).reshape({g, grain_cm}), weighted = population.mul(r.rate).slice(0, 0, g * grain_cm, 1).reshape({g, grain_cm});
  const Series f = pop.sum(-1, false), fk = weighted.sum(-1, false);
  const Series k = fk.div(f.clamp(1e-300, 1e300));                               // population-weighted rate of the grain
  const Series e = pluss::series::arange(0, g, 1, f.scalar_type()).to(f.platform()).mul(static_cast<double>(grain_cm)).add(0.5 * static_cast<double>(grain_cm - 1));
  const Series w = e.unsqueeze(1).sub(e.unsqueeze(0)).abs().mul(-1.0 / alpha_cm).exp();   // [G, G] symmetric
  // scaling d with d_i (W d)_i = f_i holding to RELATIVE accuracy in every grain (f spans tens of orders of magnitude while the high-energy grains carry the large
  // k): a few symmetric Sinkhorn sweeps for a positive start, then Newton on the relative residual r_i = d_i (W d)_i / f_i - 1 in the variables u = delta d / d,
  // where the Jacobian is I + (d_i W_ij d_j) / f_i = I + P^T (a stochastic matrix at the solution): well conditioned and quadratically convergent.
  Series d = f.sqrt();
  for (int it = 0; it < 100; ++it) d = d.mul(f.div(d.mul(w.matmul(d.unsqueeze(-1)).squeeze(-1)).clamp(1e-300, 1e300)).sqrt());
  const Series eye_g = pluss::series::eye(g, Config{}.with_dtype(f.scalar_type()).with_platform(f.platform()));
  for (int it = 0; it < 30; ++it) {
    const Series relative = d.mul(w.matmul(d.unsqueeze(-1)).squeeze(-1)).div(f).sub(1.0);
    if (relative.abs().max().item<double>() < 1e-14) break;
    const Series jac = eye_g.add(d.unsqueeze(-1).mul(w).mul(d.unsqueeze(0)).div(f.unsqueeze(-1)));
    d = d.mul(dynamics::solve(jac, relative.neg()).add(1.0));
  }
  const Series phi = d.unsqueeze(1).mul(w).mul(d.unsqueeze(0));
  const Series inv_sqrt_f = f.clamp(1e-300, 1e300).rsqrt();
  return {e, k, f, phi.mul(inv_sqrt_f.unsqueeze(1)).mul(inv_sqrt_f.unsqueeze(0))};
}
// k_uni(omega) for collision frequencies omega [...]: -lambda_max of omega (C - I) - diag(k)
[[nodiscard]] inline Series weak_collision(const MasterEquation &m, const Series &omega) {
  const std::int64_t g = m.rate.size(0);
  const Config cfg = Config{}.with_dtype(m.rate.scalar_type()).with_platform(m.rate.platform());
  const Series eye = pluss::series::eye(g, cfg);
  const Series generator = m.coupling.sub(eye).unsqueeze(0).mul(omega.reshape({omega.numel(), 1, 1})).sub(m.rate.unsqueeze(-1).mul(eye).unsqueeze(0));  // [L, G, G]
  return pluss::subsp::linearalg::eigvalsh(generator).select(-1, g - 1).neg().reshape(omega.sizes());
}

} // namespace pluss::subsp::natsci::chemistry::rate_theory
