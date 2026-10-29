#pragma once

// Protein folding thermodynamics and kinetics, batched over lanes (denaturant concentrations, temperatures, parameter sets).
//
//  * Equilibrium denaturation (Pace 1986; Santoro & Bolen 1988; Clarke & Fersht 1993 as implemented in the open PyFolding package,
//    models.py `TwoStateEquilibrium`, `TwoStateEquilibriumSloping`, `ThreeStateEquilibrium`, `TwoStateDimerEquilibrium`, read from the GitHub
//    source, SHA-256 8b277549...): linear extrapolation Delta G_U(x) = m (d50 - x) (kcal/mol), fraction unfolded
//    F = exp(m (x - d50)/RT) / (1 + exp(m (x - d50)/RT)), signal (a_N + b_N x)(1 - F) + (a_U + b_U x) F; sequential three-state N <-> I <-> D;
//    dimer N2 <-> 2D with the total monomer concentration P_t.
//  * Chevron plots (Jackson & Fersht 1991; Parker et al. 1995): two-state k_obs = k_f exp(-m_f x) + k_u exp(m_u x); three-state with a rapidly
//    equilibrating intermediate.
//  * Thermal unfolding (Becktel & Schellman, Biopolymers 26 (1987) 1859 form, checked here against the thermodynamic identities
//    dH/dT = dCp, dS/dT = dCp/T): Delta H(T) = Delta H_m + Delta Cp (T - T_m), Delta S(T) = Delta H_m / T_m + Delta Cp ln(T / T_m),
//    Delta G(T) = Delta H(T) - T Delta S(T); the temperature of maximum stability, and hot and cold melting temperatures.

#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <vector>

