#include <pluss/tils/kit/scalar_cast_compat.h>
#include "pluss/subsp/math/roots.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>


namespace roots = pluss::subsp::math::roots;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }

void test_sqrt_lanes() {
  // roots of x^2 - c over many lanes at once
  const Series c = vec({2.0, 3.0, 5.0, 10.0, 0.25, 100.0});
  const roots::Fn f = [&](const Series &x) { return x.square().sub(c); };
  const Series lo = c.mul(0.0).add(0.0), hi = c.add(1.0); // sqrt(c) < c + 1 for c > 0
  const auto b = roots::bisect(f, lo, hi);
  expect(b.bracketed && as_f64(b.root.sub(c.sqrt()).abs().max()) < 1e-13, "bisect");
  const auto il = roots::illinois(f, lo, hi);
  expect(as_f64(il.root.sub(c.sqrt()).abs().max()) < 1e-12, "illinois");
  const auto nw = roots::newton(f, c.add(1.0), [&](const Series &x) { return x.mul(2.0); });
  expect(as_f64(nw.root.sub(c.sqrt()).abs().max()) < 1e-13, "newton with analytic derivative");
  const auto nf = roots::newton(f, c.add(1.0));
  expect(as_f64(nf.root.sub(c.sqrt()).abs().max()) < 1e-8, "newton with central-difference derivative");
  const auto sc = roots::secant(f, c.add(1.0), c.add(2.0));
  expect(as_f64(sc.root.sub(c.sqrt()).abs().max()) < 1e-11, "secant");

  const auto hh = roots::householder(
      [&](const Series &x) {
        return roots::Derivatives{x.square().sub(c), x.mul(2.0),
                                  pluss::series::full_like(x, 2.0),
                                  pluss::series::zeros_like(x)};
      },
      c.add(1.0), {12, 1e-13, 1e-13, 1});
  expect(as_f64(hh.root.sub(c.sqrt()).abs().max()) < 1e-13 &&
             as_f64(hh.residual.abs().max()) < 1e-12,
         "householder with shared derivatives");
}

void test_transcendental() {
  // Dottie number: cos x = x
  const roots::Fn f = [](const Series &x) { return x.cos().sub(x); };
  const Series lo = vec({0.0, 0.5}), hi = vec({1.0, 1.0});
  const double dottie = 0.7390851332151607;
  expect(std::abs(as_f64(roots::bisect(f, lo, hi).root.select(0, 0)) - dottie) < 1e-13, "bisect Dottie number");
  expect(std::abs(as_f64(roots::illinois(f, lo, hi).root.select(0, 1)) - dottie) < 1e-13, "illinois Dottie number");
  // a cubic with a different parameter per lane: x^3 - x - k, root near 2 for k = 6
  const Series k = vec({6.0, 24.0, 60.0});
  const roots::Fn g = [&](const Series &x) { return x.pow(3.0).sub(x).sub(k); };
  const auto r = roots::illinois(g, k.mul(0.0).add(1.0), k.mul(0.0).add(10.0), {200, 0.0, 1e-14, 8});
  expect(as_f64(r.residual.abs().max()) < 1e-12, "illinois on a cubic family");
  expect(std::abs(as_f64(r.root.select(0, 0)) - 2.0) < 1e-10 && std::abs(as_f64(r.root.select(0, 1)) - 3.0) < 1e-10, "cubic roots (k = 6 -> 2, k = 24 -> 3)");
}

void test_rates_and_tolerances() {
  const roots::Fn f = [](const Series &x) { return x.exp().sub(5.0); }; // root ln 5
  const Series lo = vec({0.0}), hi = vec({3.0});
  // iterations needed to reach 1e-12 in f: regula falsi (Illinois) beats bisection
  const auto b = roots::bisect(f, lo, hi, {200, 0.0, 1e-12, 1});
  const auto il = roots::illinois(f, lo, hi, {200, 0.0, 1e-12, 1});
  expect(as_f64(b.residual.abs().max()) <= 1e-12 + 1e-9 && as_f64(il.residual.abs().max()) <= 1e-12, "tolerance early-exit");
  expect(il.iterations < b.iterations, "Illinois needs fewer iterations than bisection");
  // Newton is quadratic: a handful of iterations
  const auto nw = roots::newton(f, vec({1.0}), [](const Series &x) { return x.exp(); }, {50, 0.0, 1e-13, 1});
  expect(nw.iterations <= 8 && std::abs(as_f64(nw.root) - std::log(5.0)) < 1e-12, "Newton converges quadratically");
  // bracket without a sign change is reported
  const auto bad = roots::bisect([](const Series &x) { return x.square().add(1.0); }, vec({-1.0}), vec({1.0}));
  expect(!bad.bracketed, "missing sign change is flagged");
  bool threw = false;
  try { (void)roots::bisect([](const Series &x) { return x; }, vec({0.0, 1.0}), vec({1.0})); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "shape mismatch rejected");
}

void test_many_lanes() {
  // 200000 independent problems in one call
  const std::int64_t n = 200000;
  const Series c = pluss::series::linspace(1.0, 50.0, n, F64);
  const roots::Fn f = [&](const Series &x) { return x.square().sub(c); };
  const auto t0 = std::chrono::steady_clock::now();
  const auto r = roots::illinois(f, c.mul(0.0), c.add(1.0), {100, 0.0, 1e-13, 8});
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  expect(as_f64(r.root.sub(c.sqrt()).abs().max()) < 1e-11, "200000 lanes solved together");
  expect(s < 5.0, "batched solve of 200000 lanes is fast (" + std::to_string(s) + " s)");
}

void test_householder_graph() {
  Series parameter = pluss::series::make(2.0);
  parameter.tracks_slope_(true);
  const auto result = roots::householder(
      [parameter](const Series &x) {
        return roots::Derivatives{x.square().sub(parameter), x.mul(2.0),
                                  pluss::series::full_like(x, 2.0),
                                  pluss::series::zeros_like(x)};
      },
      pluss::series::make(1.0), {12, 1e-13, 1e-13, 1});
  result.root.reverse();
  expect(parameter.has_sensitivity() &&
             std::abs(as_f64(parameter.sensitivity()) -
                      0.5 / std::sqrt(2.0)) < 1e-11,
         "householder preserves reverse history");
}
} // namespace

int main() {
  const auto run = [](const char *name, void (*fn)()) {
    try { fn(); } catch (const std::exception &e) { std::cerr << name << ": " << e.what() << "\n"; std::exit(1); }
  };
  run("sqrt_lanes", test_sqrt_lanes);
  run("transcendental", test_transcendental);
  run("rates_and_tolerances", test_rates_and_tolerances);
  run("many_lanes", test_many_lanes);
  run("householder_graph", test_householder_graph);
  std::cout << "math roots checks passed\n";
}
