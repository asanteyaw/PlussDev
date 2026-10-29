#pragma once

// Shared electrochemistry used by chemistry and biology (membranes): Nernst, GHK, Butler-Volmer,
// Debye-Hueckel activity models and the polarisation curve of a cell. Batched over lanes; the
// overpotential inversion uses the shared root solvers (pluss/subsp/math/roots.h).
//
//   F = e N_A (exact in the 2019 SI);  E = E0 - (RT/nF) ln Q  (Nernst);  E0 = -dG0/(nF)
//   Butler-Volmer: i = i0 [exp(alpha_a F eta/RT) - exp(-alpha_c F eta/RT)]
//     alpha_a = alpha_c = 1/2: i = 2 i0 sinh(F eta/2RT);  Tafel slope ln(10) RT/(alpha F)
//   Debye-Hueckel (log10 form), I = (1/2) sum m_i z_i^2:
//     limiting  log gamma = -A z^2 sqrt(I);  Davies  log gamma = -A z^2 (sqrt(I)/(1+sqrt(I)) - 0.3 I)
//   A = 0.509 kg^(1/2) mol^(-1/2) for water at 25 C (supplied by the caller for other temperatures).
//   Polarisation voltage: V = E_rev - eta_act(i) - i R_ohm + (RT/nF) ln(1 - i/i_lim).
// Units: SI (V, A/m^2, J, mol, K).

#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::common::electrochem {

namespace constants = ::pluss::subsp::natsci::common::constants::codata2022;
using dynamics::Coeff;

inline double gas_constant() { return constants::molar_gas_constant.measurement.value().si_value(); }
inline double faraday() {
  return constants::elementary_charge.measurement.value().si_value() *
         constants::avogadro_constant.measurement.value().si_value();
}

// E = E0 - (RT/nF) ln Q.
inline Series nernst(const Series &standard_potential, const Series &temperature, double electrons,
                     const Series &reaction_quotient) {
  dynamics::require_positive(reaction_quotient, "nernst: reaction quotient");
  dynamics::require(electrons != 0.0, "nernst: electron number must be non-zero");
  return standard_potential.sub(temperature.mul(gas_constant() / (electrons * faraday())).mul(reaction_quotient.log()));
}

// Equilibrium (Nernst) potential of a single ion across a membrane: (RT/zF) ln(out/in).
inline Series ion_potential(const Series &outside, const Series &inside, double valence, const Series &temperature) {
  dynamics::require_positive(outside, "ion_potential: outside");
  dynamics::require_positive(inside, "ion_potential: inside");
  dynamics::require(valence != 0.0, "ion_potential: valence must be non-zero");
  return temperature.mul(gas_constant() / (valence * faraday())).mul(outside.div(inside).log());
}

// E0 = -dG0/(nF) with dG0 in J/mol.
inline Series standard_cell_potential(const Series &delta_g, double electrons) {
  dynamics::require(electrons != 0.0, "standard_cell_potential: electron number must be non-zero");
  return delta_g.mul(-1.0 / (electrons * faraday()));
}

// Butler-Volmer current density for overpotential eta [V].
inline Series butler_volmer(const Series &eta, const Coeff &exchange_current, const Coeff &alpha_anodic,
                            const Coeff &alpha_cathodic, const Series &temperature) {
  const Series f_rt = temperature.reciprocal().mul(faraday() / gas_constant());
  const Series anodic = alpha_anodic.on(eta).mul(f_rt).mul(eta).exp();
  const Series cathodic = alpha_cathodic.on(eta).mul(f_rt).mul(eta).neg().exp();
  return exchange_current.on(eta).mul(anodic.sub(cathodic));
}

inline Series tafel_slope(const Coeff &alpha, const Series &temperature) {
  return temperature.mul(std::log(10.0) * gas_constant() / faraday()).div(alpha.on(temperature));
}

// Overpotential that carries current density i, by the shared Illinois solver on a bracket that always
// contains the root (BV is strictly increasing in eta).
inline Series overpotential(const Series &current, const Coeff &exchange_current, const Coeff &alpha_anodic,
                            const Coeff &alpha_cathodic, const Series &temperature) {
  const auto f = [&](const Series &eta) {
    return butler_volmer(eta, exchange_current, alpha_anodic, alpha_cathodic, temperature).sub(current);
  };
  namespace roots = ::pluss::subsp::math::roots;
  const Series lo = current.mul(0.0).add(-5.0), hi = current.mul(0.0).add(5.0);
  roots::Options opt;
  opt.max_iterations = 200;
  opt.f_tolerance = 1e-14;
  return roots::illinois(f, lo, hi, opt).root;
}

// Arrhenius exchange current: i0(T) = i0_ref exp(-Ea/R (1/T - 1/T_ref)).
inline Series exchange_current_arrhenius(const Series &i0_ref, double activation_energy, const Series &temperature,
                                         double reference_temperature) {
  return i0_ref.mul(temperature.reciprocal().sub(1.0 / reference_temperature).mul(-activation_energy / gas_constant()).exp());
}

inline Series ionic_strength(const Series &molality, const Series &charge) {
  dynamics::require(molality.size(-1) == charge.size(-1), "ionic_strength: one charge per species");
  return molality.mul(charge.square()).sum(-1, false).mul(0.5);
}
inline Series log10_gamma_limiting(const Series &ionic, double charge, double a = 0.5091) {
  return ionic.sqrt().mul(-a * charge * charge);
}
inline Series log10_gamma_davies(const Series &ionic, double charge, double a = 0.5091) {
  const Series root = ionic.sqrt();
  return root.div(root.add(1.0)).sub(ionic.mul(0.3)).mul(-a * charge * charge);
}
// Mean activity coefficient of a z+ : z- electrolyte from the Davies model.
inline Series mean_activity_davies(const Series &ionic, double z_plus, double z_minus, double a = 0.5091) {
  return log10_gamma_davies(ionic, std::abs(z_plus * z_minus), a).mul(std::log(10.0)).exp();
}

// Cell voltage at current density i: reversible voltage minus activation (Butler-Volmer inverse),
// ohmic (area-specific resistance) and concentration losses.
inline Series polarisation_voltage(const Series &current, const Series &reversible_voltage, const Coeff &exchange_current,
                                   const Coeff &alpha, const Coeff &area_resistance, const Coeff &limiting_current,
                                   double electrons, const Series &temperature) {
  const Series activation = overpotential(current, exchange_current, alpha, alpha, temperature);
  const Series ohmic = current.mul(area_resistance.on(current));
  const Series ratio = current.div(limiting_current.on(current));
  dynamics::require(ratio.lt(1.0).all().item<bool>(), "polarisation_voltage: current must stay below the limiting current");
  const Series concentration = temperature.mul(gas_constant() / (electrons * faraday())).mul(ratio.neg().add(1.0).log());
  return reversible_voltage.sub(activation).sub(ohmic).add(concentration);
}

} // namespace pluss::subsp::natsci::common::electrochem
