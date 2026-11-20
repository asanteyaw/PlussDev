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
  out.writeln("Gram::Reach has value", static_cast<int>(lti::Gram::Reach), "and Gram::Observe has value", static_cast<int>(lti::Gram::Observe));
  const lti::StateSpace s = lti::state(mat({-2.0}, 1, 1), mat({1.0}, 1, 1), mat({4.0}, 1, 1), mat({0.0}, 1, 1));
  out.writeln("with b = 1 and c = 4 the two differ by c^2 / b^2 = 16: reach", lti::gram(s, lti::Gram::Reach).flatten().items<double>()[0], " observe", lti::gram(s, lti::Gram::Observe).flatten().items<double>()[0]);
}
