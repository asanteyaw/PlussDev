#pragma once

// Proton linkage (Wyman's thermodynamic linkage) for binding, folding and enzyme activity, batched over lanes.
//
// A molecular state with proton-binding sites has the proton partition function Z(pH) (the binding polynomial in the proton activity):
//   independent sites : Z = prod_g (1 + 10^{pK_g - pH})^{n_g}                  (n_g sites of type g with intrinsic pK_g)
//   polyprotic        : Z = 1 + sum_k 10^{log10 beta_k - k pH}                   (macroscopic cumulative protonation constants beta_k)
// Mean protons bound <H> = - d ln Z / (ln 10 dpH). Two states that interconvert (free <-> ligand-bound, native <-> unfolded, ...) with a
// reference equilibrium constant K_ref of the fully deprotonated forms have the pH-dependent apparent constant
//   K_app(pH) = K_ref Z_b(pH) / Z_f(pH),   d log10 K_app / d pH = <H>_f - <H>_b      (Wyman 1964: linked proton uptake/release)
// so a process that releases protons (more protons on the starting state) gets tighter at higher pH. Observed calorimetric enthalpy
// in a buffer: Delta H_obs = Delta H_int + n_H Delta H_ion,buffer (n_H = linked protons taken up by the complex; Baker & Murphy 1996 form).
// Wyman's papers were not obtained; every identity here is verified against brute-force enumeration of all 2^n microstates and finite differences.

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/biology/systems/binding.h"

#include <cmath>

namespace pluss::subsp::natsci::biology::biophysics::linkage {

namespace dynamics = ::pluss::subsp::natsci::biology::dynamics;
namespace binding = ::pluss::subsp::natsci::biology::systems::binding;

namespace detail {
inline Series softplus(const Series &x) { return x.clamp(0.0, 1e300).add(x.abs().neg().exp().add(1.0).log()); } // ln(1 + e^x), stable
} // namespace detail

// ---- independent sites ------------------------------------------------------------------------------------------------------------
// counts, pka [..., G] (or pka [G]); ph [...] -> ln Z [...]
inline Series ln_partition_independent(const Series &counts, const Series &pka, const Series &ph) {
  const Series x = pka.sub(ph.unsqueeze(-1)).mul(std::log(10.0));
  return detail::softplus(x).mul(counts).sum(-1, false);
}
inline Series mean_protons_independent(const Series &counts, const Series &pka, const Series &ph) {
  const Series f = pka.sub(ph.unsqueeze(-1)).mul(std::log(10.0)).neg().exp().add(1.0).reciprocal(); // fraction protonated
  return f.mul(counts).sum(-1, false);
}

// ---- polyprotic ---------------------------------------------------------------------------------------------------------------------
// log10 beta [..., n] (beta_k for k = 1..n)
inline Series ln_partition_polyprotic(const Series &log10_beta, const Series &ph) {
  const std::int64_t n = log10_beta.size(-1);
  const Series k = pluss::series::arange(1, n + 1, 1, log10_beta.scalar_type()).to(log10_beta.platform());
  const Series t = log10_beta.sub(k.mul(ph.unsqueeze(-1))).mul(std::log(10.0));       // ln of each protonated-species term
  const Series m = t.amax(-1, true).clamp(0.0, 1e300);                                 // include the unprotonated term (t_0 = 0)
  return m.squeeze(-1).add(t.sub(m).exp().sum(-1, false).add(m.neg().exp().squeeze(-1)).log());
}
// fraction of the molecules carrying k protons, k = 0..n: [..., n + 1]
inline Series polyprotic_fractions(const Series &log10_beta, const Series &ph) {
  const std::int64_t n = log10_beta.size(-1);
  const Series k = pluss::series::arange(1, n + 1, 1, log10_beta.scalar_type()).to(log10_beta.platform());
  const Series t = log10_beta.sub(k.mul(ph.unsqueeze(-1))).mul(std::log(10.0));
  const Series lnz = ln_partition_polyprotic(log10_beta, ph);
  return lnz.neg().exp().unsqueeze(-1).unite(std::vector<Series>{t.sub(lnz.unsqueeze(-1)).exp()}, t.dim() - 1);
}
inline Series mean_protons_polyprotic(const Series &log10_beta, const Series &ph) {
  const std::int64_t n = log10_beta.size(-1);
  const Series k = pluss::series::arange(0, n + 1, 1, log10_beta.scalar_type()).to(log10_beta.platform());
  return polyprotic_fractions(log10_beta, ph).mul(k).sum(-1, false);
}

// ---- linked equilibria ---------------------------------------------------------------------------------------------------------------
// log10 K_app = log10 K_ref + (ln Z_b - ln Z_f) / ln 10
inline Series log10_apparent_constant(const Series &log10_k_ref, const Series &ln_z_product_state, const Series &ln_z_start_state) {
  return log10_k_ref.add(ln_z_product_state.sub(ln_z_start_state).div(std::log(10.0)));
}
// binding of a ligand to a receptor at fixed pH with depletion: bound complex concentration
inline Series bound_complex(const Series &receptor_total, const Series &ligand_total, const Series &log10_k_app) {
  return binding::receptor_ligand(receptor_total, ligand_total, log10_k_app.neg().mul(std::log(10.0)).exp());
}
// Delta H_obs = Delta H_int + n_H Delta H_ion
inline Series observed_enthalpy(const Series &dh_intrinsic, const Series &protons_taken_up, const Series &dh_buffer_ionization) {
  return dh_intrinsic.add(protons_taken_up.mul(dh_buffer_ionization));
}

} // namespace pluss::subsp::natsci::biology::biophysics::linkage
