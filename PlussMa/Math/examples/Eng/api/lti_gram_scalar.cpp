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
  // dx/dt = -x + 2u, y = 3x. The Lyapunov equation A W + W A' + B B' = 0 gives W = b^2 / (2 |a|) = 2.
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({2.0}, 1, 1), mat({3.0}, 1, 1), mat({0.0}, 1, 1));
  out.writeln("reachability Gramian =", lti::gram(s, lti::Gram::Reach).flatten().items<double>(), " expected b^2 / (2|a|) = 2");
  out.writeln("observability Gramian =", lti::gram(s, lti::Gram::Observe).flatten().items<double>(), " expected c^2 / (2|a|) = 4.5");
}
