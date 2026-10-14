#include "../../kinetics/test/h2o2_mechanism.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chem = pluss::subsp::natsci::chemistry;
namespace ec = pluss::subsp::natsci::common::electrochem;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) {
  return pluss::series::make(v).to(F64);
}
double at(const Series &s, std::int64_t i) {
  return s.select(-1, i).item<double>();
}
bool close(double a, double b, double tol) {
  return std::abs(a - b) <= tol * std::max(1e-300, std::abs(b));
}

void test_constants_and_nernst() {
  expect(close(ec::faraday(), 96485.33212331001, 1e-12),
         "Faraday constant = e N_A (exact SI value)");
  // Nernst slope at 25 C: (RT/F) ln 10 = 59.16 mV per decade for a one-electron
  // couple
  const Series t = vec({298.15});
  const Series e0 = vec({0.0});
  const double slope = at(ec::nernst(e0, t, 1.0, vec({0.1}))
                              .sub(ec::nernst(e0, t, 1.0, vec({1.0}))),
                          0);
  expect(std::abs(slope * 1000.0 - 59.16) < 0.01,
         "59.16 mV per decade at 298.15 K, got " +
             std::to_string(slope * 1000.0));
  // two-electron couple halves the slope; reaction quotient 1 gives E0
  expect(close(at(ec::nernst(vec({0.34}), t, 2.0, vec({1.0})), 0), 0.34, 1e-14),
         "Q = 1 returns the standard potential");
  const double slope2 = at(ec::nernst(e0, t, 2.0, vec({0.1}))
                               .sub(ec::nernst(e0, t, 2.0, vec({1.0}))),
                           0);
  expect(close(slope2, slope / 2.0, 1e-12),
         "two-electron Nernst slope is half");
  // ion equilibrium potential: K+ 5 mM outside / 140 mM inside at 310 K: (RT/F)
  // ln(5/140) = -89.016 mV
  const double ek =
      at(ec::ion_potential(vec({5.0}), vec({140.0}), 1.0, vec({310.0})), 0);
  expect(std::abs(ek * 1000.0 + 89.0156) < 0.001,
         "K+ equilibrium potential at body temperature, got " +
             std::to_string(ek * 1000.0));
}

// H2 + 1/2 O2 -> H2O(g): reversible fuel-cell voltage E = -dG/(2F) from the
// NASA polynomials against the NIST-JANAF dfG(H2O, g) table (kJ/mol): 298.15 K
// -228.582, 500 K -219.051, 1000 K -192.590, 1500 K -164.376, 2000 K -135.528
// (JANAF reference pressure is 1 bar, the polynomials use 1 atm).
void test_fuel_cell_voltage() {
  const chem::kinetics::Mechanism mech(h2o2::species(), h2o2::reactions(),
                                       vec({300.0}),
                                       chem::kinetics::Units::cgs_cal());
  const double temps[5] = {298.15, 500.0, 1000.0, 1500.0, 2000.0};
  const double janaf[5] = {-228.582, -219.051, -192.590, -164.376, -135.528};
  const Series t =
      pluss::make_series(std::vector<double>(temps, temps + 5), {5}).to(F64);
  const Series g = mech.thermo().g_rt(t); // [5, S] standard g/RT
  const Series dg_rt = g.select(-1, mech.index("H2O"))
                           .sub(g.select(-1, mech.index("H2")))
                           .sub(g.select(-1, mech.index("O2")).mul(0.5));
  const Series delta_g =
      dg_rt.mul(t).mul(chem::thermo::gas_constant()); // J/mol
  const Series e = ec::standard_cell_potential(delta_g, 2.0);
  for (int i = 0; i < 5; ++i) {
    const double e_janaf = -janaf[i] * 1000.0 / (2.0 * ec::faraday());
    expect(std::abs(at(e, i) - e_janaf) < 0.0015,
           "reversible voltage vs JANAF at " + std::to_string(temps[i]) +
               " K: " + std::to_string(at(e, i)) + " vs " +
               std::to_string(e_janaf));
  }
  expect(std::abs(at(e, 0) - 1.1845) < 0.0015,
         "1.185 V for gaseous product water at 298 K");
  // thermodynamic consistency of the polynomials: dE/dT = dS/(nF)
  const double dt = 1e-2;
  const Series tp = vec({1200.0 + dt}),
               tm =
                   vec({1200.0 - dt}); // inside one NASA range (the polynomials
                                       // meet only to ~1e-9 at 1000 K)
  const auto delta_g_at = [&](const Series &tt) {
    const Series gg = mech.thermo().g_rt(tt);
    return gg.select(-1, mech.index("H2O"))
        .sub(gg.select(-1, mech.index("H2")))
        .sub(gg.select(-1, mech.index("O2")).mul(0.5))
        .mul(tt)
        .mul(chem::thermo::gas_constant());
  };
  const double de_dt =
      at(ec::standard_cell_potential(delta_g_at(tp), 2.0)
             .sub(ec::standard_cell_potential(delta_g_at(tm), 2.0))
             .div(2 * dt),
         0);
  const auto s_ref = mech.thermo().s_r(vec({1200.0}));
  const double ds =
      chem::thermo::gas_constant() *
      (at(s_ref, mech.index("H2O")) - at(s_ref, mech.index("H2")) -
       0.5 * at(s_ref, mech.index("O2")));
  expect(std::abs(de_dt - ds / (2.0 * ec::faraday())) < 2e-8,
         "dE/dT = dS/(nF) at 1200 K, difference " +
             std::to_string((de_dt - ds / (2.0 * ec::faraday())) * 1e9) +
             " nV/K");
  // batched Nernst correction for reactant / product partial pressures (Q =
  // p_H2O / (p_H2 p_O2^0.5))
  const Series q = vec({1.0, 0.5 / (0.5 * std::sqrt(0.5))});
  const Series e_actual = ec::nernst(at(e, 0) * pluss::series::ones({2}, F64),
                                     vec({298.15, 298.15}), 2.0, q);
  expect(at(e_actual, 0) == at(e, 0) && at(e_actual, 1) < at(e, 0),
         "higher Q lowers the voltage");
}

