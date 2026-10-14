#include "../../kinetics/test/h2o2_mechanism.h"
#include "../../kinetics/test/kinetics_reference.h"
#include "equilibrium_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chem = pluss::subsp::natsci::chemistry;
namespace th = chem::thermo;
namespace kin = chem::kinetics;
namespace eq = chem::equilibrium;
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
Series arr(const double *p, std::int64_t n) {
  return pluss::make_series(std::vector<double>(p, p + n), {n}).to(F64);
}
Series rows(const double *p, std::int64_t r, std::int64_t c) {
  return pluss::make_series(std::vector<double>(p, p + r * c), {r, c}).to(F64);
}
bool close(double a, double b, double tol) {
  return std::abs(a - b) <= tol * std::max(1e-300, std::abs(b));
}

kin::Mechanism mechanism() {
  return kin::Mechanism(h2o2::species(), h2o2::reactions(), vec({300.0}),
                        kin::Units::cgs_cal());
}

void expect_moles(const Series &got, const double *want, double tol,
                  const std::string &what) {
  for (std::int64_t k = 0; k < 10; ++k) {
    const double g = got.select(-1, k).item<double>();
    expect(close(g, want[k], tol) || std::abs(g - want[k]) < 1e-14,
           what + " species " + std::to_string(k));
  }
}

// KKT / optimality at fixed (T, P): mu_j/RT = sum_i pi_i a_ij for every species
// that exists, plus the element balance.
void expect_optimal_tp(const kin::Mechanism &m, const eq::Result &r,
                       const Series &b0, const Series &temperature,
                       const Series &pressure, const std::string &what) {
  const Series nj = r.moles;
  const Series mu =
      m.thermo()
          .g_rt(temperature)
          .add(nj.clamp(1e-300, 1e300).log())
          .sub(nj.sum(-1, false).log().unsqueeze(-1))
          .add(pressure.div(th::standard_pressure).log().unsqueeze(-1));
  const Series resid =
      mu.sub(r.potentials.matmul(m.composition().transpose(0, 1)))
          .mul(nj.gt(1e-30).to(F64));
  expect(resid.abs().max().item<double>() < 1e-8,
         what + ": element-potential optimality");
  const Series balance = nj.matmul(m.composition()).sub(b0);
  expect(balance.abs().max().item<double>() <
             1e-10 * std::max(1.0, b0.abs().max().item<double>()),
         what + ": mass balance");
}

void test_tp() {
  const auto m = mechanism();
  const Series b = arr(equilibrium_reference::b_h2o2ar, 4);
  for (int c = 0; c < 3; ++c) {
    const double t = equilibrium_reference::tp_cases_T_P[2 * c],
                 p = equilibrium_reference::tp_cases_T_P[2 * c + 1];
    const auto r = eq::tp(m, vec({t}).squeeze(0), vec({p}).squeeze(0), b);
    expect(r.converged, "tp converged");
    expect_moles(r.moles, &equilibrium_reference::tp_moles[10 * c], 1e-8,
                 "TP case " + std::to_string(c));
    expect_optimal_tp(m, r, b, vec({t}).squeeze(0), vec({p}).squeeze(0),
                      "TP case " + std::to_string(c));
  }
  // batched lanes (different T and P in one call) equal the scalar runs
  const Series lanes_t = vec({2000.0, 3000.0, 3000.0});
  const Series lanes_p = vec({th::standard_pressure, th::standard_pressure,
                              10 * th::standard_pressure});
  const Series lanes_b = b.unsqueeze(0).unite(
      std::vector<Series>{b.unsqueeze(0), b.unsqueeze(0)}, 0);
  const auto rb = eq::tp(m, lanes_t, lanes_p, lanes_b);
  expect(rb.converged && rb.moles.size(0) == 3, "batched tp converged");
  for (int c = 0; c < 3; ++c)
    expect_moles(rb.moles.select(0, c),
                 &equilibrium_reference::tp_moles[10 * c], 1e-8,
                 "batched TP lane " + std::to_string(c));
  // element totals without an inert: Ar and N2 are impossible and must carry
  // zero moles
  const auto rn = eq::tp(m, vec({2000.0}).squeeze(0),
                         vec({th::standard_pressure}).squeeze(0),
                         arr(equilibrium_reference::b_h2o2, 4));
  expect(rn.converged, "tp without inert converged");
  expect_moles(rn.moles, equilibrium_reference::tp_no_inert_2000, 1e-8,
               "TP without inert");
  expect(rn.active.select(-1, m.index("AR")).item<double>() == 0.0 &&
             rn.active.select(-1, m.index("N2")).item<double>() == 0.0,
         "impossible species are masked");
  // elements are conserved exactly
  expect(rn.moles.matmul(m.composition())
                 .sub(arr(equilibrium_reference::b_h2o2, 4))
                 .abs()
                 .max()
                 .item<double>() < 4e-10,
         "elements conserved (no inert)");
}

