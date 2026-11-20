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
  // Three controllers that differ in their proportional gain, evaluated at two frequencies.
  const ctl::Pid c = ctl::pid(pluss::make_series({1.0, 2.0, 4.0}), pluss::make_series(0.0), pluss::make_series(0.0), pluss::make_series(0.0));
  const pluss::Series h = ctl::freqresp(c, pluss::make_series({1.0, 10.0}));
  out.writeln("response shape", h.size(0), "x", h.size(1), " (controllers by frequencies)");
  out.writeln("proportional-only gains =", h.real().select(1, 0).items<double>());
}
