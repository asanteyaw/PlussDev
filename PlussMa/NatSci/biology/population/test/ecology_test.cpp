#include "pluss/ma/natsci/biology.h"
#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/stability.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace eco = bio::population::ecology;
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
Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }

void test_effort_harvesting() {
  const double r = 0.8, k = 100.0, q = 0.02;
  const Series like = vec({0.0});
  // equilibrium n* = K (1 - qE/r) is a zero of the rate
  const Series effort = vec({5.0, 10.0, 20.0, 30.0});
  const Series nstar = eco::effort_equilibrium(r, k, q, effort, effort);
  expect(eco::effort_harvest_rate(nstar, r, k, q, effort).abs().max().item<double>() < 1e-12, "harvest equilibrium is a zero of the rate");
  // MSY: the effort maximising the yield is where d(yield)/dE = 0, found with the shared Newton solver
  const auto yield = [&](const Series &e) { return eco::effort_yield(r, k, q, e, e); };
  const Series seed = vec({10.0});
  const auto msy_effort = roots::newton([&](const Series &e) { // central difference of the yield
    const Series h = e.mul(1e-5);
    return yield(e.add(h)).sub(yield(e.sub(h))).div(h.mul(2.0));
  }, seed);
  expect(std::abs(msy_effort.root.item<double>() - r / (2 * q)) < 1e-4, "effort at maximum sustainable yield is r/(2q)");
  expect(std::abs(yield(vec({r / (2 * q)})).item<double>() - eco::maximum_sustainable_yield(r, k, like).item<double>()) < 1e-10, "MSY = rK/4");
  // overfishing (qE > r) drives the stock to extinction: equilibrium formula clamps below zero
  expect(eco::effort_equilibrium(r, k, q, vec({50.0}), vec({0.0})).item<double>() < 0.0, "effort beyond r/q has no positive equilibrium");
}

void test_constant_yield_saddle_node() {
  const double r = 1.0, k = 1.0; // rK/4 = 0.25
  const Series h = vec({0.0, 0.1, 0.2, 0.24});
  const auto eq = eco::constant_yield_equilibria(r, k, h, h);
  expect(eq.exists.min().item<double>() == 1.0, "equilibria exist below rK/4");
  const auto rate = [&](const Series &n) { return eco::constant_yield_rate(n, r, k, h); };
  expect(rate(eq.stable).abs().max().item<double>() < 1e-12 && rate(eq.unstable).abs().max().item<double>() < 1e-12, "both branches are equilibria");
  // linear stability by the shared Routh-Hurwitz count: larger branch stable, smaller one a saddle
  // inside the Jacobian the state carries a perturbation axis, so per-lane coefficients are [lanes, 1, 1]
  const Series h_lanes = h.unsqueeze(-1).unsqueeze(-1);
  const auto rate_j = [&](const Series &n) { return eco::constant_yield_rate(n, r, k, h_lanes); };
  const auto s_stable = stab::local_stability(rate_j, eq.stable.unsqueeze(-1), 1e-6);
  const auto s_saddle = stab::local_stability(rate_j, eq.unstable.unsqueeze(-1).clamp(0.05, 1.0), 1e-6);
  expect(s_stable.unstable.max().item<double>() == 0.0, "larger harvest equilibrium is stable");
  expect(s_saddle.unstable.select(0, 3).item<double>() == 1.0, "smaller harvest equilibrium is a saddle");
  // saddle-node: for h > rK/4 the population collapses from the carrying capacity; below it persists.
  // Located by lane-wise bisection on the long-time outcome.
  const auto survives = [&](const Series &hh) {
    const auto f = [&](const Series &n) { return eco::constant_yield_rate(n, r, k, hh); };
    const Series end = dyn::simulate(f, hh.mul(0.0).add(k), 0.0, 200.0, 2000);
    return end.gt(0.05).to(F64).mul(2.0).sub(1.0).neg(); // -1 survives, +1 collapses
  };
  const auto thr = roots::bisect(survives, vec({0.2}), vec({0.3}), {14});
  expect(std::abs(thr.root.item<double>() - r * k / 4.0) < 5e-3, "collapse threshold equals rK/4 (found " + std::to_string(thr.root.item<double>()) + ")");
  expect(eco::constant_yield_equilibria(r, k, vec({0.3}), vec({0.3})).exists.item<double>() == 0.0, "no equilibrium above rK/4");
}

void test_levins() {
  const Series p = vec({0.1, 0.5, 0.9});
  const double c = 0.5, e = 0.2;
  const Series pstar = eco::levins_equilibrium(c, e, p);
  expect(std::abs(at(pstar, 0) - (1 - e / c)) < 1e-14, "Levins equilibrium 1 - e/c");
  expect(eco::levins_rate(pstar, c, e).abs().max().item<double>() < 1e-14, "Levins equilibrium is a zero of the rate");
  // convergence from any occupancy
  const auto end = dyn::simulate([&](const Series &x) { return eco::levins_rate(x, c, e); }, p, 0.0, 60.0, 600);
  expect(end.sub(1 - e / c).abs().max().item<double>() < 1e-6, "all starting occupancies converge to 1 - e/c");
  // extinction exceeds colonisation: occupancy -> 0 (threshold c = e)
  const auto gone = dyn::simulate([&](const Series &x) { return eco::levins_rate(x, 0.2, 0.3); }, p, 0.0, 200.0, 2000);
  expect(gone.max().item<double>() < 1e-6, "no persistence when e > c");
  // rescue effect: dp = (c - e) p (1 - p) -> p* = 1 whenever c > e
  const auto rescued = dyn::simulate([&](const Series &x) { return eco::levins_rate(x, 0.5, 0.2, true); }, p, 0.0, 120.0, 1200);
  expect(rescued.sub(1.0).abs().max().item<double>() < 1e-4, "rescue effect lifts the equilibrium to full occupancy");
}

