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
  const lti::PoleZero design = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const pluss::Series sections = lti::to_sos(design);
  const double pi = std::acos(-1.0);
  double worst = 0.0;
  for (const double hertz : {10.0, 50.0, 100.0, 200.0, 400.0}) {
    const double w = 2.0 * pi * hertz / 1000.0;
    std::complex<double> product = 1.0;
    for (std::int64_t s = 0; s < sections.size(0); ++s) {
      const auto row = sections.select(0, s).items<double>();
      product *= sig::freqz(pluss::make_series({row[0], row[1], row[2]}), pluss::make_series({row[3], row[4], row[5]}), pluss::make_series({w})).items<std::complex<double>>()[0];
    }
    const std::complex<double> direct = lti::freqresp(design, pluss::make_series({2.0 * pi * hertz})).items<std::complex<double>>()[0];
    worst = std::max(worst, std::abs(product - direct));
  }
  out.writeln("largest difference between the cascade and the pole-zero response =", worst);
}
