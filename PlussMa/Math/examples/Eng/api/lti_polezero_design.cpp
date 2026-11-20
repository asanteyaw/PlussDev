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
  // The filter designs return a PoleZero: the poles of a fourth-order Butterworth low-pass sit on the unit circle's interior.
  const lti::PoleZero design = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const auto poles = design.poles().items<std::complex<double>>();
  double largest = 0.0;
  for (const auto &p : poles) largest = std::max(largest, std::abs(p));
  out.writeln("poles", poles.size(), " zeros", design.zeros().numel(), " discrete:", design.is_disc(), " sample time", design.sample_time());
  out.writeln("largest pole magnitude =", largest, " stable:", design.is_stable());
}
