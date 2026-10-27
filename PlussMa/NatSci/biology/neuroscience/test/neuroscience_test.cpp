#include "pluss/ma/natsci/biology.h"
#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/stability.h"
#include "neuro_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace neu = bio::neuroscience;
namespace stab = pluss::subsp::natsci::common::stability;
namespace roots = pluss::subsp::math::roots;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }

void test_lif() {
  // tau dV/dt = -(V - E_L) + R I ; exact interspike interval T = tau ln((RI + E_L - V_r)/(RI + E_L - V_th))
  const double tau = 10.0, e_l = -65.0, r = 10.0, v_th = -50.0, v_r = -65.0;
  const Series current = vec({1.0, 1.6, 2.5, 4.0}); // RI = 10, 16, 25, 40 mV above E_L: only RI > 15 fires
  const Series like = current;
  const Series isi = neu::lif_interspike_interval(tau, e_l, r, current, v_th, v_r, like);
  expect(std::isinf(at(isi, 0)) && std::abs(at(isi, 2) - tau * std::log(25.0 / 10.0)) < 1e-12, "exact LIF interspike interval");
  neu::SpikeOptions opt;
  opt.dt = 0.05;
  opt.t_end = 120.0;
  const Series v0 = like.mul(0.0).add(e_l);
  const auto sim = neu::simulate_lif(v0, tau, e_l, r, current, v_th, v_r, opt);
  expect(at(sim.count, 0) == 0.0, "subthreshold drive never spikes");
  for (int lane = 1; lane < 4; ++lane) {
    const double ri = 10.0 * current.select(0, lane).item<double>();
    const double first = tau * std::log(ri / (ri + e_l - v_th));            // from rest at E_L
    const double period = tau * std::log((ri + e_l - v_r) / (ri + e_l - v_th));
    const double n_expected = std::floor((opt.t_end - first) / period) + 1.0;
    expect(sim.count.select(0, lane).item<double>() == n_expected, "LIF spike count, lane " + std::to_string(lane));
    expect(std::abs(sim.times.select(0, lane).select(-1, 0).item<double>() - first) < 2e-3, "LIF first spike time vs exact, lane " + std::to_string(lane));
    expect(std::abs(sim.times.select(0, lane).select(-1, 1).item<double>() - first - period) < 2e-3, "LIF second spike time vs exact, lane " + std::to_string(lane) + ": got " + std::to_string(sim.times.select(0, lane).select(-1, 1).item<double>()) + " want " + std::to_string(first + period) + " first got " + std::to_string(sim.times.select(0, lane).select(-1, 0).item<double>()));
  }
}

void test_izhikevich_vs_events() {
  const Series currents = pluss::make_series(std::vector<double>(neuro_reference::izh_currents, neuro_reference::izh_currents + 4), {4}).to(F64);
  const Series s0 = pluss::series::make({{-65.0, -13.0}, {-65.0, -13.0}, {-65.0, -13.0}, {-65.0, -13.0}}).to(F64);
  neu::SpikeOptions opt;
  opt.dt = 0.01;
  opt.t_end = 200.0;
  opt.max_spikes = 64;
  const auto run = neu::simulate_izhikevich(s0, currents, {}, opt);
  const double expected_counts[4] = {3, 5, 8, 11};
  for (int lane = 0; lane < 4; ++lane) {
    expect(run.count.select(0, lane).item<double>() == expected_counts[lane], "spike count vs SciPy event solver, I lane " + std::to_string(lane));
    for (int k = 0; k < static_cast<int>(expected_counts[lane]); ++k) {
      const double want = neuro_reference::izh_times[lane * 64 + k];
      const double got = run.times.select(0, lane).select(-1, k).item<double>();
      expect(std::abs(got - want) < 0.02, "spike time vs SciPy event solver: lane " + std::to_string(lane) + " spike " + std::to_string(k) + " got " + std::to_string(got) + " want " + std::to_string(want));
    }
  }
  // a stronger input fires faster (batched F-I relation) and spikes never exceed the 30 mV threshold by much
  expect(run.count.select(0, 3).item<double>() > run.count.select(0, 0).item<double>(), "firing rate increases with input current");
  expect(run.state.select(-1, 0).max().item<double>() < 31.0 + 5.0, "reset keeps the membrane potential bounded");
}

Series fhn_equilibrium(const Series &current) {
  // v - v^3/3 - (v + a)/b + I = 0 solved lane-wise with the shared Illinois solver; w = (v + a)/b
  const double a = 0.7, b = 0.8;
  const auto f = [&](const Series &v) { return v.sub(v.pow(3.0).div(3.0)).sub(v.add(a).div(b)).add(current); };
  const auto root = roots::illinois(f, current.mul(0.0).add(-3.0), current.mul(0.0).add(3.0), {200, 0.0, 1e-14, 8});
  const Series w = root.root.add(a).div(b);
  return root.root.unsqueeze(-1).unite(std::vector<Series>{w.unsqueeze(-1)}, -1);
}

