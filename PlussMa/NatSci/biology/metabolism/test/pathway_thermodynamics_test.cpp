// Max-min driving force and thermodynamics-based flux balance against scipy/HiGHS and exhaustive enumeration
// (see NatSci/biology/verification/gen_mdf_ref.py).
#include "mdf_reference.h"
#include "pluss/subsp/natsci/biology/metabolism/pathway_thermodynamics.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pt = pluss::subsp::natsci::biology::metabolism::pathway_thermodynamics;
namespace ref = mdf_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(F64); }
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
} // namespace

int main() {
  try {
    const double rt = 8.314462618e-3 * 298.15;
    // ---- analytic chain A -> B -> C: with A at its maximum and C at its minimum, B balances the two driving forces
    {
      pt::Pathway p;
      p.stoichiometry = mat({-1, 0, 1, -1, 0, 1}, 3, 2);
      p.dg0 = vec({2.0, -6.0});
      p.c_min = {1e-6, 1e-6, 1e-6};
      p.c_max = {1e-2, 1e-2, 1e-2};
      const auto r = pt::max_min_driving_force(p);
      expect(r.status == pluss::subsp::math::lp::SolveStatus::Optimal, "chain solved");
      const double xa = std::log(1e-2), xc = std::log(1e-6);
      // balance: -G1 - RT (xB - xa) = -G2 - RT (xc - xB)  =>  xB = (xa + xc)/2 + (G2 - G1)/(2 RT) ;  B = -G1 - RT (xB - xa)
      const double xb = 0.5 * (xa + xc) + (-6.0 - 2.0) / (2.0 * rt), expected = -2.0 - rt * (xb - xa);
      expect(xb >= std::log(1e-6) && xb <= std::log(1e-2), "the balancing concentration is inside its bounds");
      expect(std::abs(r.mdf - expected) < 1e-8, "MDF of the chain " + std::to_string(r.mdf) + " vs " + std::to_string(expected));
      expect(std::abs(r.driving_force[0] - r.mdf) < 1e-8 && std::abs(r.driving_force[1] - r.mdf) < 1e-8, "both reactions sit at the minimum driving force");
      expect(r.shadow_price[0] > 0.0 && r.shadow_price[1] > 0.0, "both reactions are bottlenecks");
      expect(std::abs(r.shadow_price[0] + r.shadow_price[1] - 1.0) < 1e-8, "shadow prices sum to one (dMDF / dB)");
    }
    // ---- random pathways: MDF vs scipy HiGHS on the formulation of Noor et al.
    {
      std::size_t sp = 0, gp = 0, lp_off = 0;
      const std::size_t cases = ref::mdf_status.size();
      for (std::size_t k = 0; k < cases; ++k) {
        const auto c = static_cast<std::int64_t>(ref::mdf_n[2 * k]), r = static_cast<std::int64_t>(ref::mdf_n[2 * k + 1]);
        pt::Pathway p;
        p.stoichiometry = mat(std::vector<double>(ref::mdf_S.begin() + static_cast<std::ptrdiff_t>(sp), ref::mdf_S.begin() + static_cast<std::ptrdiff_t>(sp + c * r)), c, r);
        p.dg0 = vec(std::vector<double>(ref::mdf_g0.begin() + static_cast<std::ptrdiff_t>(gp), ref::mdf_g0.begin() + static_cast<std::ptrdiff_t>(gp + r)));
        for (std::int64_t i = 0; i < c; ++i) {
          p.c_min.push_back(std::exp(ref::mdf_lo[lp_off + static_cast<std::size_t>(i)]));
          p.c_max.push_back(std::exp(ref::mdf_hi[lp_off + static_cast<std::size_t>(i)]));
        }
        if (ref::mdf_ratio[4 * k] > 0.5)
          p.ratios.push_back({static_cast<std::int64_t>(ref::mdf_ratio[4 * k + 1]), static_cast<std::int64_t>(ref::mdf_ratio[4 * k + 2]), ref::mdf_ratio[4 * k + 3]});
        sp += static_cast<std::size_t>(c * r);
        gp += static_cast<std::size_t>(r);
        lp_off += static_cast<std::size_t>(c);
        const auto res = pt::max_min_driving_force(p);
        const bool feasible_ref = ref::mdf_status[k] == 0.0;
        expect((res.status == pluss::subsp::math::lp::SolveStatus::Optimal) == feasible_ref, "feasibility of random pathway " + std::to_string(k));
        if (feasible_ref) {
          expect(std::abs(res.mdf - ref::mdf_value[k]) < 1e-6 * (1.0 + std::abs(ref::mdf_value[k])), "MDF of random pathway " + std::to_string(k) + ": " + std::to_string(res.mdf) + " vs " + std::to_string(ref::mdf_value[k]));
          double minimum = 1e300;
          for (const double d : res.driving_force)
            minimum = std::min(minimum, d);
          expect(std::abs(minimum - res.mdf) < 1e-6 * (1.0 + std::abs(res.mdf)), "the smallest driving force equals the MDF");
        }
      }
    }
    // ---- thermodynamics-based flux balance against exhaustive enumeration of direction patterns
    for (std::size_t k = 0; k < ref::tfa_value.size(); ++k) {
      const double dg = ref::tfa_cases[4 * k], lo = ref::tfa_cases[4 * k + 1], hi = ref::tfa_cases[4 * k + 2];
      const bool reversible = ref::tfa_cases[4 * k + 3] > 0.5;
      pt::TfaNetwork net;
      net.stoichiometry = mat({1, -1, 0, 0, 1, -1}, 2, 3);
      net.dg0 = vec({0.0, dg, 0.0});
      net.lower = {0.0, reversible ? -50.0 : 0.0, 0.0};
      net.upper = {10.0, 50.0, 50.0};
      net.constrained = {0, 1, 0};
      net.c_min = {lo, lo};
      net.c_max = {hi, hi};
      net.objective = {0.0, 0.0, 1.0};
      const auto r = pt::thermodynamic_flux_balance(net);
      const double want = ref::tfa_value[k];
      expect(std::isfinite(want) ? (r.status == pluss::subsp::math::lp::SolveStatus::Optimal && std::abs(r.objective - want) < 1e-6) : true, "TFA objective, case " + std::to_string(k) + ": " + std::to_string(r.objective) + " vs enumeration " + std::to_string(want));
      if (r.status == pluss::subsp::math::lp::SolveStatus::Optimal && std::abs(r.flux[1]) > 1e-9) {
        // the reported direction obeys the second law at the reported concentrations
        expect((r.flux[1] > 0.0) == (r.delta_g[1] < 0.0), "flux direction agrees with the sign of the reaction energy, case " + std::to_string(k));
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "pathway thermodynamics tests passed\n";
  return 0;
}
