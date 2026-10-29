#pragma once

// Biochemical thermodynamics: transformed (pH-adjusted) Gibbs energies and apparent equilibrium constants, batched over lanes.
//
// A reactant with several protonation states is treated as a PSEUDOISOMER GROUP (Alberty, Thermodynamics of Biochemical Reactions,
// Wiley 2003, ch. 1-2; Legendre transform at fixed pH). With the species H_{n-j} A (j = 0..n protons removed from the fully protonated
// form H_n A; stepwise acid constants pKa_1..pKa_n):
//     Delta_f G(H_{n-j}A)   = Delta_f G(H_n A) + R T ln10 sum_{m<=j} pKa_m          (standard state pH 0 for H+, mu_H+ = 0)
//     Delta_f G'(species)   = Delta_f G(species) + N_H R T ln10 pH                    (Legendre transform removing the proton potential)
//     Delta_f G'(group)     = -R T ln sum_species exp(-Delta_f G'(species)/R T)
//                           = Delta_f G(H_n A) + n R T ln10 pH - R T ln P,   P = sum_{j=0}^n 10^{ j pH - sum_{m<=j} pKa_m }
// and for a reaction sum_i nu_i Delta_f G'(group_i) = Delta_r G'^o, K' = exp(-Delta_r G'^o / R T). Ionic-strength activity corrections
// enter through the extended Debye-Hueckel expression ln gamma = -alpha z^2 sqrt(I) / (1 + B sqrt(I)) with alpha and B supplied by the
// caller (Alberty uses B = 1.6 kg^1/2 mol^-1/2 and alpha ~ 1.17582 kg^1/2 mol^-1/2 at 298.15 K; those values are NOT checked against
// the literature here).

#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <vector>

