#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace sig = pluss::subsp::eng::signal;
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

// Gain in decibels of a design at a physical frequency in hertz.
static double gain_db(const lti::PoleZero &design, double hertz) {
  const pluss::Series omega = pluss::make_series({2.0 * std::acos(-1.0) * hertz});
  return 20.0 * std::log10(lti::freqresp(design, omega).abs().items<double>()[0]);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const lti::PoleZero inverse = sig::cheby2(4, 40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero butterworth = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  for (const double f : {50.0, 100.0, 150.0, 300.0}) out.writeln("at", f, "Hz: Chebyshev II", gain_db(inverse, f), "dB, Butterworth", gain_db(butterworth, f), "dB");
}
