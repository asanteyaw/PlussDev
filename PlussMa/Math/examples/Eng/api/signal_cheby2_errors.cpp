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
  refused(out, "zero attenuation", [] { sig::cheby2(3, 0.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "negative attenuation", [] { sig::cheby2(3, -40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "order zero", [] { sig::cheby2(0, 40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a cutoff at the Nyquist frequency", [] { sig::cheby2(3, 40.0, 500.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
}
