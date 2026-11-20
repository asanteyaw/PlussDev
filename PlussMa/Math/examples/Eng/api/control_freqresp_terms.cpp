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
  const pluss::Series omega = pluss::make_series({0.5, 2.0});
  const auto only_p = ctl::freqresp(ctl::pid(s(3.0), s(0.0), s(0.0), s(0.0)), omega).items<std::complex<double>>();
  const auto only_i = ctl::freqresp(ctl::pid(s(0.0), s(3.0), s(0.0), s(0.0)), omega).items<std::complex<double>>();
  const auto only_d = ctl::freqresp(ctl::pid(s(0.0), s(0.0), s(3.0), s(0.0)), omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < 2; ++i) {
    const double w = omega.items<double>()[i];
    out.writeln("omega", w, ": P =", only_p[i], " I =", only_i[i], " (3 / j omega =", std::complex<double>(0.0, -3.0 / w), ") D =", only_d[i]);
  }
}
