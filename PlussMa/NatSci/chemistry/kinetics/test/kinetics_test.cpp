#include "h2o2_mechanism.h"
#include "kinetics_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chem = pluss::subsp::natsci::chemistry;
namespace th = chem::thermo;
namespace kin = chem::kinetics;
namespace rea = chem::reactor;
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
double at(const Series &s, std::int64_t i) {
  return s.select(-1, i).item<double>();
}
bool close(double a, double b, double tol) {
  return std::abs(a - b) <= tol * std::max(1e-300, std::abs(b));
}

kin::Mechanism mechanism() {
  return kin::Mechanism(h2o2::species(), h2o2::reactions(), vec({300.0}),
                        kin::Units::cgs_cal());
}
std::int64_t idx(const kin::Mechanism &m, const char *n) { return m.index(n); }

void test_thermo() {
  const auto m = mechanism();
  const double r = th::gas_constant();
  expect(close(r, 8.31446261815324, 1e-14),
         "gas constant from the shared CODATA table");
  const Series t298 = vec({298.15}).squeeze(0);
  const Series h = m.thermo().h_rt(t298).mul(r * 298.15 / 1000.0); // kJ/mol
  const Series s = m.thermo().s_r(t298).mul(r);                    // J/mol/K
  const Series cp = m.thermo().cp_r(t298).mul(r);
  // NIST WebBook / NIST-JANAF (Chase 1998): H2O dfH = -241.826 kJ/mol, S =
  // 188.835 J/mol/K (1 bar)
  expect(std::abs(at(h, idx(m, "H2O")) + 241.826) < 0.01,
         "H2O formation enthalpy vs NIST");
  expect(std::abs(at(s, idx(m, "H2O")) - 188.835) < 0.02,
         "H2O entropy vs NIST");
  expect(std::abs(at(s, idx(m, "H2")) - 130.680) < 0.01, "H2 entropy vs NIST");
  expect(std::abs(at(s, idx(m, "O2")) - 205.152) < 0.02, "O2 entropy vs NIST");
  expect(std::abs(at(s, idx(m, "OH")) - 183.71) < 0.05,
         "OH entropy vs NIST-JANAF");
  expect(std::abs(at(h, idx(m, "H2"))) < 1e-6 &&
             std::abs(at(h, idx(m, "O2"))) < 1e-6,
         "elements have zero formation enthalpy");
  expect(std::abs(at(cp, idx(m, "H2O")) - 33.59) < 0.02, "H2O heat capacity");
  // continuity of the two NASA ranges at the mid temperature (guards typos in
  // the transcription)
  const Series below = vec({1000.0 - 1e-9}).squeeze(0),
               above = vec({1000.0 + 1e-9}).squeeze(0);
  for (const auto &fn : {0, 1, 2}) {
    const Series lo = fn == 0   ? m.thermo().cp_r(below)
                      : fn == 1 ? m.thermo().h_rt(below)
                                : m.thermo().s_r(below);
    const Series hi = fn == 0   ? m.thermo().cp_r(above)
                      : fn == 1 ? m.thermo().h_rt(above)
                                : m.thermo().s_r(above);
    expect(lo.sub(hi).abs().div(hi.abs().add(1.0)).max().item<double>() < 1e-6,
           "NASA ranges continuous at 1000 K");
  }
  // thermodynamic consistency: dh/dT = cp and ds/dT = cp/T, over a batch of
  // temperatures
  const Series ts = vec({400.0, 800.0, 1500.0, 2500.0});
  const double d = 1e-3;
  const Series hp = m.thermo().h_rt(ts.add(d)).mul(ts.add(d).unsqueeze(-1));
  const Series hm = m.thermo().h_rt(ts.sub(d)).mul(ts.sub(d).unsqueeze(-1));
  expect(hp.sub(hm)
                 .div(2 * d)
                 .sub(m.thermo().cp_r(ts))
                 .abs()
                 .max()
                 .item<double>() < 1e-6,
         "dh/dT = cp");
  const Series sp = m.thermo().s_r(ts.add(d)), sm = m.thermo().s_r(ts.sub(d));
  expect(sp.sub(sm)
                 .div(2 * d)
                 .sub(m.thermo().cp_r(ts).div(ts.unsqueeze(-1)))
                 .abs()
                 .max()
                 .item<double>() < 1e-6,
         "ds/dT = cp/T");
  bool threw = false;
  try {
    m.thermo().check(vec({150.0}));
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "temperature below the NASA range is rejected");
}

