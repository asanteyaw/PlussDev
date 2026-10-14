#include "pluss/subsp/natsci/common/dynamics.h"
#include "pluss/subsp/natsci/common/stiff.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace dyn = pluss::subsp::natsci::common::dynamics;
namespace stiff = pluss::subsp::natsci::common::stiff;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }
bool close(double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1e-12, std::abs(b)); }

void test_linear_stiff() {
  // y' = -lambda y, lambda = 1e6: an explicit method needs h < 2e-6; the
  // Rosenbrock pair is L-stable-like and must reach t = 1 in few steps.
  const auto f = [](const Series &y) { return y.mul(-1e6); };
  stiff::Options o;
  o.rtol = 1e-6;
  o.atol = 1e-12;
  const auto r = stiff::integrate(f, vec({1.0, 2.0}), 0.0, 1.0, o);
  expect(r.converged, "linear converged");
  expect(r.y.abs().max().item<double>() < 1e-9, "stiff decay reaches zero");
  // explicit RK4 needs h < 2.8e-6 for stability, i.e. > 3.5e5 steps; the implicit
  // pair resolves the transient and then strides across the rest of [0, 1].
  expect(r.steps < 2000, "stiff solver is not stability-limited");
  // moderate mixed-rate system with a known solution y = exp(-2 t)
  const auto g = [](const Series &y) { return y.mul(-2.0); };
  // second-order pair: global error is ~1e-5 at rtol 1e-6, ~1e-7 at rtol 1e-10
  o.rtol = 1e-10;
  o.atol = 1e-14;
  const auto m = stiff::integrate(g, vec({1.0}), 0.0, 1.0, o);
  expect(close(at(m.y, 0), std::exp(-2.0), 1e-6), "exp(-2t) accuracy");
}

void test_robertson() {
  const auto f = [](const Series &y) {
    const Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
    const Series r1 = a.mul(0.04), r2 = b.mul(c).mul(1e4), r3 = b.square().mul(3e7);
    return r1.neg().add(r2).unsqueeze(-1).unite(
        std::vector<Series>{r1.sub(r2).sub(r3).unsqueeze(-1), r3.unsqueeze(-1)}, -1);
  };
  stiff::Options o;
  o.rtol = 1e-8;
  o.atol = 1e-14;
  const auto r = stiff::integrate(f, vec({1.0, 0.0, 0.0}), 0.0, 40.0, o);
  expect(r.converged, "Robertson converged");
  expect(close(at(r.y, 0), 7.158270687199e-01, 1e-5) && close(at(r.y, 1), 9.185534764578e-06, 1e-4) &&
             close(at(r.y, 2), 2.841637457453e-01, 1e-5), "Robertson vs scipy Radau");
  expect(std::abs(r.y.sum().item<double>() - 1.0) < 1e-10, "Robertson conserves mass");
  // analytic-Jacobian path gives the same answer
  const auto jac = [](const Series &y) {
    const Series b = y.select(-1, 1), c = y.select(-1, 2);
    const Series z = b.mul(0.0);
    const auto row = [&](const Series &p, const Series &q, const Series &w) {
      return p.unsqueeze(-1).unite(std::vector<Series>{q.unsqueeze(-1), w.unsqueeze(-1)}, -1).unsqueeze(-2);
    };
    const Series r0 = row(z.add(-0.04), c.mul(1e4), b.mul(1e4));
    const Series r1 = row(z.add(0.04), c.mul(-1e4).sub(b.mul(6e7)), b.mul(-1e4));
    const Series r2 = row(z, b.mul(6e7), z);
    return r0.unite(std::vector<Series>{r1, r2}, -2);
  };
  const auto ra = stiff::integrate(f, vec({1.0, 0.0, 0.0}), 0.0, 40.0, o, jac);
  expect(close(at(ra.y, 0), at(r.y, 0), 1e-6), "analytic vs finite-difference Jacobian");
}

void test_van_der_pol_and_lanes() {
  const double mu = 100.0;
  const auto f = [mu](const Series &y) {
    const Series x = y.select(-1, 0), v = y.select(-1, 1);
    const Series dv = v.mul(x.square().neg().add(1.0)).mul(mu).sub(x);
    return v.unsqueeze(-1).unite(std::vector<Series>{dv.unsqueeze(-1)}, -1);
  };
  stiff::Options o;
  o.rtol = 1e-8;
  o.atol = 1e-12;
  const Series lanes = pluss::series::make({{2.0, 0.0}, {2.0, 0.0}}).to(F64);
  const auto r = stiff::integrate(f, lanes, 0.0, 50.0, o);
  expect(r.converged, "VdP converged");
  expect(close(r.y.select(0, 0).select(-1, 0).item<double>(), r.y.select(0, 1).select(-1, 0).item<double>(), 1e-12), "identical lanes agree");
  expect(std::abs(r.y.select(0, 0).select(-1, 0).item<double>() - 1.596824040977) < 2e-3 &&
             std::abs(r.y.select(0, 0).select(-1, 1).item<double>() + 0.01030212556454) < 2e-3,
         "Van der Pol (mu=100) vs scipy Radau at t=50");
  // output times
  const auto frames = stiff::integrate_at(f, vec({2.0, 0.0}), {0.0, 10.0, 50.0}, o);
  expect(frames.size(0) == 3 && close(frames.select(0, 2).select(-1, 0).item<double>(), at(r.y.select(0, 0), 0), 1e-6), "integrate_at output times");
}

void test_validation() {
  bool threw = false;
  try { (void)stiff::integrate([](const Series &y) { return y; }, vec({1.0}), 1.0, 0.0); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "t1 < t0 rejected");
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
  run("linear_stiff", test_linear_stiff);
  run("robertson", test_robertson);
  run("van_der_pol_and_lanes", test_van_der_pol_and_lanes);
  run("validation", test_validation);
  std::cout << "NatSci stiff checks passed\n";
}
