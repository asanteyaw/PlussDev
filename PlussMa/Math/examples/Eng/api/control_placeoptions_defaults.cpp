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
  const ctl::PlaceOptions defaults;
  out.writeln("default sweeps", defaults.sweeps, " tolerance is 1e-10:", defaults.tolerance == 1e-10, " condition limit is 1e12:", defaults.condition_limit == 1e12);
  const ctl::PlaceResult r = ctl::place(A2(), B2(), pluss::make_series({-2.0, -3.0}));
  out.writeln("a single-input problem uses", r.sweeps, "sweeps (the basis is unique); residual below 1e-12:", r.residual.abs().max().item<double>() < 1e-12);
}
