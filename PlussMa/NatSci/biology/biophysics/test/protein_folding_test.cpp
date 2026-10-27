// Protein folding: equilibrium / chevron models against the open PyFolding package, thermal unfolding against numerical thermodynamics
// (see NatSci/biology/verification/gen_folding_ref.py).
#include "folding_reference.h"
#include "pluss/subsp/natsci/biology/biophysics/protein_folding.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pf = pluss::subsp::natsci::biology::biophysics::protein_folding;
namespace ref = folding_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double max_rel(const Series &got, const std::vector<double> &want) {
  const auto g = got.contiguous().host<double>();
  expect(g.size() == want.size(), "size mismatch");
  double worst = 0.0;
  for (std::size_t i = 0; i < g.size(); ++i)
    worst = std::isfinite(g[i]) ? std::max(worst, std::abs(g[i] - want[i]) / (1e-12 + std::abs(want[i]))) : 1e300;
  return worst;
}
} // namespace

int main() {
  try {
    const Series x = vec(ref::pf_x);
    const double rt = ref::pf_rt[0];
    expect(max_rel(pf::two_state_fraction_unfolded(x, ref::pf_two_p[0], ref::pf_two_p[1], rt), ref::pf_two_F) < 1e-12, "two-state fraction vs PyFolding");
    {
      const auto &p = ref::pf_sloping_p;
      // PyFolding writes the signal as (a_f + b_f x) + (a_u + b_u x) F, i.e. its second line is the DIFFERENCE between the unfolded and the native
      // baseline; the standard form Y_N (1 - F) + Y_U F used here has Y_U = Y_N + that difference.
      expect(max_rel(pf::two_state_signal(x, p[0], p[1], p[0] + p[2], p[1] + p[3], p[4], p[5], rt), ref::pf_sloping) < 1e-12, "sloping-baseline signal vs PyFolding");
    }
    {
      const auto &p = ref::pf_three_p; // Y_N, Y_I, Y_D, dG_NI, m_NI, dG_ID, m_ID
      const Series pop = pf::three_state_populations(x, p[3], p[4], p[5], p[6], rt);
      expect(max_rel(pop.select(-1, 0), ref::pf_three_FN) < 1e-12 && max_rel(pop.select(-1, 1), ref::pf_three_FI) < 1e-12 && max_rel(pop.select(-1, 2), ref::pf_three_FD) < 1e-12, "three-state populations vs PyFolding");
      const Series signal = pop.select(-1, 0).mul(p[0]).add(pop.select(-1, 1).mul(p[1])).add(pop.select(-1, 2).mul(p[2]));
      expect(max_rel(signal, ref::pf_three_signal) < 1e-12, "three-state signal vs PyFolding");
      expect(pop.sum(-1, false).sub(1.0).abs().max().item<double>() < 1e-14, "populations sum to one");
    }
    {
      const Series dimer = pf::dimer_fraction_unfolded(x, ref::pf_dimer_p[0], ref::pf_dimer_p[1], ref::pf_dimer_p[2], rt);
      expect(max_rel(dimer, ref::pf_dimer_exact) < 1e-13, "dimer fraction unfolded vs 60-digit evaluation");
      expect(max_rel(dimer, ref::pf_dimer_FD) < 1e-7, "dimer fraction unfolded vs PyFolding (whose formula loses digits for K >> Pt)");
    }
    {
      const auto &p = ref::pf_chev_two_p;
      expect(max_rel(pf::chevron_two_state(x, p[0], p[1], p[2], p[3]), ref::pf_chev_two) < 1e-12, "two-state chevron vs PyFolding");
      const auto &q = ref::pf_chev_three_p;
      expect(max_rel(pf::chevron_three_state(x, q[0], q[1], q[2], q[3], q[4], q[5]), ref::pf_chev_three) < 1e-12, "three-state chevron vs PyFolding");
      // chevron arms and equilibrium agree: at the midpoint of a two-state protein k_f exp(-m_f x) = k_u exp(m_u x), stability zero
      const double mid = std::log(p[0] / p[2]) / (p[1] + p[3]);
      expect(std::abs(pf::chevron_stability(p[0], p[1], p[2], p[3], vec({mid}), rt).host<double>().front()) < 1e-12, "stability vanishes at the chevron midpoint");
      expect(std::abs(pf::chevron_two_state(vec({mid}), p[0], p[1], p[2], p[3]).host<double>().front() - 2.0 * p[0] * std::exp(-p[1] * mid)) < 1e-12, "k_obs is twice k_f at the midpoint");
    }
    // free energy and the midpoint
    expect(std::abs(pf::unfolding_free_energy(vec({4.2}), 1.9, 4.2).host<double>().front()) < 1e-14 && std::abs(pf::two_state_fraction_unfolded(vec({4.2}), 1.9, 4.2, rt).host<double>().front() - 0.5) < 1e-14, "midpoint: dG = 0, half unfolded");
    // thermal unfolding vs integration of dH/dT = dCp, dS/dT = dCp/T
    {
      pf::Thermal p{ref::th_p[0], ref::th_p[1], ref::th_p[2]};
      const Series t = vec(ref::th_T);
      expect(max_rel(pf::thermal_gibbs(t, p), ref::th_G) < 1e-9, "Delta G(T) vs numerical thermodynamics");
      expect(pf::thermal_gibbs(t, p).sub(pf::thermal_gibbs_closed_form(t, p)).abs().max().item<double>() < 1e-12, "closed form equals H - T S");
      expect(pf::thermal_fraction_unfolded(t, p).sub(vec(ref::th_frac)).abs().max().item<double>() < 1e-10, "fraction unfolded vs oracle (absolute: K = exp(-G/RT) magnifies relative errors of G)");
      const Series cold = pf::thermal_root(p, vec({200.0}), vec({ref::th_p[1] - 1e-6}));
      expect(std::abs(cold.host<double>().front() - ref::th_roots[0]) < 1e-6, "cold denaturation temperature");
      const Series ts = pf::maximum_stability_temperature(p, vec({0.0}));
      expect(std::abs(ts.item<double>() - ref::th_Tstab_numeric[0]) < 1e-3, "temperature of maximum stability vs numerical maximum");
      expect(std::abs(pf::thermal_entropy(ts, p).item<double>()) < 1e-12, "Delta S vanishes at maximum stability");
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "protein folding tests passed\n";
  return 0;
}
