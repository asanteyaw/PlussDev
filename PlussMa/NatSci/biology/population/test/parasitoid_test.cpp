// Host-parasitoid maps: equilibria, Jacobians and stability vs numpy/scipy, May's k < 1 stabilisation, and the coupled-map lattice.
#include "pluss/subsp/natsci/biology/population/parasitoid.h"
#include "parasitoid_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pa = pluss::subsp::natsci::biology::population::parasitoid;
namespace ref = parasitoid_reference;
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
    const std::size_t stride = 1 + 4 + 2 + 4 + 1;
    for (std::size_t i = 0; i < ref::cases.size() / stride; ++i) {
      const double *c = &ref::cases[stride * i];
      const int kind = static_cast<int>(c[0]);
      const double p0 = c[1], p1 = c[2], p2 = c[3], p3 = c[4];
      const Series eq = vec({c[5], c[6]});
      std::function<Series(const Series &)> map;
      if (kind == 0) map = [=](const Series &s) { return pa::nicholson_bailey(s, p0, p1, p2); };
      else if (kind == 1) map = [=](const Series &s) { return pa::negative_binomial(s, p0, p1, p2, p3); };
      else map = [=](const Series &s) { return pa::beddington(s, p0, p1, p2, p3); };
      const std::string tag = "case " + std::to_string(i) + " (kind " + std::to_string(kind) + "): ";
      expect(map(eq).sub(eq).abs().max().item<double>() < 1e-9, tag + "the reference equilibrium is a fixed point of the map");
      if (kind == 0) expect(pa::nicholson_bailey_equilibrium(p0, p1, p2, eq.select(0, 0)).sub(eq).abs().max().item<double>() < 1e-9, tag + "analytic Nicholson-Bailey equilibrium");
      if (kind == 1) expect(pa::negative_binomial_equilibrium(p0, p1, p2, p3, eq.select(0, 0)).sub(eq).abs().max().item<double>() < 1e-9, tag + "analytic negative-binomial equilibrium");
      const Series jac = pa::jacobian(map, eq);
      const auto jv = jac.contiguous().host<double>();
      for (int k = 0; k < 4; ++k)
        expect(std::abs(jv[static_cast<std::size_t>(k)] - c[7 + k]) < 1e-6 * (1.0 + std::abs(c[7 + k])), tag + "Jacobian entry " + std::to_string(k));
      expect(std::abs(pa::spectral_radius(jac).item<double>() - c[11]) < 1e-6, tag + "spectral radius " + std::to_string(pa::spectral_radius(jac).item<double>()) + " vs " + std::to_string(c[11]));
      // stability decides the long-run behaviour: iterate from near the equilibrium
      if (c[11] < 0.98) {
        Series s = eq.mul(1.05);
        for (int t = 0; t < 4000; ++t) s = map(s);
        expect(s.sub(eq).abs().max().item<double>() < 1e-6, tag + "a stable equilibrium attracts");
      } else if (c[11] > 1.02) {
        Series s = eq.mul(1.0 + 1e-6);
        double dev = 0.0;
        for (int t = 0; t < 400; ++t) { s = map(s); dev = std::max(dev, s.sub(eq).abs().max().item<double>() / eq.max().item<double>()); }
        expect(dev > 1e-2, tag + "an unstable equilibrium repels the orbit");
      }
    }
    // May (1978): aggregation with k < 1 stabilises, k > 1 does not (lambda = 2, a = 0.05, c = 1)
    {
      for (const double k : {0.2, 0.5, 0.9}) {
        const Series eq = pa::negative_binomial_equilibrium(2.0, 0.05, 1.0, k, vec({0.0}).select(0, 0));
        const auto map = [=](const Series &s) { return pa::negative_binomial(s, 2.0, 0.05, 1.0, k); };
        expect(pa::spectral_radius(pa::jacobian(map, eq)).item<double>() < 1.0, "k = " + std::to_string(k) + " < 1 is stable");
      }
      for (const double k : {1.2, 2.0, 5.0}) {
        const Series eq = pa::negative_binomial_equilibrium(2.0, 0.05, 1.0, k, vec({0.0}).select(0, 0));
        const auto map = [=](const Series &s) { return pa::negative_binomial(s, 2.0, 0.05, 1.0, k); };
        expect(pa::spectral_radius(pa::jacobian(map, eq)).item<double>() > 1.0, "k = " + std::to_string(k) + " > 1 is unstable");
      }
      // k -> infinity recovers Nicholson-Bailey
      const Series s = vec({12.0, 9.0});
      expect(pa::negative_binomial(s, 2.0, 0.05, 1.0, 1e7).sub(pa::nicholson_bailey(s, 2.0, 0.05, 1.0)).abs().max().item<double>() < 1e-5, "k -> infinity limit is Nicholson-Bailey");
    }
    // spectral radius of known matrices (real pair and complex pair)
    {
      const Series a = pluss::make_series(std::vector<double>{2.0, 0.0, 0.0, -3.0, 0.0, -1.0, 1.0, 0.0}, {2, 2, 2}).to(F64);
      const auto r = pa::spectral_radius(a).contiguous().host<double>();
      expect(std::abs(r[0] - 3.0) < 1e-12 && std::abs(r[1] - 1.0) < 1e-12, "spectral radius of a real pair and of a rotation");
    }
    // coupled-map lattice: vs numpy; total abundance conserved by pure dispersal; synchrony of a uniform lattice is zero
    {
      const auto local = [](const Series &n) { return n.mul(n.neg().add(1.0).mul(ref::cml_r).exp()); };
      Series n = vec(ref::cml_n0);
      for (int t = 0; t < 40; ++t) n = pa::ring_step(local, n, ref::cml_d);
      expect(n.sub(vec(ref::cml_n40)).abs().max().item<double>() < 1e-9, "coupled-map lattice after 40 steps vs numpy");
      const auto identity = [](const Series &x) { return x; };
      const Series m = pa::ring_step(identity, vec(ref::cml_n0), 0.4);
      expect(std::abs(m.sum().item<double>() - vec(ref::cml_n0).sum().item<double>()) < 1e-12, "dispersal conserves the total");
      expect(pa::synchrony(vec({0.7, 0.7, 0.7, 0.7})).item<double>() < 1e-15, "uniform lattice is synchronous");
      // strong dispersal synchronises a chaotic lattice
      Series a = vec(ref::cml_n0), b = vec(ref::cml_n0);
      for (int t = 0; t < 600; ++t) { a = pa::ring_step(local, a, 0.05); b = pa::ring_step(local, b, 0.5); }
      expect(pa::synchrony(b).item<double>() < pa::synchrony(a).item<double>() + 1e-12, "stronger dispersal does not reduce synchrony");
    }
    std::cout << "parasitoid tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
