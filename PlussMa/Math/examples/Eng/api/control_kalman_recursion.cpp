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
  // The scalar Kalman recursion written out by hand: predict p + q, gain k = p / (p + r), update.
  const auto y = readings();
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto mean = r.filtered_mean.flatten().items<double>(), cov = r.filtered_cov.flatten().items<double>();
  double x = 0.0, p = 10.0, worst_mean = 0.0, worst_cov = 0.0;
  for (std::size_t k = 0; k < y.size(); ++k) {
    const double predicted = p + 0.1, gain = predicted / (predicted + 1.0);
    x += gain * (y[k] - x);
    p = (1.0 - gain) * predicted;
    worst_mean = std::max(worst_mean, std::abs(x - mean[k]));
    worst_cov = std::max(worst_cov, std::abs(p - cov[k]));
  }
  out.writeln("filtered mean =", mean);
  out.writeln("largest difference from the hand recursion: mean", worst_mean, " covariance", worst_cov);
}