void test_rate_constants_and_equilibrium() {
  const auto m = mechanism();
  const Series c = arr(kinetics_reference::conc_si, 10);
  const Series temps = arr(kinetics_reference::temps, 3);
  const Series conc = c.unsqueeze(0).unite(
      std::vector<Series>{c.unsqueeze(0), c.unsqueeze(0)}, 0);
  const Series kf = m.forward_rate_constants(temps, conc);
  const Series kc = m.equilibrium_constants(temps);
  expect(kf.size(0) == 3 && kf.size(1) == 29, "rate-constant shape");
  for (int s = 0; s < 3; ++s)
    for (int r = 0; r < 29; ++r) {
      expect(
          close(kf.select(0, s).select(-1, r).item<double>(),
                kinetics_reference::kf_si[s * 29 + r], 1e-10),
          "forward rate constant vs independent CGS implementation, reaction " +
              std::to_string(r + 1));
      expect(close(kc.select(0, s).select(-1, r).item<double>(),
                   kinetics_reference::kc_si[s * 29 + r], 1e-9),
             "equilibrium constant vs independent implementation, reaction " +
                 std::to_string(r + 1));
    }
  // hand-evaluated Arrhenius: H + O2 <=> O + OH at 1500 K (reaction 11), SI
  // units
  const double k_hand = 2.65e16 * std::pow(1500.0, -0.6707) *
                        std::exp(-17041.0 / (1.98720425864083 * 1500.0)) * 1e-6;
  expect(close(kf.select(0, 1).select(-1, 10).item<double>(), k_hand, 1e-9),
         "Arrhenius by hand");
  // net production rates vs the independent implementation (three states,
  // batched)
  const Series w = m.net_production_rates(temps, conc);
  for (int s = 0; s < 3; ++s)
    for (int k = 0; k < 10; ++k)
      expect(close(w.select(0, s).select(-1, k).item<double>(),
                   kinetics_reference::wdot_si[s * 10 + k], 1e-8) ||
                 std::abs(w.select(0, s).select(-1, k).item<double>() -
                          kinetics_reference::wdot_si[s * 10 + k]) < 1e-12,
             "production rate vs independent implementation, state " +
                 std::to_string(s) + " species " + std::to_string(k));
  // mass conservation of every reaction: element balance of the net
  // stoichiometry
  const Series balance = m.stoichiometry().matmul(m.composition());
  expect(balance.abs().max().item<double>() < 1e-14,
         "every reaction conserves elements");
}

void test_detailed_balance() {
  const auto m = mechanism();
  // Choose a state that satisfies OH + H2 <=> H + H2O (reaction 21) exactly:
  // forward and reverse fluxes must cancel, which exercises k_r = k_f / Kc.
  const Series t = vec({1300.0});
  const Series kc = m.equilibrium_constants(t);
  const double kc21 = kc.select(-1, 20).host<double>().front();
  std::vector<double> conc(10, 1.0);
  conc[static_cast<std::size_t>(idx(m, "OH"))] = 2.0e-3;
  conc[static_cast<std::size_t>(idx(m, "H2"))] = 0.7;
  conc[static_cast<std::size_t>(idx(m, "H"))] = 3.0e-3;
  conc[static_cast<std::size_t>(idx(m, "H2O"))] =
      kc21 * 2.0e-3 * 0.7 / 3.0e-3; // Kc = [H][H2O]/([OH][H2])
  const Series c = pluss::make_series(conc, {1, 10}).to(F64);
  const auto q = m.rates_of_progress(t, c);
  expect(std::abs(q.forward.select(-1, 20).host<double>().front() /
                      q.reverse.select(-1, 20).host<double>().front() -
                  1.0) < 1e-10,
         "forward = reverse at Kc");
  // irreversible reactions have no reverse flux
  auto specs = h2o2::reactions();
  specs[10].reversible = false;
  const kin::Mechanism irr(h2o2::species(), specs, vec({300.0}),
                           kin::Units::cgs_cal());
  expect(irr.rates_of_progress(t, c)
                 .reverse.select(-1, 10)
                 .abs()
                 .max()
                 .item<double>() == 0.0,
         "irreversible reaction has no reverse flux");
}