void test_hp() {
  const auto m = mechanism();
  const auto r = eq::hp(m, vec({th::standard_pressure}).squeeze(0),
                        vec({equilibrium_reference::hp_h0[0]}).squeeze(0),
                        arr(equilibrium_reference::b_h2o2ar, 4));
  expect(r.converged, "hp converged");
  expect(std::abs(r.temperature.item<double>() -
                  equilibrium_reference::hp_T[0]) < 1e-5,
         "HP temperature (2 H2 + O2 + 7 Ar)");
  expect_moles(r.moles, equilibrium_reference::hp_moles, 1e-7,
               "HP composition");
  // stoichiometric H2 / air: adiabatic flame temperature ~2380-2390 K without
  // NOx chemistry
  const auto air = eq::hp(m, vec({th::standard_pressure}).squeeze(0),
                          vec({equilibrium_reference::air_h0[0]}).squeeze(0),
                          arr(equilibrium_reference::b_air, 4));
  expect(air.converged, "hp air converged");
  expect(std::abs(air.temperature.item<double>() -
                  equilibrium_reference::air_T[0]) < 1e-5,
         "HP flame temperature vs independent solve");
  expect(air.temperature.item<double>() > 2370.0 &&
             air.temperature.item<double>() < 2400.0,
         "H2/air adiabatic flame temperature in the published range");
  // enthalpy is conserved by construction: sum n_j H_j(T) = h0
  const Series h = m.thermo().h_rt(air.temperature);
  const double total = air.moles.mul(h).sum().item<double>() *
                       th::gas_constant() * air.temperature.item<double>();
  expect(std::abs(total - equilibrium_reference::air_h0[0]) <
             1e-6 * std::abs(equilibrium_reference::air_h0[0]) + 1e-3,
         "HP enthalpy balance");
}

void test_tv_uv() {
  const auto m = mechanism();
  const double volume = equilibrium_reference::uv_V_u0[0],
               u0 = equilibrium_reference::uv_V_u0[1];
  const Series b = arr(equilibrium_reference::b_uv, 4);
  const auto r = eq::uv(m, vec({volume}).squeeze(0), vec({u0}).squeeze(0), b);
  expect(r.converged, "uv converged");
  expect(std::abs(r.temperature.item<double>() -
                  equilibrium_reference::uv_T[0]) < 1e-5,
         "UV temperature vs independent solve");
  expect_moles(r.moles, equilibrium_reference::uv_moles, 1e-7,
               "UV composition");
  // the closed adiabatic reactor from test_reactor relaxes to this state (Radau
  // reference after 5 ms)
  expect(std::abs(r.temperature.item<double>() -
                  kinetics_reference::final_T[1]) < 1e-4,
         "UV equilibrium equals the reactor's final temperature");
  for (const char *name : {"H2O", "H2", "O2", "OH"}) {
    const auto k = static_cast<std::int64_t>(m.index(name));
    const double conc = r.moles.select(-1, k).item<double>() / volume;
    expect(
        std::abs(conc / kinetics_reference::final_conc_1100_si[k] - 1.0) < 2e-3,
        std::string(
            "UV equilibrium concentration equals the reactor's final state: ") +
            name);
  }
  const auto tv = eq::tv(m, vec({equilibrium_reference::tv_T_V[0]}).squeeze(0),
                         vec({equilibrium_reference::tv_T_V[1]}).squeeze(0), b);
  expect(tv.converged, "tv converged");
  expect_moles(tv.moles, equilibrium_reference::tv_moles, 1e-7,
               "TV composition");
  // TV and TP describe the same state when P is the pressure the TV solution
  // produces
  const auto back = eq::tp(m, tv.temperature, tv.pressure, b);
  expect(back.converged, "tp at the tv pressure converged");
  expect(back.moles.sub(tv.moles).abs().max().item<double>() < 1e-9,
         "TV and TP agree at the induced pressure");
  // lanes: two volumes in one call; more volume (lower pressure) at the same
  // energy dissociates more
  const Series vols = vec({volume, 2.0 * volume});
  const Series us = vec({u0, u0});
  const Series lane_b =
      b.unsqueeze(0).unite(std::vector<Series>{b.unsqueeze(0)}, 0);
  const auto lanes = eq::uv(m, vols, us, lane_b);
  expect(lanes.converged && lanes.temperature.size(0) == 2,
         "batched uv converged");
  expect(std::abs(lanes.temperature.select(0, 0).item<double>() -
                  equilibrium_reference::uv_T[0]) < 1e-5,
         "batched uv lane 0 equals scalar");
  expect(lanes.pressure.select(0, 1).item<double>() <
             lanes.pressure.select(0, 0).item<double>(),
         "doubling the volume lowers the pressure");
}

void test_validation() {
  const auto m = mechanism();
  bool threw = false;
  try {
    (void)eq::tp(m, vec({2000.0}).squeeze(0), vec({101325.0}).squeeze(0),
                 vec({-1.0, 2.0, 0.0, 0.0}));
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "negative element totals rejected");
  threw = false;
  try {
    (void)eq::tp(m, vec({2000.0}).squeeze(0), vec({101325.0}).squeeze(0),
                 vec({0.0, 0.0, 0.0, 0.0}));
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "empty system rejected");
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
  run("tp", test_tp);
  run("hp", test_hp);
  run("tv_uv", test_tv_uv);
  run("validation", test_validation);
  std::cout << "NatSci chemistry equilibrium checks passed\n";
}
