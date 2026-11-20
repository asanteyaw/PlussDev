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
  // x[k+1] = x[k] + u[k]: the state climbs by a known 0.5 per step and the sensor sees it with a deterministic disturbance.
  const lti::StateSpace system = lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0);
  std::vector<double> y, truth;
  for (int k = 0; k < 8; ++k) { truth.push_back(0.5 * k); y.push_back(0.5 * k + 0.3 * std::sin(7.0 * k)); }
  const pluss::Series u = mat(std::vector<double>(8, 0.5), 8, 1);
  const ctl::KalmanResult with_input = ctl::kalman(system, mat(y, 8, 1), u, mat({0.01}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({1.0}, 1, 1));
  const ctl::KalmanResult without = ctl::kalman(system, mat(y, 8, 1), mat({0.01}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({1.0}, 1, 1));
  double e_with = 0.0, e_without = 0.0;
  const auto a = with_input.filtered_mean.flatten().items<double>(), b = without.filtered_mean.flatten().items<double>();
  for (std::size_t k = 0; k < 8; ++k) { e_with += std::abs(a[k] - truth[k]); e_without += std::abs(b[k] - truth[k]); }
  out.writeln("mean absolute error of the filtered mean: with the input", e_with / 8.0, " without it", e_without / 8.0);
}
