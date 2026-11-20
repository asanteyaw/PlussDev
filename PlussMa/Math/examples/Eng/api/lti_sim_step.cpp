#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
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
  // dx/dt = -x + u: the step response is 1 - exp(-t).
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  std::vector<double> t, u;
  for (int i = 0; i <= 10; ++i) { t.push_back(0.5 * i); u.push_back(1.0); }
  const lti::SimRun run = lti::sim(s, pluss::make_series(u, {11, 1}), pluss::make_series(t));
  const auto y = run.values.flatten().items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < y.size(); ++i) worst = std::max(worst, std::abs(y[i] - (1.0 - std::exp(-t[i]))));
  out.writeln("y(0) =", y.front(), " y(5) =", y.back(), " exact", 1.0 - std::exp(-5.0), " largest difference over all 11 samples =", worst);
}
