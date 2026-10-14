#pragma once

// Membrane electrophysiology. Voltages in mV, times in ms, conductances in
// mS/cm^2, currents in uA/cm^2 unless a function says otherwise.
//
// Sources: A. L. Hodgkin & A. F. Huxley, J. Physiol. 117:500-544 (1952), eqs. 26, 7, 15, 16
// (membrane current I = C dV/dt + gK n^4 (V-VK) + gNa m^3 h (V-VNa) + gl (V-Vl);
// n, m, h kinetics); Goldman equation / Nernst equation (secondary sources, unverified);
// parameter set gNa=120, gK=36, gL=0.3 mS/cm^2, ENa=+115, EK=-12, EL=+10.613 mV,
// Cm=1 uF/cm^2: Table 3 of the original paper (read from a scan), whose V has the
// opposite sign (V_Na = -115, V_K = +12, V_l = -10.613); here V = Vm - Vrest with
// depolarisation positive. Cable equation: lambda^2 V_xx = tau V_t + V.

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/common/electrochem.h"
#include "pluss/subsp/natsci/common/field.h"

namespace pluss::subsp::natsci::biology::biophysics::membrane {

namespace spatial = ::pluss::subsp::natsci::common::spatial;
using dynamics::Coeff;

// E = (RT / zF) ln(out / in); rt_over_f in the same voltage unit (26.7 mV at 37C). The temperature form
// lives in common::electrochem::ion_potential (shared with chemistry); this keeps the rt_over_f spelling.
inline Series nernst(const Series &outside, const Series &inside, double valence, double rt_over_f) {
  dynamics::require_positive(outside, "nernst: outside");
  dynamics::require_positive(inside, "nernst: inside");
  dynamics::require(valence != 0.0, "nernst: valence must be non-zero");
  return outside.div(inside).log().mul(rt_over_f / valence);
}

// GHK voltage for monovalent ions: cations enter as (P, out, in), anions as
// (P, out, in) with their roles swapped inside the logarithm.
// permeability/outside/inside: [..., n_ions]; sign: +1 cation, -1 anion [n_ions].
inline Series ghk_voltage(const Series &permeability, const Series &outside,
                          const Series &inside, const Series &sign,
                          double rt_over_f) {
  dynamics::require_nonnegative(permeability, "ghk: permeability");
  dynamics::require_positive(outside, "ghk: outside");
  dynamics::require_positive(inside, "ghk: inside");
  const Series cation = sign.gt(0.0).to(permeability.scalar_type());
  const Series anion = cation.neg().add(1.0);
  const Series p = permeability;
  const Series numerator =
      p.mul(cation.mul(outside).add(anion.mul(inside))).sum(-1, false);
  const Series denominator =
      p.mul(cation.mul(inside).add(anion.mul(outside))).sum(-1, false);
  return numerator.div(denominator).log().mul(rt_over_f);
}

namespace detail {
// x / (exp(x / y) - 1) with its removable singularity filled (limit y).
inline Series vtrap(const Series &x, double y) {
  const Series z = x.div(y);
  const Series tiny = z.abs().lt(1e-7);
  const Series safe = tiny.where(z.mul(0.0).add(1.0), z);
  return tiny.where(x.mul(0.0).add(y).sub(x.mul(0.5)), x.div(safe.expm1()));
}
} // namespace detail

struct Gating final {
  Series alpha = pluss::series::undefined();
  Series beta = pluss::series::undefined();
};

// V = Vm - Vrest in mV, depolarisation positive. The paper (Hodgkin & Huxley 1952,
// J. Physiol. 117:500-544, eqs. 12, 13, 20, 21, 23, 24 and Table 3) uses the opposite
// sign for V (depolarisation negative: V_Na = -115, V_K = +12, V_l = -10.613 mV), so
// every expression below is the paper's with V -> -V:
//   alpha_n = 0.01 (10 - V)/(exp((10 - V)/10) - 1),  beta_n = 0.125 exp(-V/80)
//   alpha_m = 0.1 (25 - V)/(exp((25 - V)/10) - 1),   beta_m = 4 exp(-V/18)
//   alpha_h = 0.07 exp(-V/20),                       beta_h = 1/(exp((30 - V)/10) + 1)
// (rates in 1/ms at 6.3 C).
inline Gating hh_n(const Series &v) {
  return {detail::vtrap(v.neg().add(10.0), 10.0).mul(0.01), v.div(-80.0).exp().mul(0.125)};
}
inline Gating hh_m(const Series &v) {
  return {detail::vtrap(v.neg().add(25.0), 10.0).mul(0.1), v.div(-18.0).exp().mul(4.0)};
}
inline Gating hh_h(const Series &v) {
  return {v.div(-20.0).exp().mul(0.07), v.neg().add(30.0).div(10.0).exp().add(1.0).reciprocal()};
}

inline Series steady_gate(const Gating &g) { return g.alpha.div(g.alpha.add(g.beta)); }

struct HodgkinHuxley final {
  double g_na{120.0}, g_k{36.0}, g_leak{0.3};
  double e_na{115.0}, e_k{-12.0}, e_leak{10.613};
  double capacitance{1.0};
  // The paper's rate functions are for 6.3 C; at temperature T all rate constants are
  // multiplied by phi = 3^((T - 6.3)/10) (Q10 = 3; p. 523 of the paper).
  double celsius{6.3};
  [[nodiscard]] double rate_factor() const { return std::pow(3.0, (celsius - 6.3) / 10.0); }
};

// State [..., 4] = (V, m, h, n); i_ext is the injected current density.
inline Series hodgkin_huxley_rate(const Series &s, const Coeff &i_ext,
                                  const HodgkinHuxley &p = {}) {
  dynamics::require_species(s, 4, "hodgkin_huxley: state");
  const Series v = s.select(-1, 0);
  const Series m = s.select(-1, 1);
  const Series h = s.select(-1, 2);
  const Series n = s.select(-1, 3);
  const Series m3 = m.square().mul(m);
  const Series n4 = n.square().square();
  const Series i_na = m3.mul(h).mul(v.sub(p.e_na)).mul(p.g_na);
  const Series i_k = n4.mul(v.sub(p.e_k)).mul(p.g_k);
  const Series i_l = v.sub(p.e_leak).mul(p.g_leak);
  const Series dv = i_ext.on(v).sub(i_na).sub(i_k).sub(i_l).div(p.capacitance);
  const Gating gm = hh_m(v), gh = hh_h(v), gn = hh_n(v);
  const double phi = p.rate_factor();
  const Series dm = gm.alpha.mul(m.neg().add(1.0)).sub(gm.beta.mul(m)).mul(phi);
  const Series dh = gh.alpha.mul(h.neg().add(1.0)).sub(gh.beta.mul(h)).mul(phi);
  const Series dn = gn.alpha.mul(n.neg().add(1.0)).sub(gn.beta.mul(n)).mul(phi);
  return dv.unsqueeze(-1).unite(
      std::vector<Series>{dm.unsqueeze(-1), dh.unsqueeze(-1), dn.unsqueeze(-1)}, -1);
}

// Resting state (V = 0): gates at their steady values.
inline Series hodgkin_huxley_rest(const Series &like) {
  const Series v = like.mul(0.0);
  return v.unsqueeze(-1).unite(
      std::vector<Series>{steady_gate(hh_m(v)).unsqueeze(-1),
                          steady_gate(hh_h(v)).unsqueeze(-1),
                          steady_gate(hh_n(v)).unsqueeze(-1)},
      -1);
}

// Propagating action potential along an axon (paper eq. 29): with axoplasm resistivity R2
// [ohm cm] and fibre radius a [cm],  C dV/dt = (a/(2 R2)) V_xx - I_ion  with V in mV,
// x in cm, t in ms, I in uA/cm^2; the 1000 converts mV/(ohm cm^2) to uA/cm^2.
// State [..., 4, N] = (V, m, h, n) fields on a periodic grid (trailing spatial axis).
inline Series hodgkin_huxley_axon_rate(const Series &s, double radius_cm, double axoplasm_ohm_cm,
                                       const spatial::PeriodicGrid &grid, const HodgkinHuxley &p = {}) {
  dynamics::require(s.dim() >= 2 && s.size(-2) == 4, "hodgkin_huxley_axon: state must be [..., 4, N]");
  const Series v = s.select(-2, 0), m = s.select(-2, 1), h = s.select(-2, 2), n = s.select(-2, 3);
  const Series m3 = m.square().mul(m);
  const Series n4 = n.square().square();
  const Series i_ion = m3.mul(h).mul(v.sub(p.e_na)).mul(p.g_na).add(n4.mul(v.sub(p.e_k)).mul(p.g_k))
                           .add(v.sub(p.e_leak).mul(p.g_leak));
  const double axial = 1000.0 * radius_cm / (2.0 * axoplasm_ohm_cm);
  const Series dv = spatial::laplacian(v, grid).mul(axial).sub(i_ion).div(p.capacitance);
  const Gating gm = hh_m(v), gh = hh_h(v), gn = hh_n(v);
  const double phi = p.rate_factor();
  const Series dm = gm.alpha.mul(m.neg().add(1.0)).sub(gm.beta.mul(m)).mul(phi);
  const Series dh = gh.alpha.mul(h.neg().add(1.0)).sub(gh.beta.mul(h)).mul(phi);
  const Series dn = gn.alpha.mul(n.neg().add(1.0)).sub(gn.beta.mul(n)).mul(phi);
  return dv.unsqueeze(-2).unite(std::vector<Series>{dm.unsqueeze(-2), dh.unsqueeze(-2), dn.unsqueeze(-2)}, -2);
}

// Passive cable on the periodic grid: tau V_t = lambda^2 V_xx - V + drive.
inline Series cable_rate(const Series &v, const Coeff &lambda, const Coeff &tau,
                         const spatial::PeriodicGrid &grid,
                         const Series &drive = pluss::series::undefined()) {
  dynamics::require_positive(lambda, v, "cable: lambda");
  dynamics::require_positive(tau, v, "cable: tau");
  const Series l = lambda.on(v);
  Series total = spatial::laplacian(v, grid).mul(l.square()).sub(v);
  if (drive.defined())
    total = total.add(drive);
  return total.div(tau.on(v));
}

} // namespace pluss::subsp::natsci::biology::biophysics::membrane
