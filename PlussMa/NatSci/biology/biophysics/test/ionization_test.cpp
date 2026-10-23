// Charge, isoelectric point and buffer capacity against Biopython's Bjellqvist implementation and exact identities.
#include "pluss/subsp/natsci/biology/biophysics/ionization.h"
#include "ionization_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace io = pluss::subsp::natsci::biology::biophysics::ionization;
namespace ref = ionization_reference;
using pluss::Series;

namespace {
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }
} // namespace

int main() {
  try {
    const auto g = io::protein_groups(ref::sequences);
    const std::size_t n = ref::sequences.size(), np = ref::ph.size();
    // charge at every pH: lanes [N, P] -> broadcast counts over the pH axis
    double worst = 0.0;
    for (std::size_t p = 0; p < np; ++p) {
      const auto q = io::charge(g.counts, g.pka, g.sign, vec(std::vector<double>(n, ref::ph[p]))).contiguous().to_vec<double>();
      for (std::size_t i = 0; i < n; ++i)
        worst = std::max(worst, std::abs(q[i] - ref::charge[i * np + p]));
    }
    expect(worst < 1e-10, "charge vs Biopython charge_at_pH, worst " + std::to_string(worst));
    // isoelectric point: Biopython bisects to 1e-4 inside [4.05, 12]; compare inside that window, check the clamp outside it
    const auto pi = io::isoelectric_point(g.counts, g.pka, g.sign).contiguous().to_vec<double>();
    const auto q0 = io::charge(g.counts, g.pka, g.sign, vec(pi)).contiguous().to_vec<double>();
    std::size_t inside = 0;
    for (std::size_t i = 0; i < n; ++i) {
      expect(std::abs(q0[i]) < 1e-9, "charge vanishes at the isoelectric point, sequence " + std::to_string(i));
      if (ref::pi[i] > 4.06 && ref::pi[i] < 11.99) {
        ++inside;
        expect(std::abs(pi[i] - ref::pi[i]) < 2e-4, "pI vs Biopython, sequence " + ref::sequences[i] + ": " + std::to_string(pi[i]) + " vs " + std::to_string(ref::pi[i]));
      } else if (ref::pi[i] <= 4.06) {
        expect(pi[i] <= 4.06, "pI at or below Biopython's lower clamp");
      } else {
        expect(pi[i] >= 11.99, "pI at or above Biopython's upper clamp");
      }
    }
    expect(inside > 150, "enough pI comparisons: " + std::to_string(inside));
    // documented Biopython examples
    const auto ex = io::protein_groups({"INGAR", "PETER"});
    const auto ex_pi = io::isoelectric_point(ex.counts, ex.pka, ex.sign).contiguous().to_vec<double>();
    expect(std::abs(ex_pi[0] - 9.75) < 5e-3 && std::abs(ex_pi[1] - 4.53) < 5e-3, "documented pI of INGAR and PETER");
    // buffer capacity = -dQ/dpH (central difference)
    const std::vector<double> grid = {3.0, 5.0, 7.0, 9.0, 11.0};
    for (double ph : grid) {
      const double h = 1e-5;
      const Series up = io::charge(g.counts, g.pka, g.sign, vec(std::vector<double>(n, ph + h))), dn = io::charge(g.counts, g.pka, g.sign, vec(std::vector<double>(n, ph - h)));
      const auto fd = up.sub(dn).div(-2.0 * h).contiguous().to_vec<double>();
      const auto b = io::buffer_capacity(g.counts, g.pka, g.sign, vec(std::vector<double>(n, ph))).contiguous().to_vec<double>();
      for (std::size_t i = 0; i < n; ++i)
        expect(std::abs(b[i] - fd[i]) < 1e-7 * (1.0 + std::abs(b[i])), "buffer capacity equals -dQ/dpH");
    }
    // a single acid titrated: beta peaks at pH = pK with value ln10 / 4
    const Series counts = vec({1.0}).unsqueeze(0), pka = vec({4.76}).unsqueeze(0), sign = vec({-1.0});
    const double beta = io::buffer_capacity(counts, pka, sign, vec({4.76})).item<double>();
    expect(std::abs(beta - std::log(10.0) / 4.0) < 1e-12, "maximum buffer capacity ln10/4 at pH = pK");
    std::cout << "ionization tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
