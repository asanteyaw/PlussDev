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
  // Backward-Euler integrator and derivative at 100 Hz against the continuous controller.
  const auto scalar = [](double v) { return pluss::make_series(v); };
  const ctl::Pid continuous = ctl::pid(scalar(2.0), scalar(1.0), scalar(0.5), scalar(0.1));
  const ctl::Pid discrete = ctl::pid(scalar(2.0), scalar(1.0), scalar(0.5), scalar(0.1), 0.01, ctl::PidRule::Backward, ctl::PidRule::Backward);
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0, 50.0});
  const auto a = ctl::freqresp(continuous, omega).items<std::complex<double>>();
  const auto b = ctl::freqresp(discrete, omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < a.size(); ++i) out.writeln("omega", omega.items<double>()[i], ": |C| continuous", std::abs(a[i]), " discrete", std::abs(b[i]));
  out.writeln("sample time", discrete.sample_time(), " discrete:", discrete.is_disc());
}
