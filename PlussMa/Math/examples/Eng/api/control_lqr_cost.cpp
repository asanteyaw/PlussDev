#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // The optimal cost from x0 is x0' P x0. Simulate the closed loop and integrate the cost to check it.
  const lti::StateSpace system = lti::state(A2(), B2(), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::lqr(system, q, r);
  const lti::StateSpace closed = lti::state(sol.closed_loop, B2(), pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::zeros({2, 1}, pluss::ScalarType::Float64));
  const std::int64_t n = 8001;
  std::vector<double> t(n);
  for (std::int64_t i = 0; i < n; ++i) t[i] = 0.005 * i;
  const lti::SimRun run = lti::sim(closed, pluss::series::zeros({n, 1}, pluss::ScalarType::Float64), pluss::make_series(t), pluss::make_series({1.0, 0.0}));
  const pluss::Series x = run.states;
  const pluss::Series u = -x.matmul(sol.gain.transpose(0, 1));
  const pluss::Series stage = (x * x).sum(1) + (u * u).sum(1);
  const double cost = ((stage.slice(0, 0, n - 1) + stage.slice(0, 1, n)) * 0.5 * 0.005).sum().item<double>();
  out.writeln("x0 = (1, 0): cost integrated over 40 s =", cost, " x0' P x0 =", sol.value.flatten().items<double>()[0]);
}
