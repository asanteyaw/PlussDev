// Nearest-neighbour nucleic-acid melting against Biopython's MeltingTemp.Tm_NN (the open implementation; see verification/gen_nucleic_ref.py).
#include "nucleic_reference.h"
#include "pluss/subsp/natsci/biology/biophysics/nucleic_acids.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace na = pluss::subsp::natsci::biology::biophysics::nucleic_acids;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
} // namespace

int main() {
  try {
    double worst = 0.0;
    int counted = 0;
    for (const auto &c : nucleic_reference::cases) {
      const auto duplex = na::duplex_thermodynamics(c.seq, na::table(c.table), c.selfcomp);
      const Series tm = na::melting_temperature(duplex, vec({c.dnac1}), vec({c.dnac2}), c.saltcorr, vec({c.na}), vec({c.k}), vec({c.tris}), vec({c.mg}), vec({c.dntps}));
      const double err = std::abs(tm.host<double>().front() - c.tm);
      worst = std::max(worst, err);
      expect(err < 1e-9, std::string("Tm ") + c.kind + " " + c.seq + " table " + c.table + " salt " + std::to_string(c.saltcorr) + ": " + std::to_string(tm.host<double>().front()) + " vs Biopython " + std::to_string(c.tm));
      ++counted;
    }
    expect(counted >= 100, "all reference cases are exercised");
    // lanes: one call with many concentrations equals the scalar calls
    {
      const auto duplex = na::duplex_thermodynamics("ATGCGTACGTTAGC", na::table("DNA_NN4"));
      const std::vector<double> conc{10.0, 100.0, 1000.0, 1e4};
      const Series all = na::melting_temperature(duplex, vec(conc), vec(conc), 5, vec({50, 50, 50, 50}), vec({0, 0, 0, 0}), vec({0, 0, 0, 0}), vec({0, 0, 0, 0}), vec({0, 0, 0, 0}));
      const auto v = all.host<double>();
      for (std::size_t i = 0; i < conc.size(); ++i) {
        const double one = na::melting_temperature(duplex, vec({conc[i]}), vec({conc[i]}), 5, vec({50}), vec({0}), vec({0}), vec({0}), vec({0})).host<double>().front();
        expect(std::abs(v[i] - one) < 1e-12, "lanes agree with scalar calls");
      }
      expect(v[3] > v[0], "Tm rises with strand concentration");
    }
    // two-state melting curve: at the melting temperature of the duplex half of the limiting strand is paired (equal strand concentrations)
    {
      const auto duplex = na::duplex_thermodynamics("CGATCGATCGAT", na::table("DNA_NN4"));
      const double c_nm = 250.0;
      const double tm_c = na::melting_temperature(duplex, vec({c_nm}), vec({c_nm}), 0, vec({50}), vec({0}), vec({0}), vec({0}), vec({0})).host<double>().front();
      const Series t = vec({tm_c + 273.15});
      const Series k = na::equilibrium_constant(duplex, t);
      const double frac = na::duplex_fraction(k, vec({c_nm * 1e-9}), vec({c_nm * 1e-9}), false).host<double>().front();
      // at the reference Tm (no salt correction) K C = 2 for equal strand concentrations C, i.e. half of the strands are paired
      expect(std::abs(frac - 0.5) < 1e-9, "duplex fraction is 1/2 at the melting temperature, got " + std::to_string(frac));
      const Series temps = vec({280.0, 300.0, 320.0, 340.0, 360.0});
      const auto fr = na::duplex_fraction(na::equilibrium_constant(duplex, temps), vec({1e-6}), vec({1e-6}), false).host<double>();
      for (std::size_t i = 1; i < fr.size(); ++i)
        expect(fr[i] <= fr[i - 1] + 1e-15, "duplex fraction decreases with temperature");
      expect(fr.front() > 0.99 && fr.back() < 0.01, "complete duplex when cold, melted when hot");
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "nucleic acid tests passed\n";
  return 0;
}
