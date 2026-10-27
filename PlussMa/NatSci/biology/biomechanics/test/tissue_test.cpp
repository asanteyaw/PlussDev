// Hyperelastic soft-tissue stresses against sympy differentiation of the strain energy with respect to F, plus limiting-model identities.
#include "pluss/subsp/natsci/biology/biomechanics/tissue.h"
#include "tissue_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ts = pluss::subsp::natsci::biology::biomechanics::tissue;
namespace ref = tissue_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double rel(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch " + std::to_string(av.size()) + " vs " + std::to_string(b.size()));
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]) / (1e-12 + std::abs(b[i])));
  return w;
}
void check(const std::string &name, const ts::Isotropic &m, const std::vector<double> &uni, const std::vector<double> &bi, const std::vector<double> &pl, const std::vector<double> &sh) {
  expect(rel(ts::uniaxial_stress(m, vec(ref::lam_uni)), uni) < 1e-11, name + ": uniaxial");
  expect(rel(ts::equibiaxial_stress(m, vec(ref::lam_bi)), bi) < 1e-11, name + ": equibiaxial");
  expect(rel(ts::planar_stress_difference(m, vec(ref::lam_pl)), pl) < 1e-11, name + ": planar");
  const auto s = ts::simple_shear(m, vec(ref::gam_sh));
  const auto a = s.shear_stress.contiguous().host<double>(), b = s.normal_difference.contiguous().host<double>();
  for (std::size_t i = 0; i < ref::gam_sh.size(); ++i)
    expect(std::abs(a[i] - sh[2 * i]) < 1e-11 * (1 + std::abs(sh[2 * i])) && std::abs(b[i] - sh[2 * i + 1]) < 1e-11 * (1 + std::abs(sh[2 * i + 1])), name + ": simple shear");
  // consistency of the energy with W1, W2: central differences in I1 and I2
  const Series i1 = vec({3.4, 4.1, 5.5}), i2 = vec({3.7, 4.6, 6.0});
  const double h = 1e-6;
  expect(m.energy(i1.add(h), i2).sub(m.energy(i1.sub(h), i2)).div(2.0 * h).sub(m.w1(i1, i2)).abs().max().item<double>() < 1e-6, name + ": W1 = dW/dI1");
  expect(m.energy(i1, i2.add(h)).sub(m.energy(i1, i2.sub(h))).div(2.0 * h).sub(m.w2(i1, i2)).abs().max().item<double>() < 1e-6, name + ": W2 = dW/dI2");
}
} // namespace

