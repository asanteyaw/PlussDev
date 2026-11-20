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
  const lti::PoleZero design = sig::cheby1(4, 1.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  double highest = -1e9, lowest = 1e9;
  for (double f = 1.0; f <= 100.0; f += 1.0) {
    const double g = gain_db(design, f);
    highest = std::max(highest, g);
    lowest = std::min(lowest, g);
  }
  out.writeln("passband (1 to 100 Hz) spans", lowest, "to", highest, "dB for a ripple of 1 dB");
  out.writeln("gain at 200 Hz =", gain_db(design, 200.0), "dB");
}
