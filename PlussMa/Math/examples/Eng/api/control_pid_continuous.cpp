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
  // C(s) = kp + ki / s + kd s / (filter s + 1) with kp = 2, ki = 1, kd = 0.5 and filter = 0.1.
  const ctl::Pid c = ctl::pid(pluss::make_series(2.0), pluss::make_series(1.0), pluss::make_series(0.5), pluss::make_series(0.1));
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0});
  const auto h = ctl::freqresp(c, omega).items<std::complex<double>>();
  double worst = 0.0;
  for (std::size_t i = 0; i < h.size(); ++i) {
    const double w = omega.items<double>()[i];
    const std::complex<double> s(0.0, w);
    const std::complex<double> exact = 2.0 + 1.0 / s + 0.5 * s / (0.1 * s + 1.0);
    worst = std::max(worst, std::abs(h[i] - exact));
    out.writeln("omega", w, ": |C| =", std::abs(h[i]), " phase (rad) =", std::arg(h[i]));
  }
  out.writeln("largest difference from the closed form =", worst);
}
