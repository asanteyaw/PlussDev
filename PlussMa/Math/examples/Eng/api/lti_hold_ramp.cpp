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
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});
  const pluss::Series ramp = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0}, {5, 1});
  // For u = t the exact response from rest is t - 1 + exp(-t).
  for (const auto [label, hold] : {std::pair{"Zero  ", lti::Hold::Zero}, {"Linear", lti::Hold::Linear}, {"Auto  ", lti::Hold::Auto}}) {
    const auto y = lti::sim(s, ramp, t, {.hold = hold}).values.flatten().items<double>();
    double worst = 0.0;
    for (std::size_t i = 0; i < y.size(); ++i) worst = std::max(worst, std::abs(y[i] - (t.items<double>()[i] - 1.0 + std::exp(-t.items<double>()[i]))));
    out.writeln(label, "y(2) =", y.back(), " largest error against the exact ramp response =", worst);
  }
}
