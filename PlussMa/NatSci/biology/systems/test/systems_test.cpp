#include "pluss/ma/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace kin = bio::systems::kinetics;
namespace net = bio::systems::network;
namespace gene = bio::systems::gene_networks;
namespace sto = bio::population::stochastic;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }
bool close(double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

void test_kinetics() {
  const Series s = vec({2.0});
  expect(close(kin::michaelis_menten(s, 3.0, 1.0).item<double>(), 2.0, 1e-14), "MM");
  expect(close(kin::michaelis_constant(vec({1.0}), vec({0.5}), vec({2.0})).item<double>(), 0.75, 1e-14), "Km");
  expect(close(kin::hill_activation(s, 4.0, 2.0, 3.0).item<double>(), 2.0, 1e-14), "hill act half");
  expect(close(kin::hill_repression(s, 4.0, 2.0, 3.0).item<double>(), 2.0, 1e-14), "hill rep half");
  expect(close(kin::hill_activation(s, 1.0, 1.0, 2.0).item<double>() +
                   kin::hill_repression(s, 1.0, 1.0, 2.0).item<double>(), 1.0, 1e-14), "hill complement");
  expect(close(kin::competitive_inhibition(s, vec({1.0}), 3.0, 1.0, 0.5).item<double>(), 6.0 / 5.0, 1e-14), "competitive");
}

// species (S, E, ES, P); S+E->ES, ES->S+E, ES->E+P
net::Network enzyme() {
  return net::make(mat({{-1, -1, 1, 0}, {1, 1, -1, 0}, {0, 1, -1, 1}}),
                   mat({{1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}}));
}

void test_network_ode_and_jacobian() {
  const auto n = enzyme();
  const Series k = vec({2.0, 1.0, 0.5});
  const auto rate = [&](const Series &x) { return net::rate(n, x, k); };
  const Series y = dyn::simulate(rate, vec({2.0, 0.3, 0.0, 0.0}), 0, 3, 1500);
  const double ref[4] = {1.50762872, 0.09874006, 0.20125994, 0.29111134};
  for (int i = 0; i < 4; ++i)
    expect(close(at(y, i), ref[i], 1e-7), "enzyme mechanism vs scipy");
  // conservation: E+ES and S+ES+P
  expect(close(at(y, 1) + at(y, 2), 0.3, 1e-10), "enzyme conserved");
  expect(close(at(y, 0) + at(y, 2) + at(y, 3), 2.0, 1e-10), "substrate conserved");
  // analytic Jacobian vs central differences, batched over 3 lanes
  const Series x = mat({{2.0, 0.3, 0.1, 0.2}, {1.0, 0.5, 0.3, 0.0}, {0.4, 0.2, 0.9, 1.1}});
  const Series j = net::jacobian(n, x, k);
  expect(j.size(0) == 3 && j.size(1) == 4 && j.size(2) == 4, "jacobian shape");
  const double h = 1e-6;
  double worst = 0.0;
  for (int c = 0; c < 4; ++c) {
    const Series unit = pluss::series::eye(4, F64).select(0, c).unsqueeze(0);
    const Series fd = net::rate(n, x.add(unit.mul(h)), k).sub(net::rate(n, x.sub(unit.mul(h)), k)).div(2 * h);
    worst = std::max(worst, fd.sub(j.select(-1, c)).abs().max().item<double>());
  }
  expect(worst < 1e-7, "jacobian vs finite difference");
}

// 2A <-> D : orders A=2 forward, D=1 reverse
net::Network dimer() {
  return net::make(mat({{-2, 1}, {2, -1}}), mat({{2, 0}, {0, 1}}));
}

void test_steady_state() {
  const auto n = dimer();
  const Series k = vec({1.0, 0.5});
  const auto r = net::steady_state(n, k, vec({1.0, 1.0}), mat({{1.0, 2.0}}), vec({3.0}));
  expect(r.converged, "newton converged");
  expect(close(at(r.state, 0), 0.75, 1e-10) && close(at(r.state, 1), 1.125, 1e-10), "dimer steady state");
  // batched totals
  const Series totals = mat({{3.0}, {6.0}});
  const Series x0 = mat({{1.0, 1.0}, {1.0, 1.0}});
  const auto rb = net::steady_state(n, k, x0, mat({{1.0, 2.0}}), totals);
  expect(rb.converged, "batched newton");
  const double a2 = (-1 + std::sqrt(1 + 8 * 1.0 * 6.0 / 0.5)) / (4 * 1.0 / 0.5);
  expect(close(rb.state.select(0, 1).select(-1, 0).item<double>(), a2, 1e-10), "batched steady state lane 2");
}

void test_gene_networks() {
  // Paper parameters (Elowitz & Leibler 2000): alpha=216, alpha0=0.216, beta=0.2, n=2.
  const auto rep = [](const Series &s) { return gene::repressilator_rate(s, 216.0, 0.216, 0.2, 2.0); };
  const Series r = dyn::simulate(rep, vec({1, 0, 0, 2, 0, 0.5}), 0, 20, 8000);
  const double rref[6] = {25.1160522329, 4.6682304288, 0.6575655910, 10.1832486018, 19.6435100097, 2.4102996503};
  for (int i = 0; i < 6; ++i)
    expect(close(at(r, i), rref[i], 1e-6), "repressilator vs scipy");
  // sustained oscillation: late-time protein range matches the scipy limit cycle (1.25 .. 59.2)
  const Series settled = dyn::simulate(rep, vec({1, 0, 0, 2, 0, 0.5}), 0, 100, 5000);
  const auto cycle = dyn::trajectory(rep, settled, 100.0, 200.0, 200, 25);
  const Series p1 = cycle.states.select(-1, 3);
  expect(std::abs(p1.min().item<double>() - 1.25) < 0.1 && std::abs(p1.max().item<double>() - 59.2) < 1.0, "repressilator limit cycle amplitude");
  const auto tog = [](const Series &s) { return gene::toggle_switch_rate(s, 150.0, 120.0, 2.5, 3.0); };
  const Series t = dyn::simulate(tog, vec({10, 1}), 0, 3, 3000);
  expect(close(at(t, 0), 139.227259, 1e-7) && close(at(t, 1), 0.0503422993, 1e-6), "toggle vs scipy");
  const auto gw = [](const Series &s) { return gene::goodwin_rate(s, 10.0, 0.5, 0.5, 12.0); };
  const Series g = dyn::simulate(gw, vec({0.5, 0.3, 1.2}), 0, 10, 2000);
  expect(close(at(g, 0), 1.90423233, 1e-7) && close(at(g, 1), 0.75354425, 1e-7) && close(at(g, 2), 1.02885279, 1e-7), "goodwin vs scipy");
  // bistability: two different starts reach different toggle states (batched)
  const Series two = mat({{10, 1}, {1, 10}});
  const Series end = dyn::simulate(tog, two, 0, 3, 3000);
  expect(at(end.select(0, 0), 0) > 100.0 && at(end.select(0, 1), 1) > 100.0, "toggle bistable lanes");
}

void test_mass_action_ssa() {
  // A <-> B, kf=1, kr=0.5; Binomial(10, pA(t)).
  const auto n = net::make(mat({{-1, 1}, {1, -1}}), mat({{1, 0}, {0, 1}}));
  const Series c = vec({1.0, 0.5});
  const auto prop = [&](const Series &x) { return net::propensity(n, x, c); };
  const std::int64_t B = 30000;
  sto::SsaOptions opt;
  opt.seed = 99;
  const Series x0 = pluss::series::zeros({B, 2}, F64).add(vec({10.0, 0.0}));
  const auto r = sto::gillespie(prop, n.stoichiometry, x0, 2.0, opt);
  expect(r.completed, "ssa completed");
  const double pa = 0.5 / 1.5 + 1.0 / 1.5 * std::exp(-1.5 * 2.0);
  const Series a = r.state.select(-1, 0);
  expect(std::abs(a.mean().item<double>() - 10 * pa) < 4 * std::sqrt(10 * pa * (1 - pa) / B), "SSA mean");
  const double var = a.sub(a.mean()).square().mean().item<double>();
  expect(std::abs(var - 10 * pa * (1 - pa)) < 0.1, "SSA variance");
  expect(r.state.sum(-1, false).sub(10.0).abs().max().item<double>() == 0.0, "SSA conserves A+B");
  // dimer propensity combinatorics: 2A -> D with c=1, A=5 -> C(5,2)=10
  const auto d = net::make(mat({{-2, 1}}), mat({{2, 0}}));
  expect(close(net::propensity(d, mat({{5.0, 0.0}}), vec({1.0})).item<double>(), 10.0, 1e-12), "dimer propensity");
}

void test_validation() {
  bool threw = false;
  try { (void)kin::michaelis_menten(vec({1.0}), 1.0, 0.0); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "Km<=0 rejected");
  threw = false;
  try { (void)net::make(mat({{-1, 1}}), mat({{-1, 0}})); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "negative order rejected");
}
} // namespace

int main() {
  test_kinetics();
  test_network_ode_and_jacobian();
  test_steady_state();
  test_gene_networks();
  test_mass_action_ssa();
  test_validation();
  std::cout << "NatSci systems checks passed\n";
}
