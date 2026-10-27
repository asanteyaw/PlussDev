// Proton linkage: partition functions vs brute-force microstate enumeration, Wyman's slope relation, bell-shaped enzyme profile, linked binding.
#include "pluss/subsp/natsci/biology/biophysics/linkage.h"
#include "linkage_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace lk = pluss::subsp::natsci::biology::biophysics::linkage;
namespace ref = linkage_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double max_abs(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch");
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]));
  return w;
}
} // namespace

int main() {
  try {
    const Series ph = vec(ref::ph);
    // independent sites listed one per row of counts = 1 (all five sites distinct entries)
    const Series ones = vec({1, 1, 1, 1, 1});
    const Series pk_f = vec(ref::free_pk), pk_b = vec(ref::bound_pk);
    // lanes over pH: counts/pka broadcast against ph [P]
    const Series lz_f = lk::ln_partition_independent(ones, pk_f, ph), lz_b = lk::ln_partition_independent(ones, pk_b, ph);
    expect(max_abs(lz_f, ref::lnz_free) < 1e-12 && max_abs(lz_b, ref::lnz_bound) < 1e-12, "ln Z vs enumeration of 2^5 microstates");
    expect(max_abs(lk::mean_protons_independent(ones, pk_f, ph), ref::h_free) < 1e-12 && max_abs(lk::mean_protons_independent(ones, pk_b, ph), ref::h_bound) < 1e-12, "mean bound protons vs enumeration");
    // mean protons = -d ln Z / (ln 10 dpH)
    {
      const double h = 1e-5;
      const Series up = lk::ln_partition_independent(ones, pk_f, ph.add(h)), dn = lk::ln_partition_independent(ones, pk_f, ph.sub(h));
      expect(max_abs(up.sub(dn).div(-2.0 * h * std::log(10.0)), ref::h_free) < 1e-8, "<H> = -d ln Z / (ln10 dpH)");
    }
    // Wyman: d log10 K_app / dpH = <H>_free - <H>_bound
    {
      const Series k0 = vec(std::vector<double>(8, 6.0));
      const auto kapp = [&](const Series &p) { return lk::log10_apparent_constant(k0, lk::ln_partition_independent(ones, pk_b, p), lk::ln_partition_independent(ones, pk_f, p)); };
      const double h = 1e-5;
      const Series slope = kapp(ph.add(h)).sub(kapp(ph.sub(h))).div(2.0 * h);
      std::vector<double> expected;
      for (std::size_t i = 0; i < ref::ph.size(); ++i)
        expected.push_back(ref::h_free[i] - ref::h_bound[i]);
      expect(max_abs(slope, expected) < 1e-7, "Wyman linkage: d log K / dpH = <H>_f - <H>_b");
      // limits: far below every pK both states fully protonated -> K_app flat at K_ref * 10^{sum(pK_b - pK_f)}; far above -> K_ref
      const Series lo = kapp(vec(std::vector<double>(8, -9.0))).select(0, 0), hi = kapp(vec(std::vector<double>(8, 30.0))).select(0, 0);
      double dpk = 0.0;
      for (std::size_t i = 0; i < 5; ++i)
        dpk += ref::bound_pk[i] - ref::free_pk[i];
      expect(std::abs(lo.item<double>() - (6.0 + dpk)) < 1e-9 && std::abs(hi.item<double>() - 6.0) < 1e-9, "low- and high-pH limits of K_app");
    }
    // diprotic enzyme: active EH fraction is the textbook bell-shaped pH profile
    {
      const Series lb = vec(ref::poly_logbeta);
      const Series frac = lk::polyprotic_fractions(lb, ph); // [P, 3]
      expect(std::abs(frac.sum(-1, false).sub(1.0).abs().max().item<double>()) < 1e-13, "species fractions sum to 1");
      expect(max_abs(frac.select(-1, 1), ref::bell) < 1e-13, "EH fraction equals 1 / (1 + 10^(pK1 - pH) + 10^(pH - pK2))");
      // peak of the bell at pH = (pK1 + pK2) / 2
      const double mid = 0.5 * (5.6 + 8.1);
      const Series grid = vec({mid - 0.2, mid, mid + 0.2});
      const Series f = lk::polyprotic_fractions(lb, grid).select(-1, 1);
      expect(f.select(0, 1).item<double>() > f.select(0, 0).item<double>() && f.select(0, 1).item<double>() > f.select(0, 2).item<double>(), "bell maximum at the mean of the two pKs");
      // consistency of the two formulations: two equivalent sites <=> statistical-factor polyprotic constants
      const double pk = 6.4;
      const Series lbeta = vec({pk + std::log10(2.0), 2.0 * pk});
      const Series phs = vec({4.0, 6.0, 6.4, 7.0, 9.0});
      const Series a = lk::ln_partition_independent(vec({2.0}), vec({pk}), phs), b = lk::ln_partition_polyprotic(lbeta, phs);
      expect(a.sub(b).abs().max().item<double>() < 1e-12, "two equivalent sites = polyprotic with statistical factors");
      expect(lk::mean_protons_polyprotic(lbeta, phs).sub(lk::mean_protons_independent(vec({2.0}), vec({pk}), phs)).abs().max().item<double>() < 1e-12, "mean protons agree");
    }
    // linked binding with depletion: occupancy follows K_app(pH) and matches the closed form at the pH-independent limit
    {
      const Series kapp = vec({6.0, 6.0, 6.0});
      const Series rl = lk::bound_complex(vec({1e-6, 1e-6, 1e-6}), vec({1e-6, 5e-6, 1e-5}), kapp);
      const double kd = 1e-6;
      const double rt = 1e-6;
      const auto exact = [&](double lt) { const double b = rt + lt + kd; return 0.5 * (b - std::sqrt(b * b - 4.0 * rt * lt)); };
      expect(std::abs(rl.select(0, 1).item<double>() - exact(5e-6)) < 1e-15 && std::abs(rl.select(0, 2).item<double>() - exact(1e-5)) < 1e-15, "bound complex vs the exact quadratic");
      const Series dh = lk::observed_enthalpy(vec({-10.0}), vec({0.8}), vec({11.5}));
      expect(std::abs(dh.host<double>().front() - (-10.0 + 0.8 * 11.5)) < 1e-14, "buffer-linked observed enthalpy");
    }
    std::cout << "linkage tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
