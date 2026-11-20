#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto det2 = [](const pluss::Series &m) { const auto v = m.flatten().items<double>(); return v[0] * v[3] - v[1] * v[2]; };
  const lti::StateSpace observable = oscillator();
  out.writeln("observability matrix [C; CA] =", lti::obsv(observable).flatten().items<double>(), " determinant", det2(lti::obsv(observable)));
  // Only the first state is measured and it does not depend on the second: the second is invisible.
  const lti::StateSpace hidden = lti::state(mat({-1.0, 0.0, 0.0, -2.0}, 2, 2), mat({1.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  out.writeln("invisible state: [C; CA] =", lti::obsv(hidden).flatten().items<double>(), " determinant", det2(lti::obsv(hidden)), "(zero: not observable)");
}
