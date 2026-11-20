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
  // Two poles at -0.2 +- 1.99j (the damped oscillator of the other pages) and a zero at -1.
  const std::complex<double> p(-0.2, std::sqrt(4.0 - 0.04));
  const lti::PoleZero g = lti::zpk(pluss::make_series({-1.0}), pluss::make_series({p, std::conj(p)}), pluss::make_series(4.0));
  const auto poles = g.poles().items<std::complex<double>>();
  out.writeln("poles", poles[0], poles[1], " stable:", g.is_stable());
  out.writeln("DC gain =", lti::freqresp(g, pluss::make_series({0.0})).items<std::complex<double>>()[0], " expected 4 / |p|^2 =", 4.0 / std::norm(p));
}