namespace pluss::subsp::natsci::biology::biophysics::protein_folding {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace roots = ::pluss::subsp::math::roots;
namespace constants = ::pluss::subsp::natsci::common::constants::codata2022;
using dynamics::Coeff;

// R in kcal/(mol K) from the CODATA molar gas constant
inline double gas_constant_kcal() { return constants::molar_gas_constant.measurement.value().si_value() / 4184.0; }
inline double rt_kcal(double temperature_k) { return gas_constant_kcal() * temperature_k; }

// ---- equilibrium denaturation -----------------------------------------------------------------------------------------
// Delta G of unfolding (kcal/mol) at denaturant concentration x with m-value m (kcal/mol/M) and midpoint d50 (M): m (d50 - x)
inline Series unfolding_free_energy(const Series &x, const Coeff &m, const Coeff &d50) { return d50.on(x).sub(x).mul(m.on(x)); }
inline Series two_state_fraction_unfolded(const Series &x, const Coeff &m, const Coeff &d50, double rt) {
  const Series e = x.sub(d50.on(x)).mul(m.on(x)).div(rt).exp();
  return e.div(e.add(1.0));
}
// observed signal with sloping baselines
inline Series two_state_signal(const Series &x, const Coeff &alpha_n, const Coeff &beta_n, const Coeff &alpha_u, const Coeff &beta_u, const Coeff &m, const Coeff &d50, double rt) {
  const Series f = two_state_fraction_unfolded(x, m, d50, rt);
  return alpha_n.on(x).add(beta_n.on(x).mul(x)).mul(f.mul(-1.0).add(1.0)).add(alpha_u.on(x).add(beta_u.on(x).mul(x)).mul(f));
}
// three-state N <-> I <-> D (Hecky & Muller 2005): K_NI = exp((-dG_NI + m_NI x)/RT), K_ID likewise; signal = sum Y_s F_s.
// Returns the three populations [..., 3] (N, I, D)
inline Series three_state_populations(const Series &x, const Coeff &dg_ni, const Coeff &m_ni, const Coeff &dg_id, const Coeff &m_id, double rt) {
  const Series k_ni = x.mul(m_ni.on(x)).sub(dg_ni.on(x)).div(rt).exp();
  const Series k_id = x.mul(m_id.on(x)).sub(dg_id.on(x)).div(rt).exp();
  const Series z = k_ni.mul(0.0).add(1.0).add(k_ni).add(k_ni.mul(k_id));
  return z.reciprocal().unsqueeze(-1).unite(std::vector<Series>{k_ni.div(z).unsqueeze(-1), k_ni.mul(k_id).div(z).unsqueeze(-1)}, -1);
}
// dimer N2 <-> 2D at total monomer concentration pt (M): K_U = Pt exp(m (x - d50)/RT) (PyFolding, Mallam & Jackson 2005),
// fraction of unfolded monomers F_D = (sqrt(K_U^2 + 8 K_U Pt) - K_U) / (4 Pt)
inline Series dimer_fraction_unfolded(const Series &x, const Coeff &m, const Coeff &d50, const Coeff &pt, double rt) {
  const Series p = pt.on(x);
  const Series ku = p.mul(x.sub(d50.on(x)).mul(m.on(x)).div(rt).exp());
  // PyFolding writes (sqrt(K^2 + 8 K Pt) - K) / (4 Pt), which loses digits once K >> Pt; the rationalised form 2 K / (sqrt(K^2 + 8 K Pt) + K) is identical
  return ku.mul(2.0).div(ku.square().add(ku.mul(p).mul(8.0)).sqrt().add(ku));
}

// ---- chevron plots -------------------------------------------------------------------------------------------------------
inline Series chevron_two_state(const Series &x, const Coeff &kf, const Coeff &mf, const Coeff &ku, const Coeff &mu) {
  return kf.on(x).mul(x.mul(mf.on(x)).mul(-1.0).exp()).add(ku.on(x).mul(x.mul(mu.on(x)).exp()));
}
// three-state chevron with a rapidly equilibrating intermediate (Parker et al. 1995): k_obs = k_fi + k_if / (1 + 1/K_iu)
inline Series chevron_three_state(const Series &x, const Coeff &kfi, const Coeff &mif, const Coeff &kif, const Coeff &mi, const Coeff &kiu, const Coeff &mu) {
  const Series k_fi = kfi.on(x).mul(x.mul(mif.on(x)).mul(-1.0).exp());
  const Series k_if = kif.on(x).mul(x.mul(mi.on(x).sub(mif.on(x))).exp());
  const Series k_iu = kiu.on(x).mul(x.mul(mu.on(x).sub(mi.on(x))).exp());
  return k_fi.add(k_if.div(k_iu.reciprocal().add(1.0)));
}
// stability of a two-state protein from its chevron arms: K_U = k_u / k_f at denaturant x, so Delta G_U = -RT ln K_U = RT ln(k_f exp(-m_f x) / (k_u exp(m_u x)))
// (kcal/mol, positive = folded is stable); the equilibrium m-value is RT (m_f + m_u)
inline Series chevron_stability(const Coeff &kf, const Coeff &mf, const Coeff &ku, const Coeff &mu, const Series &x, double rt) {
  const Series ln_kf = kf.on(x).log().sub(x.mul(mf.on(x))), ln_ku = ku.on(x).log().add(x.mul(mu.on(x)));
  return ln_kf.sub(ln_ku).mul(rt);
}

// ---- thermal unfolding ---------------------------------------------------------------------------------------------------
struct Thermal final {
  Coeff dh_m = 0.0; // enthalpy of unfolding at T_m (kcal/mol)
  Coeff tm = 0.0;   // melting temperature (K)
  Coeff dcp = 0.0;  // heat-capacity change of unfolding (kcal/mol/K)
};
inline Series thermal_enthalpy(const Series &t, const Thermal &p) { return p.dh_m.on(t).add(p.dcp.on(t).mul(t.sub(p.tm.on(t)))); }
inline Series thermal_entropy(const Series &t, const Thermal &p) { return p.dh_m.on(t).div(p.tm.on(t)).add(p.dcp.on(t).mul(t.div(p.tm.on(t)).log())); }
inline Series thermal_gibbs(const Series &t, const Thermal &p) { return thermal_enthalpy(t, p).sub(t.mul(thermal_entropy(t, p))); }
// closed form: Delta H_m (1 - T/T_m) - Delta Cp [T_m - T + T ln(T/T_m)]
inline Series thermal_gibbs_closed_form(const Series &t, const Thermal &p) {
  const Series tm = p.tm.on(t);
  return p.dh_m.on(t).mul(t.div(tm).mul(-1.0).add(1.0)).sub(p.dcp.on(t).mul(tm.sub(t).add(t.mul(t.div(tm).log()))));
}
// fraction unfolded in the two-state model at temperature t (K): K = exp(-Delta G/RT), F = K/(1+K)
inline Series thermal_fraction_unfolded(const Series &t, const Thermal &p) {
  const Series k = thermal_gibbs(t, p).div(t.mul(gas_constant_kcal())).mul(-1.0).exp();
  return k.div(k.add(1.0));
}
// temperature of maximum stability: Delta S(T_S) = 0  =>  T_S = T_m exp(-Delta H_m / (Delta Cp T_m))
inline Series maximum_stability_temperature(const Thermal &p, const Series &like) {
  const Series tm = p.tm.on(like);
  return tm.mul(p.dh_m.on(like).div(p.dcp.on(like).mul(tm)).mul(-1.0).exp());
}
// the two roots of Delta G(T) = 0 (cold and hot denaturation) when Delta Cp > 0 ... here: roots on the given brackets by bisection
inline Series thermal_root(const Thermal &p, const Series &lo, const Series &hi) {
  const auto f = [&](const Series &t) { return thermal_gibbs(t, p); };
  roots::Options opt;
  opt.max_iterations = 100;
  return roots::bisect(f, lo, hi, opt).root;
}

} // namespace pluss::subsp::natsci::biology::biophysics::protein_folding