void test_patch_network() {
  // complete graph on 4 patches with connectivity a: dominant eigenvalue 3a (exact)
  const double a = 0.4;
  const Series cmat = mat({{0, a, a, a}, {a, 0, a, a}, {a, a, 0, a}, {a, a, a, 0}});
  expect(std::abs(eco::metapopulation_capacity(cmat).item<double>() - 3 * a) < 1e-9, "metapopulation capacity of the complete graph is 3a");
  // rank-one connectivity u v^T: dominant eigenvalue v.u
  const Series u = vec({1.0, 2.0, 3.0}), v = vec({0.5, 0.25, 1.0});
  const Series c1 = u.unsqueeze(-1).mul(v.unsqueeze(-2));
  expect(std::abs(eco::metapopulation_capacity(c1).item<double>() - u.mul(v).sum().item<double>()) < 1e-9, "metapopulation capacity of a rank-one network");
  // persistence threshold: the empty state loses stability exactly at e = lambda_M. Found by lane-wise
  // bisection on the number of unstable modes of the Jacobian C - e I (shared Routh-Hurwitz count).
  const auto unstable_at = [&](const Series &e) { // e: [lanes]
    const Series ee = e.unsqueeze(-1).unsqueeze(-1);
    const Series jac = cmat.unsqueeze(0).sub(pluss::series::eye(4, F64).mul(ee));
    return stab::unstable_modes(jac).unstable;
  };
  const auto thr = roots::bisect([&](const Series &e) { return unstable_at(e).neg().add(0.5); }, vec({0.5}), vec({3.0}), {50});
  expect(std::abs(thr.root.item<double>() - 3 * a) < 1e-6, "extinction threshold equals the metapopulation capacity");
  // dynamics: above threshold (e < 3a) the network persists, below it (e > 3a) it dies out
  const Series p0 = vec({0.2, 0.2, 0.2, 0.2});
  const auto persist = dyn::simulate([&](const Series &x) { return eco::patch_occupancy_rate(x, cmat, 0.5); }, p0, 0.0, 300.0, 3000);
  const auto die = dyn::simulate([&](const Series &x) { return eco::patch_occupancy_rate(x, cmat, 2.0); }, p0, 0.0, 300.0, 3000);
  // symmetric fixed point: (3a p)(1 - p) = e p  ->  p = 1 - e/(3a)
  expect(persist.sub(1 - 0.5 / (3 * a)).abs().max().item<double>() < 1e-6, "persisting network reaches p = 1 - e/(3a)");
  expect(die.max().item<double>() < 1e-6, "network below the persistence threshold goes extinct");
}

void test_migration() {
  // total abundance is conserved; symmetric migration equalises, asymmetric rates give n1 : n2 = m21 : m12
  const Series sym = mat({{0.0, 0.3}, {0.3, 0.0}});
  const Series n0 = vec({10.0, 2.0});
  const auto eq = dyn::simulate([&](const Series &x) { return eco::migration_rate(x, sym); }, n0, 0.0, 40.0, 400);
  expect(std::abs(eq.sum().item<double>() - 12.0) < 1e-10, "migration conserves the total");
  expect(std::abs(at(eq, 0) - 6.0) < 1e-6 && std::abs(at(eq, 1) - 6.0) < 1e-6, "symmetric migration equalises densities");
  const Series asym = mat({{0.0, 0.5}, {0.1, 0.0}}); // 0 -> 1 at 0.5, 1 -> 0 at 0.1
  const auto eq2 = dyn::simulate([&](const Series &x) { return eco::migration_rate(x, asym); }, n0, 0.0, 80.0, 800);
  expect(std::abs(at(eq2, 0) / at(eq2, 1) - 0.1 / 0.5) < 1e-6 && std::abs(eq2.sum().item<double>() - 12.0) < 1e-10, "asymmetric migration: n0/n1 = m10/m01");
  // batched lanes with different migration-rate matrices
  const Series lanes = mat({{10.0, 2.0}, {10.0, 2.0}});
  expect(eco::migration_rate(lanes, sym).sum(-1, false).abs().max().item<double>() < 1e-14, "zero net flux summed over patches in every lane");
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
  run("effort_harvesting", test_effort_harvesting);
  run("constant_yield_saddle_node", test_constant_yield_saddle_node);
  run("levins", test_levins);
  run("patch_network", test_patch_network);
  run("migration", test_migration);
  std::cout << "NatSci ecology checks passed\n";
}
