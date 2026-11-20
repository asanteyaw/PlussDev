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
  const lti::SimOptions defaults;
  out.writeln("the default hold is Auto:", defaults.hold == lti::Hold::Auto);
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});
  const pluss::Series ramp = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0}, {5, 1});
  const auto automatic = lti::sim(s, ramp, t).values.flatten().items<double>();
  const auto linear = lti::sim(s, ramp, t, {.hold = lti::Hold::Linear}).values.flatten().items<double>();
  const auto zero = lti::sim(s, ramp, t, {.hold = lti::Hold::Zero}).values.flatten().items<double>();
  double to_linear = 0.0, to_zero = 0.0;
  for (std::size_t i = 0; i < automatic.size(); ++i) { to_linear = std::max(to_linear, std::abs(automatic[i] - linear[i])); to_zero = std::max(to_zero, std::abs(automatic[i] - zero[i])); }
  out.writeln("Auto against Linear, largest difference", to_linear, "; Auto against Zero", to_zero);
}
