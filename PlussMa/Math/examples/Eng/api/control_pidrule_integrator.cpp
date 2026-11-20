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
  const auto s = [](double v) { return pluss::make_series(v); };
  for (const double w : {1.0, 10.0, 40.0}) {
    const pluss::Series one = pluss::make_series({w});
    out.writeln("omega", w, ": ideal magnitude", 1.0 / w, " phase", -std::acos(-1.0) / 2.0);
    for (const auto [label, rule] : {std::pair{"Forward  ", ctl::PidRule::Forward}, {"Backward ", ctl::PidRule::Backward}, {"Trapezoid", ctl::PidRule::Trapezoid}}) {
      const std::complex<double> g = ctl::freqresp(ctl::pid(s(0.0), s(1.0), s(0.0), s(0.0), 0.05, rule, ctl::PidRule::Backward), one).items<std::complex<double>>()[0];
      out.writeln("   ", label, ": magnitude", std::abs(g), " phase", std::arg(g));
    }
  }
}
