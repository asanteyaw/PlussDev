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
  const lti::StateSpace siso = oscillator();
  const pluss::Series c1 = lti::ctrb(siso);
  out.writeln("two states, one input:", c1.size(0), "x", c1.size(1));
  const lti::StateSpace mimo = lti::state(mat({0.0, 1.0, 0.0, 0.0, 0.0, 1.0, -1.0, -2.0, -3.0}, 3, 3), mat({0.0, 1.0, 0.0, 0.0, 1.0, 1.0}, 3, 2), mat({1.0, 0.0, 0.0}, 1, 3), mat({0.0, 0.0}, 1, 2));
  const pluss::Series c2 = lti::ctrb(mimo);
  out.writeln("three states, two inputs:", c2.size(0), "x", c2.size(1), "(states by states times inputs)");
}
