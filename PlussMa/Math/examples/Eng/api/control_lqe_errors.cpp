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

// A scalar random walk x[k+1] = x[k] + w, y[k] = x[k] + v, sampled every second.
static lti::StateSpace walk() { return lti::state(mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// Eight observations of a constant 1 with a deterministic disturbance in place of noise.
static std::vector<double> readings(int n = 8) {
  std::vector<double> y;
  for (int k = 0; k < n; ++k) y.push_back(1.0 + 0.3 * std::sin(7.0 * k));
  return y;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace s = lti::state(mat({0.0, 1.0, 0.0, 0.0}, 2, 2), mat({0.0, 0.0}, 2, 1), mat({0.0, 1.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  refused(out, "a noise covariance that is not positive definite", [&] { ctl::lqe(s, q, mat({-1.0}, 1, 1)); });
  refused(out, "a process covariance of the wrong size", [&] { ctl::lqe(s, mat({1.0}, 1, 1), mat({1.0}, 1, 1)); });
  const lti::StateSpace blind = lti::state(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({0.0, 0.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  refused(out, "an unstable mode that the sensor cannot see", [&] { ctl::lqe(blind, q, mat({1.0}, 1, 1)); });
}
