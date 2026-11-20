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
  const lti::StateSpace controllable = oscillator();
  out.writeln("controllability matrix [B, AB] =", lti::ctrb(controllable).flatten().items<double>(), " determinant", det2(lti::ctrb(controllable)));
  // The second state is driven by nothing and does not feed the first, so no input can reach it.
  const lti::StateSpace hidden = lti::state(mat({-1.0, 0.0, 0.0, -2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), mat({1.0, 1.0}, 1, 2), mat({0.0}, 1, 1));
  out.writeln("decoupled state: [B, AB] =", lti::ctrb(hidden).flatten().items<double>(), " determinant", det2(lti::ctrb(hidden)), "(zero: not controllable)");
}
