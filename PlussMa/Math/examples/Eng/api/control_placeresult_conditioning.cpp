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
  const struct { const char *label; double p1, p2; } cases[] = {{"well separated  ", -2.0, -5.0}, {"close together  ", -2.0, -2.01}, {"very close      ", -2.0, -2.0001}};
  for (const auto &c : cases) {
    const ctl::PlaceResult r = ctl::place(A2(), B2(), pluss::make_series({c.p1, c.p2}));
    out.writeln(c.label, "poles", c.p1, c.p2, ": condition of the basis", r.condition.max().item<double>(), " residual", r.residual.abs().max().item<double>());
  }
}