void test_butler_volmer() {
  const Series t = vec({298.15, 298.15, 350.0});
  const Series eta = vec({0.01, -0.02, 0.05});
  // symmetric transfer coefficients: i = 2 i0 sinh(F eta / 2RT) exactly
  const Series i = ec::butler_volmer(eta, 10.0, 0.5, 0.5, t);
  const Series sinh_form =
      eta.mul(ec::faraday() / (2.0 * ec::gas_constant()))
          .div(t)
          .exp()
          .sub(
              eta.mul(-ec::faraday() / (2.0 * ec::gas_constant())).div(t).exp())
          .mul(10.0);
  expect(i.sub(sinh_form).abs().max().item<double>() < 1e-12,
         "symmetric Butler-Volmer equals 2 i0 sinh(F eta / 2RT)");
  // linear region: i = i0 (alpha_a + alpha_c) F eta / RT
  const Series small =
      vec({1e-8}); // second-order terms are ~(F eta/RT) ~ 4e-7 relative
  const Series lin = ec::butler_volmer(small, 10.0, 0.6, 0.4, vec({298.15}));
  expect(
      close(at(lin, 0),
            10.0 * 1.0 * ec::faraday() * 1e-8 / (ec::gas_constant() * 298.15),
            1e-6),
      "linearised Butler-Volmer");
  // Tafel region: d eta / d ln|i| -> RT/(alpha F)
  const Series e1 = vec({0.30}), e2 = vec({0.35});
  const Series i1 = ec::butler_volmer(e1, 1e-3, 0.5, 0.5, vec({298.15})),
               i2 = ec::butler_volmer(e2, 1e-3, 0.5, 0.5, vec({298.15}));
  const double decade_slope = 0.05 / std::log10(at(i2, 0) / at(i1, 0));
  expect(std::abs(decade_slope - at(ec::tafel_slope(0.5, vec({298.15})), 0)) <
             1e-6,
         "Tafel slope 2 RT ln10 / F = 118 mV per decade for alpha = 0.5");
  // overpotential inversion (shared Illinois solver) round-trips over lanes
  // spanning many decades
  const Series cur = vec({-5e3, -1.0, 1e-3, 3.0, 8e4});
  const Series tt = pluss::series::ones({5}, F64).mul(333.15);
  const Series eta_back = ec::overpotential(cur, 2.0, 0.55, 0.45, tt);
  expect(ec::butler_volmer(eta_back, 2.0, 0.55, 0.45, tt)
                 .sub(cur)
                 .abs()
                 .div(cur.abs())
                 .max()
                 .item<double>() < 1e-9,
         "overpotential inversion round trip");
  // exchange current rises with temperature (Arrhenius)
  const Series i0 =
      ec::exchange_current_arrhenius(vec({1.0}), 50e3, vec({343.15}), 298.15);
  expect(at(i0, 0) > 1.0 && close(at(i0, 0),
                                  std::exp(50e3 / ec::gas_constant() *
                                           (1.0 / 298.15 - 1.0 / 343.15)),
                                  1e-12),
         "Arrhenius exchange current");
}

