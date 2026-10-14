#include "../../kinetics/test/h2o2_mechanism.h"
#include "transport_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chem = pluss::subsp::natsci::chemistry;
namespace tr = chem::transport;
namespace tref = transport_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
const int kept[9] = {0, 1, 2, 3, 4,
                     6, 7, 8, 9}; // all species except polar H2O (index 5)
const char *names[10] = {"H2",  "H",   "O",    "O2", "OH",
                         "H2O", "HO2", "H2O2", "AR", "N2"};

Series temps() {
  return pluss::make_series(std::vector<double>(tref::temps, tref::temps + 3),
                            {3})
      .to(F64);
}
Series drop_h2o(const Series &s) { // [..., 10] -> [..., 9]
  std::vector<Series> cols;
  for (const int i : kept)
    cols.push_back(s.select(-1, i).unsqueeze(-1));
  return cols[0].unite(std::vector<Series>(cols.begin() + 1, cols.end()), -1);
}
Series x9(const double *x10) {
  std::vector<double> v;
  for (const int i : kept)
    v.push_back(x10[i]);
  return pluss::make_series(v, {9}).to(F64);
}

struct Setup {
  chem::kinetics::Mechanism mech;
  std::vector<tr::SpeciesData> data;
  Setup()
      : mech(h2o2::species(), h2o2::reactions(),
             pluss::series::make({300.0}).to(F64),
             chem::kinetics::Units::cgs_cal()) {
    for (const int i : kept) {
      tr::SpeciesData s;
      s.name = names[i];
      s.geometry = static_cast<int>(tref::geometry[i]);
      s.well_depth_k = tref::eps_over_k[i];
      s.diameter_angstrom = tref::sigma_angstrom[i];
      s.rotational_relaxation = tref::zrot[i];
      s.molar_mass = tref::molar_mass_kg_per_mol[i];
      data.push_back(s);
    }
  }
  tr::GasTransport engine() const {
    return tr::GasTransport(data, pluss::series::make({300.0}).to(F64));
  }
  Series cp_r(const Series &t) const {
    return drop_h2o(mech.thermo().eval(t).cp_r);
  }
};

void test_pure_species_vs_cantera() {
  Setup su;
  const auto gt = su.engine();
  const Series t = temps();
  const auto sp = gt.species_properties(t, su.cp_r(t));
  double worst_eta = 0, worst_lambda = 0, worst_d = 0;
  for (int s = 0; s < 3; ++s)
    for (int k = 0; k < 9; ++k) {
      const int ki = kept[k];
      worst_eta = std::max(
          worst_eta,
          std::abs(sp.viscosity.select(0, s).select(-1, k).item<double>() /
                       tref::pure_viscosity[s * 10 + ki] -
                   1.0));
      worst_lambda = std::max(
          worst_lambda,
          std::abs(sp.conductivity.select(0, s).select(-1, k).item<double>() /
                       tref::pure_conductivity[s * 10 + ki] -
                   1.0));
      for (int j = 0; j < 9; ++j)
        worst_d = std::max(
            worst_d,
            std::abs(
                sp.binary_diffusion.select(0, s)
                        .select(0, k)
                        .select(-1, j)
                        .item<double>() /
                    101325.0 /
                    tref::binary_diffusion_1atm[s * 100 + ki * 10 + kept[j]] -
                1.0));
    }
  // analytic Neufeld fits vs Cantera's Monchick-Mason tables: a few tenths of a
  // percent
  expect(worst_eta < 0.004, "pure-species viscosity vs Cantera, worst " +
                                std::to_string(worst_eta));
  expect(worst_lambda < 0.010, "pure-species conductivity vs Cantera, worst " +
                                   std::to_string(worst_lambda));
  expect(worst_d < 0.0015,
         "binary diffusion vs Cantera, worst " + std::to_string(worst_d));
  // binary diffusion is symmetric
  expect(sp.binary_diffusion.sub(sp.binary_diffusion.transpose(-1, -2))
                 .abs()
                 .max()
                 .item<double>() < 1e-18,
         "D_jk is symmetric");
}

void test_mixtures_vs_cantera() {
  Setup su;
  const auto gt = su.engine();
  const Series t = temps();
  const auto sp = gt.species_properties(t, su.cp_r(t));
  const Series pressure = pluss::series::zeros({3}, F64).add(101325.0);
  struct Case {
    const double *x, *eta, *lambda, *d_mole, *d_chemkin;
  };
  const Case cases[2] = {
      {tref::x_a, tref::mix_viscosity_a, tref::mix_conductivity_a,
       tref::mix_diff_mole_a, tref::mix_diff_default_a},
      {tref::x_b, tref::mix_viscosity_b, tref::mix_conductivity_b,
       tref::mix_diff_mole_b, tref::mix_diff_default_b}};
  for (int c = 0; c < 2; ++c) {
    // the same composition on three temperature lanes
    const Series x =
        x9(cases[c].x)
            .unsqueeze(0)
            .unite(std::vector<Series>{x9(cases[c].x).unsqueeze(0),
                                       x9(cases[c].x).unsqueeze(0)},
                   0);
    const Series eta = gt.mixture_viscosity(sp, x),
                 lam = gt.mixture_conductivity(sp, x);
    const Series dm = gt.mixture_diffusion(sp, x, pressure),
                 dchem = gt.mixture_diffusion_chemkin(sp, x, pressure);
    for (int s = 0; s < 3; ++s) {
      expect(std::abs(eta.select(0, s).item<double>() / cases[c].eta[s] - 1.0) <
                 0.005,
             "mixture viscosity vs Cantera, composition " + std::to_string(c));
      expect(
          std::abs(lam.select(0, s).item<double>() / cases[c].lambda[s] - 1.0) <
              0.012,
          "mixture conductivity vs Cantera, composition " + std::to_string(c));
      for (int k = 0; k < 9; ++k) {
        if (cases[c].x[kept[k]] < 1e-12)
          continue; // species absent from the mixture: Cantera floors its mole
                    // fraction differently
        expect(std::abs(dm.select(0, s).select(-1, k).item<double>() /
                            cases[c].d_mole[s * 10 + kept[k]] -
                        1.0) < 0.004,
               "mixture-averaged diffusion (mole) vs Cantera");
        expect(std::abs(dchem.select(0, s).select(-1, k).item<double>() /
                            cases[c].d_chemkin[s * 10 + kept[k]] -
                        1.0) < 0.004,
               "CHEMKIN mixture-averaged diffusion vs Cantera mix_diff_coeffs");
      }
    }
  }
}