int main() {
  try {
    check("neo-Hookean", ts::neo_hookean(ref::neo_par[0]), ref::neo_uni, ref::neo_bi, ref::neo_pl, ref::neo_sh);
    check("Mooney-Rivlin", ts::mooney_rivlin(ref::mr_par[0], ref::mr_par[1]), ref::mr_uni, ref::mr_bi, ref::mr_pl, ref::mr_sh);
    check("Yeoh", ts::yeoh(ref::yeoh_par[0], ref::yeoh_par[1], ref::yeoh_par[2]), ref::yeoh_uni, ref::yeoh_bi, ref::yeoh_pl, ref::yeoh_sh);
    check("Gent", ts::gent(ref::gent_par[0], ref::gent_par[1]), ref::gent_uni, ref::gent_bi, ref::gent_pl, ref::gent_sh);
    check("Fung", ts::fung(ref::fung_par[0], ref::fung_par[1]), ref::fung_uni, ref::fung_bi, ref::fung_pl, ref::fung_sh);
    // Ogden (three terms) in principal stretches vs the reduced-energy derivative
    {
      const Series mu = vec(ref::ogden_mu), al = vec(ref::ogden_alpha);
      expect(rel(ts::ogden_uniaxial(vec(ref::lam_uni), mu, al), ref::ogden_uni) < 1e-11, "Ogden uniaxial");
      expect(rel(ts::ogden_equibiaxial(vec(ref::lam_bi), mu, al), ref::ogden_bi) < 1e-11, "Ogden equibiaxial");
      expect(rel(ts::ogden_planar_difference(vec(ref::lam_pl), mu, al), ref::ogden_pl) < 1e-11, "Ogden planar");
    }
    // limiting models and small-strain consistency
    {
      const Series l = vec({1.1, 1.4, 2.0});
      expect(ts::uniaxial_stress(ts::mooney_rivlin(0.9, 0.0), l).sub(ts::uniaxial_stress(ts::neo_hookean(1.8), l)).abs().max().item<double>() < 1e-12, "Mooney-Rivlin(c01 = 0) = neo-Hookean(mu = 2 c10)");
      expect(ts::uniaxial_stress(ts::yeoh(0.9, 0.0, 0.0), l).sub(ts::uniaxial_stress(ts::neo_hookean(1.8), l)).abs().max().item<double>() < 1e-12, "Yeoh(c20 = c30 = 0) = neo-Hookean");
      expect(ts::uniaxial_stress(ts::gent(1.8, 1e9), l).sub(ts::uniaxial_stress(ts::neo_hookean(1.8), l)).abs().max().item<double>() < 1e-6, "Gent(Jm -> inf) = neo-Hookean");
      // Gent stiffens and diverges as I1 - 3 -> Jm (finite extensibility)
      const auto gent = ts::gent(1.0, 2.0);
      expect(ts::uniaxial_stress(gent, vec({2.0})).host<double>().front() > 5.0 * ts::uniaxial_stress(ts::neo_hookean(1.0), vec({2.0})).host<double>().front(), "Gent stiffening near the locking stretch");
      // small strain: sigma ~ E eps with E = 3 mu0 for uniaxial tension, for every model
      const Series eps = vec({1e-6});
      const std::vector<ts::Isotropic> models{ts::neo_hookean(1.7), ts::mooney_rivlin(0.9, 0.35), ts::yeoh(0.8, 0.05, 0.004), ts::gent(1.2, 9.0), ts::fung(0.6, 0.8)};
      for (const auto &m : models) {
        const Series mu0 = ts::initial_shear_modulus(m, eps);
        const double young = ts::uniaxial_stress(m, eps.add(1.0)).div(eps).host<double>().front();
        expect(std::abs(young - 3.0 * mu0.host<double>().front()) < 1e-5 * young, "small-strain Young's modulus = 3 mu0");
      }
      const Series mu = vec({0.63, 0.0012, -0.01}), al = vec({1.3, 5.0, -2.0});
      const double young = ts::ogden_uniaxial(vec({1.0 + 1e-6}), mu, al).host<double>().front() / 1e-6;
      expect(std::abs(young - 3.0 * ts::ogden_shear_modulus(mu, al).item<double>()) < 1e-4 * young, "Ogden small-strain modulus = 3 sum mu alpha / 2");
    }
    // fibre-reinforced artery (HGO / GOH) vs the sympy F-based energy with the Macaulay bracket (last case: fibres in compression carry nothing)
    {
      const std::size_t cases = ref::hgo.size() / 9;
      for (std::size_t i = 0; i < cases; ++i) {
        const auto *c = &ref::hgo[9 * i];
        ts::Hgo m;
        m.c10 = c[2]; m.k1 = c[3]; m.k2 = c[4]; m.kappa = c[5]; m.gamma = c[6];
        const auto s = ts::artery_stresses(m, vec({c[0]}), vec({c[1]}));
        expect(std::abs(s.theta_minus_radial.host<double>().front() - c[7]) < 1e-10 * (1 + std::abs(c[7])) && std::abs(s.axial_minus_radial.host<double>().front() - c[8]) < 1e-10 * (1 + std::abs(c[8])),
               "HGO artery case " + std::to_string(i) + ": " + std::to_string(s.theta_minus_radial.host<double>().front()) + ", " + std::to_string(s.axial_minus_radial.host<double>().front()) + " vs " + std::to_string(c[7]) + ", " + std::to_string(c[8]));
      }
      // no fibres (k1 = 0) is neo-Hookean with mu = 2 c10; stretched fibre families stiffen it
      ts::Hgo m;
      m.c10 = 0.9; m.k1 = 0.0; m.k2 = 3.0; m.kappa = 0.1; m.gamma = 0.7;
      const Series lt = vec({1.15}), lz = vec({1.10});
      const auto s0 = ts::artery_stresses(m, lt, lz);
      const double nh = 2.0 * 0.9 * (1.15 * 1.15 - std::pow(1.0 / (1.15 * 1.10), 2.0));
      expect(std::abs(s0.theta_minus_radial.host<double>().front() - nh) < 1e-12, "k1 = 0 reduces to the neo-Hookean wall");
      m.k1 = 2.4;
      expect(ts::artery_stresses(m, lt, lz).theta_minus_radial.host<double>().front() > s0.theta_minus_radial.host<double>().front(), "fibres stiffen the stretched wall");
      // isotropic limit kappa = 1/3: E = (I1 - 3)/3 independent of the fibre angle, so the response does not depend on gamma
      ts::Hgo a = m, b = m;
      a.kappa = b.kappa = 1.0 / 3.0;
      a.gamma = 0.3;
      b.gamma = 1.2;
      expect(std::abs(ts::artery_stresses(a, lt, lz).theta_minus_radial.host<double>().front() - ts::artery_stresses(b, lt, lz).theta_minus_radial.host<double>().front()) < 1e-12, "kappa = 1/3 is isotropic (fibre angle irrelevant)");
    }
    std::cout << "tissue tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
