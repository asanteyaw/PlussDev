#include "pluss/ma/natsci/biology.h"
#include "hh_reference.h"
#include "pluss/subsp/math/roots.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace dif = bio::biophysics::diffusion;
namespace mem = bio::biophysics::membrane;
namespace spatial = pluss::subsp::natsci::common::spatial;
namespace common = pluss::subsp::natsci::common;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
constexpr double kPi = 3.14159265358979323846;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }
bool close(double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

spatial::PeriodicGrid grid1d(std::int64_t n, double h) {
  return spatial::make_periodic_grid({n}, {common::ScalarQuantity<common::Length>::from_si(h)});
}
Series coords(std::int64_t n, double h, double origin) {
  return pluss::series::arange(0, n, 1, F64).mul(h).add(origin);
}

void test_diffusion_modes() {
  const std::int64_t n = 64;
  const double h = 0.5, d = 0.7;
  const auto g = grid1d(n, h);
  const Series x = coords(n, h, 0.0);
  const double k = 2.0 * kPi * 3.0 / (n * h);
  const Series u0 = x.mul(k).cos();
  const auto rate = [&](const Series &u) { return dif::diffusion_rate(u, d, g); };
  const double lam = d * (2.0 - 2.0 * std::cos(k * h)) / (h * h);
  const double t = 0.8;
  const Series u = dyn::simulate(rate, u0, 0.0, t, 400);
  const double err = u.sub(u0.mul(std::exp(-lam * t))).abs().max().item<double>();
  expect(err < 1e-10, "discrete Fourier-mode decay");
  // mass conservation
  const Series bump = x.sub(16.0).square().neg().exp();
  const Series ub = dyn::simulate(rate, bump, 0.0, 1.0, 200);
  expect(std::abs(ub.sum().item<double>() - bump.sum().item<double>()) < 1e-10, "mass conserved");
  // batched lanes with different D (Series coefficient broadcast on lane axis)
  const Series lanes = bump.unsqueeze(0).unite(std::vector<Series>{bump.unsqueeze(0)}, 0);
  const Series dd = vec({0.2, 1.0}).unsqueeze(-1);
  const auto rb = [&](const Series &u2) { return dif::diffusion_rate(u2, dd, g); };
  const Series ul = dyn::simulate(rb, lanes, 0.0, 1.0, 200);
  const Series s0 = dyn::simulate([&](const Series &q) { return dif::diffusion_rate(q, 0.2, g); }, bump, 0.0, 1.0, 200);
  expect(ul.select(0, 0).sub(s0).abs().max().item<double>() < 1e-12, "batched D lane");
}

void test_heat_kernel() {
  const std::int64_t n = 800;
  const double h = 0.05, d = 0.5;
  const auto g = grid1d(n, h);
  const Series x = coords(n, h, -20.0);
  const Series r2 = x.square();
  const Series u0 = dif::heat_kernel(r2, d, 1.0, 1);
  const Series u = dyn::simulate([&](const Series &q) { return dif::diffusion_rate(q, d, g); }, u0, 0.0, 1.5, 600);
  const Series exact = dif::heat_kernel(r2, d, 2.5, 1);
  expect(u.sub(exact).abs().max().item<double>() < 2e-5, "heat kernel semigroup property");
  expect(std::abs(u.sum().item<double>() * h - 1.0) < 1e-8, "kernel unit mass");
}

void test_fisher_kpp() {
  expect(close(dif::fisher_kpp_min_speed(0.5, 2.0), 2.0, 1e-14), "min speed");
  const double d = 0.8, r = 1.7;
  const double c = dif::fisher_kpp_exact_speed(d, r);
  expect(c > dif::fisher_kpp_min_speed(d, r), "exact speed above minimum");
  double previous = 0.0;
  for (const std::int64_t n : {200, 400}) {
    const double L = 40.0, h = L / static_cast<double>(n);
    const auto g = grid1d(n, h);
    const Series x = coords(n, h, -L / 2);
    const Series u = dif::fisher_kpp_exact(x, 0.0, d, r);
    const Series f = dif::reaction_diffusion_rate(
        u, d, g, [&](const Series &q) { return dif::fisher_kpp_reaction(q, r); });
    // d/dt of the exact solution: -c u_x; u_x analytic
    const double s = std::sqrt(r / d) / std::sqrt(6.0);
    const Series e = x.mul(s).exp();
    const Series ux = e.mul(-2.0 * s).mul(e.add(1.0).pow(-3.0));
    const Series residual = f.add(ux.mul(c));
    const std::int64_t lo = n / 4, hi = 3 * n / 4;
    const double err = residual.slice(0, lo, hi, 1).abs().max().item<double>();
    if (previous > 0.0)
      expect(previous / err > 3.5 && previous / err < 4.6, "second-order spatial convergence");
    previous = err;
  }
  // front moves right: batched r lanes keep ordering of speeds
  const std::int64_t n = 400;
  const double L = 80.0, h = L / n;
  const auto g = grid1d(n, h);
  const Series x = coords(n, h, -L / 2);
  const Series u0 = x.neg().sub(10.0).mul(1.0).sigmoid();
  const Series lanes = u0.unsqueeze(0).unite(std::vector<Series>{u0.unsqueeze(0)}, 0);
  const Series rr = vec({0.5, 2.0}).unsqueeze(-1);
  const auto rhs = [&](const Series &u) {
    return dif::reaction_diffusion_rate(u, 1.0, g, [&](const Series &q) { return dif::fisher_kpp_reaction(q, rr); });
  };
  const Series u = dyn::simulate(rhs, lanes, 0.0, 12.0, 1200);
  const Series pos = u.gt(0.5).to(F64).sum(-1, false).mul(h); // front position (cells behind front)
  expect(pos.select(0, 1).item<double>() > pos.select(0, 0).item<double>() + 5.0, "faster r gives faster front");
  expect(u.min().item<double>() > -1e-6 && u.max().item<double>() < 1.0 + 1e-6, "bounded in [0,1]");
}

void test_membrane() {
  const Series ko = vec({5.0}), ki = vec({140.0});
  expect(close(mem::nernst(ko, ki, 1.0, 26.7).host<double>().front(), 26.7 * std::log(5.0 / 140.0), 1e-14), "nernst K");
  expect(close(mem::nernst(vec({110.0}), vec({10.0}), -1.0, 26.7).host<double>().front(), -26.7 * std::log(11.0), 1e-14), "nernst Cl");
  const Series p = vec({1.0, 0.05, 0.45}); // K, Na, Cl
  const Series out = vec({5.0, 145.0, 110.0}), in = vec({140.0, 12.0, 10.0}), sign = vec({1, 1, -1});
  const double expected = 26.7 * std::log((1.0 * 5 + 0.05 * 145 + 0.45 * 10) / (1.0 * 140 + 0.05 * 12 + 0.45 * 110));
  expect(close(mem::ghk_voltage(p, out, in, sign, 26.7).item<double>(), expected, 1e-13), "GHK voltage");
  // single permeant cation reduces GHK to Nernst
  expect(close(mem::ghk_voltage(vec({1.0}), vec({5.0}), vec({140.0}), vec({1.0}), 26.7).item<double>(),
               mem::nernst(ko, ki, 1.0, 26.7).host<double>().front(), 1e-13), "GHK -> Nernst");
}

void test_hodgkin_huxley() {
  const Series start = mem::hodgkin_huxley_rest(vec({0.0, 0.0, 0.0}));
  expect(start.size(0) == 3 && start.size(1) == 4, "rest shape");
  const Series rest = start.select(0, 0);
  expect(close(at(rest, 1), 0.05293248525724958, 1e-12) && close(at(rest, 2), 0.5961207535084603, 1e-12) &&
             close(at(rest, 3), 0.31767691406069737, 1e-12), "resting gates");
  expect(std::abs(at(mem::hodgkin_huxley_rate(start, 0.0).select(0, 0), 0)) < 0.01, "rest is near equilibrium");
  // singularities of the alpha functions are filled (paper's alpha_n(V=-10), alpha_m(V=-25) removable points;
  // here depolarisation positive, so V = +10 and +25)
  const Series sing = vec({10.0, 25.0});
  expect(close(mem::hh_n(sing).alpha.select(0, 0).item<double>(), 0.1, 1e-6) &&
             close(mem::hh_m(sing).alpha.select(0, 1).item<double>(), 1.0, 1e-6), "removable singularities");
  // Gating rates have the physical direction: depolarisation speeds activation and closes inactivation gates.
  const Series v = vec({0.0, 30.0});
  expect(mem::hh_m(v).alpha.select(0, 1).item<double>() > mem::hh_m(v).alpha.select(0, 0).item<double>() &&
             mem::hh_h(v).beta.select(0, 1).item<double>() > mem::hh_h(v).beta.select(0, 0).item<double>(), "depolarisation opens m and closes h");
  // Independent implementation written in the paper's own sign convention (hh_paper_ref.py, DOP853):
  // lanes = {initial depolarisation 15 mV, 6 mV, rest with 10 uA/cm^2 injected}, observed at six times.
  const Series s0 = mem::hodgkin_huxley_rest(vec({0.0, 0.0, 0.0}))
                        .mul(pluss::series::ones({4}, F64)); // keep the rest gates
  Series init = s0;
  init = pluss::series::make({{15.0, at(rest, 1), at(rest, 2), at(rest, 3)}, {6.0, at(rest, 1), at(rest, 2), at(rest, 3)}, {0.0, at(rest, 1), at(rest, 2), at(rest, 3)}}).to(F64);
  const Series current = vec({0.0, 0.0, 10.0});
  const auto rhs = [&](const Series &s) { return mem::hodgkin_huxley_rate(s, current); };
  const double times[6] = {0.5, 1.0, 2.0, 3.0, 5.0, 8.0};
  Series y = init;
  double t_prev = 0.0;
  const double* refs[3] = {hh_reference::v0_15, hh_reference::v0_6, hh_reference::i10};
  const int ref_col[4] = {0, 2, 3, 1};
  for (int k = 0; k < 6; ++k) {
    y = dyn::simulate(rhs, y, t_prev, times[k], static_cast<std::int64_t>((times[k] - t_prev) / 0.001));
    t_prev = times[k];
    for (int lane = 0; lane < 3; ++lane)
      for (int i = 0; i < 4; ++i)
        // reference columns are (V, n, m, h); the C++ state is (V, m, h, n)
        expect(close(y.select(0, lane).select(-1, i).item<double>(), refs[lane][4 * k + ref_col[i]], 2e-6) ||
                   std::abs(y.select(0, lane).select(-1, i).item<double>() - refs[lane][4 * k + ref_col[i]]) < 2e-7,
               "HH vs independent paper-convention implementation, lane " + std::to_string(lane) + " time index " + std::to_string(k) + " var " + std::to_string(i));
  }
  // The paper's Fig. 12 (6 C): initial depolarisations of 7, 15 and 90 mV give full action potentials of
  // about 100 mV, while 6 mV is "just below threshold": the threshold lies in (6, 7] mV.
  const auto peak_of = [&](double v0) {
    Series z = pluss::series::make({{v0, at(rest, 1), at(rest, 2), at(rest, 3)}}).to(F64);
    const auto path = dyn::trajectory([&](const Series &s) { return mem::hodgkin_huxley_rate(s, 0.0); }, z, 0.0, 12.0, 48, 10);
    return path.states.select(-1, 0).max().item<double>();
  };
  expect(peak_of(6.0) < 7.0, "6 mV initial depolarisation is subthreshold (paper, Fig. 12)");
  const double p7 = peak_of(7.0), p15 = peak_of(15.0);
  expect(p7 > 95.0 && p7 < 112.0 && p15 > 95.0 && p15 < 112.0, "7 and 15 mV initial depolarisations fire ~100 mV action potentials (paper, Fig. 12)");
  // threshold located by lane-wise bisection on the initial depolarisation equals the independent value (6.502 mV)
  const Series lo = vec({6.0}), hi = vec({7.0});
  const Series th = pluss::subsp::math::roots::bisect(
      [&](const Series &v0) {
        const Series z = v0.unsqueeze(-1).unite(std::vector<Series>{vec({at(rest, 1)}).unsqueeze(0), vec({at(rest, 2)}).unsqueeze(0), vec({at(rest, 3)}).unsqueeze(0)}, -1);
        const auto path = dyn::trajectory([&](const Series &s) { return mem::hodgkin_huxley_rate(s, 0.0); }, z, 0.0, 12.0, 24, 10);
        return path.states.select(-1, 0).amax(0, false).sub(40.0);
      },
      lo, hi, {12}).root;
  expect(std::abs(th.host<double>().front() - hh_reference::threshold_v0[0]) < 1e-3, "firing threshold equals the independent paper-convention value");
}

// Table 4 of the paper: membrane action potential from a 15 mV initial depolarisation at 18.5 C:
// spike height 96.8 mV, max rate of rise 564 V/s (= mV/ms), rising phase (20 mV -> peak) 0.275 ms,
// falling phase (peak -> V = 0) 0.61 ms.
void test_table4_membrane_action_potential() {
  mem::HodgkinHuxley p;
  p.celsius = 18.5;
  const Series rest = mem::hodgkin_huxley_rest(vec({0.0})).squeeze(0);
  const Series y0 = pluss::series::make({{15.0, at(rest, 1), at(rest, 2), at(rest, 3)}}).to(F64);
  const auto rhs = [&](const Series &s) { return mem::hodgkin_huxley_rate(s, 0.0, p); };
  const std::int64_t samples = 400; // 0 .. 2 ms in 0.005 ms steps
  const double dt = 2.0 / samples;
  const auto path = dyn::trajectory(rhs, y0, 0.0, 2.0, samples, 5);
  const Series v = path.states.select(-1, 0).squeeze(-1);   // [samples+1]
  const Series t = path.times;
  const double height = v.max().item<double>();
  expect(std::abs(height - 96.8) < 0.6, "spike height vs Table 4 (96.8 mV), got " + std::to_string(height));
  const Series slope = mem::hodgkin_huxley_rate(path.states.squeeze(1), 0.0, p).select(-1, 0);
  const double rise_rate = slope.max().item<double>();
  expect(std::abs(rise_rate - 564.0) < 8.0, "maximum rate of rise vs Table 4 (564 V/s), got " + std::to_string(rise_rate));
  const double t_peak = v.eq(height).to(F64).mul(t).sum().item<double>() / v.eq(height).to(F64).sum().item<double>();
  const Series before = t.lt(t_peak).to(F64);
  const double t_20 = dt * v.lt(20.0).to(F64).mul(before).sum().item<double>(); // samples before the peak still below 20 mV
  expect(std::abs((t_peak - t_20) - 0.275) < 0.012, "rising phase 20 mV -> peak vs Table 4 (0.275 ms), got " + std::to_string(t_peak - t_20));
  const Series after = t.gt(t_peak).to(F64);
  const double t_zero = t_peak + dt * v.gt(0.0).to(F64).mul(after).sum().item<double>();
  expect(std::abs((t_zero - t_peak) - 0.61) < 0.02, "falling phase peak -> V = 0 vs Table 4 (0.61 ms), got " + std::to_string(t_zero - t_peak));
}

// Propagated action potential (paper eq. 29 / Fig. 15 / p. 528): squid axon, a = 238 um, R2 = 35.4 ohm cm,
// C = 1 uF/cm^2, 18.5 C. The paper reports a calculated conduction velocity of 18.8 m/s.
void test_propagated_action_potential() {
  mem::HodgkinHuxley p;
  p.celsius = 18.5;
  const std::int64_t n = 600;
  const double length = 24.0, dx = length / n;       // cm
  const auto g = grid1d(n, dx);
  const Series x = coords(n, dx, 0.0);
  const Series rest = mem::hodgkin_huxley_rest(vec({0.0})).squeeze(0); // [4]
  // rest everywhere, a depolarised stimulus near the left end (x in [0.8, 1.2] cm). On a periodic ring the
  // pulse splits in two; the left-mover wraps around, so only 2.5 < x < 14 cm is tracked and all
  // measurements are taken before the two pulses can reach each other.
  const Series stim = x.ge(0.8).logical_and(x.le(1.2)).to(F64);
  const Series ones = x.mul(0.0).add(1.0);
  const Series state = stim.mul(40.0).unsqueeze(-2).unite(
      std::vector<Series>{ones.mul(at(rest, 1)).unsqueeze(-2), ones.mul(at(rest, 2)).unsqueeze(-2), ones.mul(at(rest, 3)).unsqueeze(-2)}, -2);
  const auto make_rhs = [&](const mem::HodgkinHuxley &params) {
    return [&g, params](const Series &s) { return mem::hodgkin_huxley_axon_rate(s, 0.0238, 35.4, g, params); };
  };
  const double dt = 0.002;
  const auto advance = [&](const dyn::RateFn &rhs, const Series &s, double t0, double t1) {
    return dyn::simulate(rhs, s, t0, t1, static_cast<std::int64_t>(std::llround((t1 - t0) / dt)));
  };
  const Series right = x.gt(2.5).logical_and(x.lt(14.0)).to(F64);
  const auto front = [&](const Series &s) { // centroid of the right-moving region above 30 mV
    const Series above = s.select(-2, 0).gt(30.0).to(F64).mul(right);
    return above.mul(x).sum().item<double>() / std::max(above.sum().item<double>(), 1e-12);
  };
  const auto warm = make_rhs(p);
  const Series s2 = advance(warm, state, 0.0, 2.0);
  const Series s4 = advance(warm, s2, 2.0, 4.0);
  const double speed_m_per_s = (front(s4) - front(s2)) / 2.0 * 10.0; // cm/ms -> m/s
  expect(front(s4) > front(s2) && front(s2) > 2.5, "the right-moving pulse travels away from the stimulus");
  expect(std::abs(speed_m_per_s - 18.8) < 0.03 * 18.8, "conduction velocity matches the paper's 18.8 m/s (measured " + std::to_string(speed_m_per_s) + ")");
  expect(s4.select(-2, 0).max().item<double>() > 85.0 && s4.select(-2, 0).max().item<double>() < 120.0, "propagating pulse keeps a ~100 mV amplitude");
  // Q10 scaling: a colder axon conducts more slowly (all rates x3 per 10 C)
  mem::HodgkinHuxley cold;
  cold.celsius = 8.5;
  const auto cold_rhs = make_rhs(cold);
  const Series c3 = advance(cold_rhs, state, 0.0, 3.0);
  const Series c6 = advance(cold_rhs, c3, 3.0, 6.0);
  const double cold_speed = (front(c6) - front(c3)) / 3.0 * 10.0;
  expect(cold_speed > 0.0 && cold_speed < 0.7 * speed_m_per_s, "slower conduction at lower temperature (measured " + std::to_string(cold_speed) + " m/s)");
}

void test_cable() {
  const std::int64_t n = 128;
  const double h = 0.1, lambda = 0.4, tau = 2.0;
  const auto g = grid1d(n, h);
  const Series x = coords(n, h, 0.0);
  const double k = 2.0 * kPi * 4.0 / (n * h);
  const Series v0 = x.mul(k).cos();
  const auto rate = [&](const Series &v) { return mem::cable_rate(v, lambda, tau, g); };
  const double lam = (1.0 + lambda * lambda * (2.0 - 2.0 * std::cos(k * h)) / (h * h)) / tau;
  const double t = 1.5;
  const Series v = dyn::simulate(rate, v0, 0.0, t, 600);
  expect(v.sub(v0.mul(std::exp(-lam * t))).abs().max().item<double>() < 1e-8, "cable mode decay");
  // uniform drive reaches the steady value V = drive (lambda-independent)
  const Series drive = x.mul(0.0).add(1.0);
  const Series s = dyn::simulate([&](const Series &q) { return mem::cable_rate(q, lambda, tau, g, drive); },
                                 x.mul(0.0), 0.0, 60.0, 4000);
  expect(s.sub(1.0).abs().max().item<double>() < 1e-9, "cable steady state");
}

void test_validation() {
  bool threw = false;
  try { (void)dif::diffusion_rate(vec({1.0, 2.0, 3.0}), -1.0, grid1d(3, 1.0)); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "negative D rejected");
  threw = false;
  try { (void)mem::nernst(vec({1.0}), vec({0.0}), 1.0, 26.7); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "zero concentration rejected");
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
  run("diffusion_modes", test_diffusion_modes);
  run("heat_kernel", test_heat_kernel);
  run("fisher_kpp", test_fisher_kpp);
  run("membrane", test_membrane);
  run("hodgkin_huxley", test_hodgkin_huxley);
  run("table4_membrane_action_potential", test_table4_membrane_action_potential);
  run("propagated_action_potential", test_propagated_action_potential);
  run("cable", test_cable);
  run("validation", test_validation);
  std::cout << "NatSci biophysics checks passed\n";
}