void test_debye_huckel() {
  // ionic strength of 0.01 mol/kg NaCl and of 0.01 mol/kg CaCl2 (charges +1,-1
  // / +2,-1)
  const Series nacl = ec::ionic_strength(vec({0.01, 0.01}), vec({1.0, -1.0}));
  expect(close(nacl.item<double>(), 0.01, 1e-14), "I(NaCl 0.01) = 0.01");
  expect(close(ec::ionic_strength(vec({0.01, 0.02}), vec({2.0, -1.0}))
                   .item<double>(),
               0.03, 1e-14),
         "I(CaCl2 0.01) = 0.03");
  // limiting law: log gamma = -0.5091 z^2 sqrt(I)
  expect(close(at(ec::log10_gamma_limiting(vec({0.0001}), 1.0), 0),
               -0.5091 * 0.01, 1e-14),
         "limiting law");
  // Davies mean activity coefficient of NaCl vs the measured values (Robinson &
  // Stokes): 0.904 at 0.01 m, 0.778 at 0.1 m
  const Series g = ec::mean_activity_davies(vec({0.01, 0.1}), 1.0, -1.0);
  expect(std::abs(at(g, 0) / 0.904 - 1.0) < 0.005 &&
             std::abs(at(g, 1) / 0.778 - 1.0) < 0.01,
         "Davies mean activity coefficient of NaCl vs measured values");
  // doubly charged ions interact more strongly (z^2 scaling): Davies log gamma
  // scales by 4
  expect(close(at(ec::log10_gamma_davies(vec({0.01}), 2.0), 0),
               4.0 * at(ec::log10_gamma_davies(vec({0.01}), 1.0), 0), 1e-14),
         "z^2 scaling");
  expect(
      at(ec::log10_gamma_davies(vec({1.0}), 1.0), 0) >
          at(ec::log10_gamma_davies(vec({0.3}), 1.0), 0),
      "Davies term turns activity coefficients back up at high ionic strength");
}

void test_polarisation_curve() {
  const Series cur = vec({1.0, 1e3, 5e3, 8e3, 9.5e3});
  const Series v = ec::polarisation_voltage(
      cur, vec({1.18}).mul(pluss::series::ones({5}, F64)), 1.0, 0.5, 5e-5, 1e4,
      2.0, pluss::series::ones({5}, F64).mul(353.15));
  expect(at(v, 0) > 0.9 && at(v, 0) < 1.18,
         "small current: close to but below the reversible voltage");
  for (int k = 1; k < 5; ++k)
    expect(at(v, k) < at(v, k - 1),
           "voltage falls monotonically with current density");
  expect(at(v, 4) < at(v, 3) - 0.05,
         "concentration loss collapses the voltage near the limiting current");
  bool threw = false;
  try {
    (void)ec::polarisation_voltage(vec({2e4}), vec({1.18}), 1.0, 0.5, 5e-5, 1e4,
                                   2.0, vec({353.15}));
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "current beyond the limiting current is rejected");
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
  run("constants_and_nernst", test_constants_and_nernst);
  run("fuel_cell_voltage", test_fuel_cell_voltage);
  run("butler_volmer", test_butler_volmer);
  run("debye_huckel", test_debye_huckel);
  run("polarisation_curve", test_polarisation_curve);
  std::cout << "NatSci chemistry electrochemistry checks passed\n";
}