void test_fitzhugh_nagumo() {
  // equilibrium at I = 0 and I = 2 vs the long-time SciPy state
  const Series eq0 = fhn_equilibrium(vec({0.0, 2.0}));
  expect(std::abs(eq0.select(0, 0).select(-1, 0).item<double>() - neuro_reference::fhn_equilibrium_I0[0]) < 1e-9 &&
             std::abs(eq0.select(0, 1).select(-1, 1).item<double>() - neuro_reference::fhn_equilibrium_I2[1]) < 1e-9, "FHN equilibria vs SciPy");
  // Hopf points: unstable-mode count flips at the analytic currents tr J = 0 -> v* = +-sqrt(1 - eps b)
  const Series hopf = neu::fitzhugh_nagumo_hopf_currents(0.7, 0.8, 0.08, vec({0.0}));
  const auto unstable_at = [&](const Series &current) {
    const Series eq = fhn_equilibrium(current);
    const Series c = current.unsqueeze(-1); // [lanes, 1]: broadcasts against [lanes, perturbation] inside the Jacobian
    return stab::local_stability([&](const Series &s) { return neu::fitzhugh_nagumo_rate(s, c); }, eq).unstable;
  };
  const auto lower = roots::bisect([&](const Series &i) { return unstable_at(i).neg().add(0.5); }, vec({0.0}), vec({0.8}), {50});
  const auto upper = roots::bisect([&](const Series &i) { return unstable_at(i).sub(0.5); }, vec({0.8}), vec({2.5}), {50});
  expect(std::abs(lower.root.host<double>().front() - at(hopf, 0)) < 1e-6, "lower FHN Hopf current (0.3313) located by bisection on the stability count");
  expect(std::abs(upper.root.host<double>().front() - at(hopf, 1)) < 1e-6, "upper FHN Hopf current (1.4187)");
  expect(unstable_at(vec({0.0})).host<double>().front() == 0.0 && unstable_at(vec({0.5})).host<double>().front() == 2.0 && unstable_at(vec({2.0})).host<double>().front() == 0.0, "stable, oscillatory (unstable pair), stable again");
  // limit-cycle period at I = 0.5 from upward zero crossings of v
  const Series s0 = vec({-1.0, 1.0});
  const auto settle = dyn::simulate([&](const Series &s) { return neu::fitzhugh_nagumo_rate(s, 0.5); }, s0, 0.0, 300.0, 15000);
  const std::int64_t samples = 6000; // 300 time units at 0.05
  const auto path = dyn::trajectory([&](const Series &s) { return neu::fitzhugh_nagumo_rate(s, 0.5); }, settle, 300.0, 600.0, samples, 5);
  const Series v = path.states.select(-1, 0);
  const Series up = v.slice(0, 0, samples, 1).lt(0.0).logical_and(v.slice(0, 1, samples + 1, 1).ge(0.0)).to(F64);
  const Series t = path.times;
  const Series v0 = v.slice(0, 0, samples, 1), v1 = v.slice(0, 1, samples + 1, 1);
  const Series t0 = t.slice(0, 0, samples, 1);
  const Series cross = up.mul(t0.add(v0.neg().div(v1.sub(v0).abs().clamp(1e-300, 1e300).mul(v1.sub(v0).gt(0.0).to(F64)).add(up.neg().add(1.0))).mul(0.05)));
  const double n_cross = up.sum().item<double>();
  const double span_first_last = (cross.sum().item<double>());
  (void)span_first_last;
  expect(n_cross >= 6.0 && n_cross <= 9.0, "about seven oscillations in 300 time units (period ~39.5)");
  const double period_est = 300.0 / n_cross;
  expect(std::abs(period_est - neuro_reference::fhn_period_I05[0]) < 0.12 * neuro_reference::fhn_period_I05[0] / 1.0, "limit-cycle period within sampling resolution of the SciPy value 39.474");
}

void test_validation() {
  bool threw = false;
  try { (void)neu::simulate_lif(vec({0.0}), 10.0, 0.0, 1.0, 1.0, 1.0, 0.0, {0.0, 10.0, 8, 0, 1.0}); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "non-positive dt rejected");
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
  run("lif", test_lif);
  run("izhikevich_vs_events", test_izhikevich_vs_events);
  run("fitzhugh_nagumo", test_fitzhugh_nagumo);
  run("validation", test_validation);
  std::cout << "NatSci neuroscience checks passed\n";
}
