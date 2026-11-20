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

// L(s) = k / (s (s + 1) (s + 2)).
static lti::PoleZero loop_gain(double k = 1.0) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(k)); }
// A logarithmic frequency grid from 0.01 to 100 rad/s.
static pluss::Series grid(int n = 2000) {
  std::vector<double> w;
  for (int i = 0; i < n; ++i) w.push_back(std::pow(10.0, -2.0 + 4.0 * i / (n - 1)));
  return pluss::make_series(w);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Two decoupled first-order loops with a coupling: L(s) = C (sI - A)^-1 B for a square 2 x 2 system.
  const lti::StateSpace plant = lti::state(mat({-1.0, 0.0, 0.0, -2.0}, 2, 2), mat({1.0, 0.5, 0.0, 1.0}, 2, 2), mat({1.0, 0.0, 0.0, 1.0}, 2, 2), mat({0.0, 0.0, 0.0, 0.0}, 2, 2));
  const pluss::Series omega = pluss::make_series({0.1, 1.0});
  const ctl::LoopResult r = ctl::loop(plant, omega);
  out.writeln("response shape", r.response.size(0), "x", r.response.size(1), "x", r.response.size(2), " (outputs by inputs by frequencies)");
  const auto s = r.sens.flatten().items<std::complex<double>>(), tt = r.comp.flatten().items<std::complex<double>>();
  double worst = 0.0;
  for (std::int64_t o = 0; o < 2; ++o)
    for (std::int64_t k = 0; k < 2; ++k)
      for (std::int64_t f = 0; f < 2; ++f) worst = std::max(worst, std::abs(s[(o * 2 + k) * 2 + f] + tt[(o * 2 + k) * 2 + f] - (o == k ? 1.0 : 0.0)));
  out.writeln("largest entry of S + T - I =", worst, "(the identity S + T = I holds as matrices)");
}