namespace pluss::subsp::natsci::biology::metabolism::thermodynamics {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace constants = ::pluss::subsp::natsci::common::constants::codata2022;
using dynamics::Coeff;

inline double gas_constant() { return constants::molar_gas_constant.measurement.value().si_value(); }

// P = sum_{j=0}^n 10^{ j pH - sum_{m<=j} pKa_m } for pKa [..., n] and pH [...] (or [] broadcast); the j = 0 term is 1
inline Series proton_binding_polynomial(const Series &ph, const Series &pka) {
  const std::int64_t n = pka.size(-1);
  Series p = ph.mul(0.0).add(1.0);
  Series cumulative = ph.mul(0.0);
  for (std::int64_t j = 1; j <= n; ++j) {
    cumulative = cumulative.add(pka.select(-1, j - 1));
    p = p.add(ph.mul(static_cast<double>(j)).sub(cumulative).mul(std::log(10.0)).exp());
  }
  return p;
}

// Delta_f G'(group) in J/mol from Delta_f G(H_n A) (J/mol), the number of hydrogens n of the fully protonated species, pKa [..., n]
inline Series transformed_formation_gibbs(const Series &dgf_fully_protonated, double n_hydrogens, const Series &pka, const Series &ph, double temperature) {
  const double rt = gas_constant() * temperature;
  const Series p = proton_binding_polynomial(ph, pka);
  return dgf_fully_protonated.add(ph.mul(n_hydrogens * rt * std::log(10.0))).sub(p.log().mul(rt));
}
// a species that cannot (de)protonate in the range of interest: only the Legendre term
inline Series transformed_formation_gibbs_fixed(const Series &dgf, double n_hydrogens, const Series &ph, double temperature) {
  return dgf.add(ph.mul(n_hydrogens * gas_constant() * temperature * std::log(10.0)));
}
// Delta_r G'^o = sum nu_i Delta_f G'_i for [..., S] formation energies and stoichiometric coefficients nu [S]
inline Series reaction_gibbs(const Series &dgf_transformed, const Series &nu) { return dgf_transformed.mul(nu).sum(-1, false); }
inline Series apparent_equilibrium_constant(const Series &dgr_transformed, double temperature) {
  return dgr_transformed.div(-gas_constant() * temperature).exp();
}
// Delta_r G' = Delta_r G'^o + R T ln Q (Q in activities / concentrations relative to the 1 M standard state)
inline Series reaction_gibbs_at(const Series &dgr_transformed, const Series &ln_q, double temperature) {
  return dgr_transformed.add(ln_q.mul(gas_constant() * temperature));
}
// extended Debye-Hueckel activity coefficient: ln gamma = -alpha z^2 sqrt(I) / (1 + B sqrt(I)); alpha supplied by the caller
// (alpha(T) / (R T) of debye_huckel_factor is the matching value)
inline Series debye_huckel_ln_gamma(const Series &charge, const Series &ionic_strength, double alpha, double b = 1.6) {
  const Series root = ionic_strength.sqrt();
  return charge.square().mul(root).div(root.mul(b).add(1.0)).mul(-alpha);
}


// ---- ionic strength, charge and magnesium (eQuilibrator / Alberty form) -----------------------------------------------------
// Debye-Hueckel transformation coefficient DH(I, T) in J/mol (T in K, I in M)
inline Series debye_huckel_factor(const Series &ionic_strength, double temperature) {
  const double a1 = 9.20483e-3, a2 = 1.284668e-5, a3 = 4.95199e-8, b = 1.6; // kJ/mol/M^0.5/K^n, 1/M^0.5
  const double alpha_kj = a1 * temperature - a2 * temperature * temperature + a3 * temperature * temperature * temperature;
  const Series root = ionic_strength.sqrt();
  return root.div(root.mul(b).add(1.0)).mul(alpha_kj * 1000.0); // alpha / (1/sqrt(I) + B) = alpha sqrt(I) / (1 + B sqrt(I))
}
// Delta_f G of Mg2+ at temperature T (J/mol), Gibbs-Helmholtz interpolation between 298.15 K values of G and H
inline double magnesium_formation_gibbs(double temperature) {
  return ((temperature / 298.15) * (-455.3) + (1.0 - temperature / 298.15) * (-467.0)) * 1000.0;
}
// Legendre transform of a microspecies in J/mol: N_H R T ln10 pH + N_Mg (R T ln10 pMg - dGf(Mg)) - DH (z^2 - N_H - 4 N_Mg)
inline Series microspecies_legendre_transform(double charge, double n_protons, double n_magnesium, const Series &ph, double p_mg, const Series &ionic_strength, double temperature) {
  const double rt = gas_constant() * temperature;
  const Series proton = ph.mul(n_protons * rt * std::log(10.0));
  const double mg = n_magnesium * (rt * std::log(10.0) * p_mg - magnesium_formation_gibbs(temperature));
  const Series is = debye_huckel_factor(ionic_strength, temperature).mul(charge * charge - n_protons - 4.0 * n_magnesium);
  return proton.add(mg).sub(is);
}
struct Microspecies final {
  double charge{0.0}, n_protons{0.0}, n_magnesium{0.0};
  double ddg_over_rt{0.0}; // Delta G (this - major microspecies) / R T at 298.15 K, from the dissociation constants
};
// Delta_f G'(group) - Delta_f G(major microspecies reference) in J/mol: -R T logsumexp_i( -(ddg_i/RT * 298.15/T + transform_i / RT) )
// (the relative Delta G is a constant in kJ/mol, hence the temperature rescaling used by eQuilibrator)
inline Series transformed_group_gibbs(const std::vector<Microspecies> &species, const Series &ph, double p_mg, const Series &ionic_strength, double temperature) {
  dynamics::require(!species.empty(), "thermodynamics: a group needs at least one microspecies");
  const double rt = gas_constant() * temperature;
  Series acc = ph.mul(0.0).sub(1e300); // running logsumexp
  Series first = ph.mul(0.0);
  bool initialised = false;
  for (const auto &m : species) {
    const Series t = microspecies_legendre_transform(m.charge, m.n_protons, m.n_magnesium, ph, p_mg, ionic_strength, temperature).div(rt).add(m.ddg_over_rt * 298.15 / temperature);
    const Series neg = t.mul(-1.0);
    if (!initialised) {
      acc = neg;
      initialised = true;
    } else {
      const Series mx = acc.gt(neg).where(acc, neg);
      acc = mx.add(acc.sub(mx).exp().add(neg.sub(mx).exp()).log());
    }
  }
  (void)first;
  return acc.mul(-rt);
}

} // namespace pluss::subsp::natsci::biology::metabolism::thermodynamics
