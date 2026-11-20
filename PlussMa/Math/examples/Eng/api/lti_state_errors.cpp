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
  const pluss::Series a = mat({0.0, 1.0, -4.0, -0.4}, 2, 2), b = mat({0.0, 1.0}, 2, 1), c = mat({1.0, 0.0}, 1, 2), d = mat({0.0}, 1, 1);
  refused(out, "a non-square A", [&] { lti::state(mat({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, 2, 3), b, c, d); });
  refused(out, "B with the wrong number of rows", [&] { lti::state(a, mat({1.0, 2.0, 3.0}, 3, 1), c, d); });
  refused(out, "C with the wrong number of columns", [&] { lti::state(a, b, mat({1.0, 0.0, 0.0}, 1, 3), d); });
  refused(out, "D that does not match the outputs and inputs", [&] { lti::state(a, b, c, mat({0.0, 0.0}, 1, 2)); });
  refused(out, "a negative sample time", [&] { lti::state(a, b, c, d, -0.1); });
}