void test_reactor() {
  const auto m = mechanism();
  const double ig_ref[3] = {kinetics_reference::ignition_s[0],
                            kinetics_reference::ignition_s[1],
                            kinetics_reference::ignition_s[2]};
  const double t0s[3] = {1000.0, 1100.0, 1200.0};
  const double r = th::gas_constant();
  chem::reactor::stiff::Options opt;
  opt.rtol = 1e-6;
  opt.atol = 1e-18;
  double previous = 1e9;
  for (int i = 0; i < 3; ++i) {
    // 2 H2 + O2 + 7 Ar at 1 atm
    const double ctot = th::standard_pressure / (r * t0s[i]);
    std::vector<double> conc(10, 0.0);
    conc[static_cast<std::size_t>(idx(m, "H2"))] = 0.2 * ctot;
    conc[static_cast<std::size_t>(idx(m, "O2"))] = 0.1 * ctot;
    conc[static_cast<std::size_t>(idx(m, "AR"))] = 0.7 * ctot;
    const Series y0 = rea::make_state(
        vec({t0s[i]}), pluss::make_series(conc, {1, 10}).to(F64));
    // sampled path for the maximum-dT/dt definition
    std::vector<double> times;
    const int samples = 250;
    for (int k = 0; k <= samples; ++k)
      times.push_back(2.2 * ig_ref[i] * k / samples);
    const Series path = rea::trajectory(m, y0, times, true, opt);
    const Series temps = path.select(-1, 0); // [samples+1, 1]
    const double ig = rea::max_slope_time(times, temps).host<double>().front();
    expect(std::abs(ig / ig_ref[i] - 1.0) < 0.015,
           "ignition delay vs independent implementation at T0 = " +
               std::to_string(t0s[i]));
    expect(ig < previous, "ignition delay decreases with temperature");
    previous = ig;
    // conserved quantities along the path
    const Series elements = rea::element_totals(m, path);
    expect(elements.sub(elements.select(0, 0))
                   .abs()
                   .div(elements.select(0, 0).add(1e-30))
                   .max()
                   .item<double>() < 1e-8,
           "elements conserved");
    const Series u = rea::internal_energy_density(m, path);
    expect(u.sub(u.select(0, 0)).abs().max().item<double>() /
                   std::abs(th::standard_pressure) <
               1e-4,
           "internal energy conserved (adiabatic, constant volume)");
    expect(rea::concentrations(path).min().item<double>() > -1e-12,
           "concentrations stay non-negative");
  }
  // final state after 5 ms at T0 = 1100 K vs the independent integration
  // (Radau, rtol 1e-10)
  const double ctot = th::standard_pressure / (r * 1100.0);
  std::vector<double> conc(10, 0.0);
  conc[static_cast<std::size_t>(idx(m, "H2"))] = 0.2 * ctot;
  conc[static_cast<std::size_t>(idx(m, "O2"))] = 0.1 * ctot;
  conc[static_cast<std::size_t>(idx(m, "AR"))] = 0.7 * ctot;
  const Series y0 =
      rea::make_state(vec({1100.0}), pluss::make_series(conc, {1, 10}).to(F64));
  const auto end = rea::integrate(m, y0, 5e-3, true, opt);
  expect(end.converged, "reactor integration converged");
  expect(std::abs(end.y.select(-1, 0).host<double>().front() -
                  kinetics_reference::final_T[1]) < 1.0,
         "final temperature vs independent implementation");
  for (const char *name : {"H2O", "H2", "O2", "OH"}) {
    const auto k = static_cast<std::int64_t>(idx(m, name));
    expect(std::abs(end.y.select(-1, 1 + k).host<double>().front() /
                        kinetics_reference::final_conc_1100_si[k] -
                    1.0) < 0.01,
           std::string("final concentration vs independent implementation: ") +
               name);
  }
  // isothermal variant keeps T fixed
  const auto iso = rea::integrate(m, y0, 2e-4, false, opt);
  expect(std::abs(iso.y.select(-1, 0).host<double>().front() - 1100.0) < 1e-9,
         "isothermal reactor holds temperature");
}

void test_validation() {
  auto specs = h2o2::reactions();
  specs[0].reactants["XX"] = 1.0;
  bool threw = false;
  try {
    (void)kin::Mechanism(h2o2::species(), specs, vec({300.0}),
                         kin::Units::cgs_cal());
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "unknown species rejected");
  specs = h2o2::reactions();
  specs[2].a = -1.0;
  threw = false;
  try {
    (void)kin::Mechanism(h2o2::species(), specs, vec({300.0}),
                         kin::Units::cgs_cal());
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "non-positive pre-exponential rejected");
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
  run("thermo", test_thermo);
  run("rate_constants_and_equilibrium", test_rate_constants_and_equilibrium);
  run("detailed_balance", test_detailed_balance);
  run("reactor", test_reactor);
  run("validation", test_validation);
  std::cout << "NatSci chemistry kinetics checks passed\n";
}
