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
  // G(s) = 2 / (s + 2): no zeros, one pole at -2 and gain 2.
  const lti::PoleZero g = lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({-2.0}), pluss::make_series(2.0));
  out.writeln("zeros", g.zeros().numel(), " poles", g.poles().items<double>(), " gain", g.gain().real().item<double>());
  const pluss::Series omega = pluss::make_series({0.0, 2.0});
  out.writeln("|G| at 0 and 2 rad/s =", lti::freqresp(g, omega).abs().items<double>(), " expected 1 and 1/sqrt(2) =", 1.0, 1.0 / std::sqrt(2.0));
}