void test_limits_and_lanes() {
  Setup su;
  const auto gt = su.engine();
  const Series t = temps();
  const auto sp = gt.species_properties(t, su.cp_r(t));
  // a pure gas: mixture rules return the pure-species values exactly (Wilke
  // phi_kk = 1; harmonic mean of equal values)
  const int ar = 7; // position of AR among the nine kept species (H2, H, O, O2,
                    // OH, HO2, H2O2, AR, N2)
  std::vector<double> pure(9, 0.0);
  pure[ar] = 1.0;
  const Series xp = pluss::make_series(pure, {9}).to(F64).unsqueeze(0).mul(
      pluss::series::ones({3, 1}, F64));
  const Series eta = gt.mixture_viscosity(sp, xp);
  expect(eta.sub(sp.viscosity.select(-1, ar))
                 .abs()
                 .div(sp.viscosity.select(-1, ar))
                 .max()
                 .item<double>() < 1e-9,
         "Wilke mixture of one gas equals its viscosity");
  expect(gt.mixture_conductivity(sp, xp)
                 .sub(sp.conductivity.select(-1, ar))
                 .abs()
                 .div(sp.conductivity.select(-1, ar))
                 .max()
                 .item<double>() < 1e-9,
         "conductivity mixture of one gas equals its conductivity");
  // kinetic-theory scalings: viscosity grows with T, conductivity of an atom is
  // (15/4) R eta / M exactly (no internal modes)
  expect(sp.viscosity.select(0, 2).select(-1, ar).item<double>() >
             sp.viscosity.select(0, 0).select(-1, ar).item<double>(),
         "viscosity increases with temperature");
  const double r = chem::thermo::gas_constant();
  const double ar_lambda =
      sp.conductivity.select(0, 1).select(-1, ar).item<double>();
  const double ar_eta = sp.viscosity.select(0, 1).select(-1, ar).item<double>();
  const double ar_mass = tref::molar_mass_kg_per_mol[8];
  expect(std::abs(ar_lambda / (3.75 * r * ar_eta / ar_mass) - 1.0) < 1e-12,
         "monatomic gas: lambda = (15/4)(R/M) eta");
  // diffusion scales as 1/p
  const Series x =
      x9(tref::x_a).unsqueeze(0).mul(pluss::series::ones({3, 1}, F64));
  const Series p1 = pluss::series::zeros({3}, F64).add(101325.0),
               p2 = p1.mul(2.0);
  expect(gt.mixture_diffusion(sp, x, p2)
                 .mul(2.0)
                 .sub(gt.mixture_diffusion(sp, x, p1))
                 .abs()
                 .max()
                 .item<double>() < 1e-15,
         "diffusion is inversely proportional to pressure");
  // lanes with different compositions evaluate independently
  const Series xb = x9(tref::x_b);
  const Series both = x9(tref::x_a).unsqueeze(0).unite(
      std::vector<Series>{xb.unsqueeze(0), xb.unsqueeze(0)}, 0);
  const Series e_all = gt.mixture_viscosity(sp, both);
  const Series e_b = gt.mixture_viscosity(
      sp, xb.unsqueeze(0).mul(pluss::series::ones({3, 1}, F64)));
  expect(std::abs(e_all.select(0, 1).item<double>() -
                  e_b.select(0, 1).item<double>()) < 1e-15,
         "lanes are independent");
}

void test_validation() {
  Setup su;
  auto data = su.data;
  data[0].dipole = 1e-30;
  bool threw = false;
  try {
    (void)tr::GasTransport(data, pluss::series::make({300.0}).to(F64));
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "polar species are rejected");
}
} // namespace

int main() {
  const auto run = [](const char *name, void (*fn)()) {
    try {
      fn();
    } catch (const std::exception &e) {
      std::cerr << name << ": " << e.what() << "\n";
      std::exit(1);
    }
  };
  run("pure_species_vs_cantera", test_pure_species_vs_cantera);
  run("mixtures_vs_cantera", test_mixtures_vs_cantera);
  run("limits_and_lanes", test_limits_and_lanes);
  run("validation", test_validation);
  std::cout << "NatSci chemistry transport checks passed\n";
}
