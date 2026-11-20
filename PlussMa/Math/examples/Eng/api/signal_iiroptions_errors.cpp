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
  refused(out, "a negative sample rate", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = -1000.0}); });
  refused(out, "a zero sample rate", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 0.0}); });
  refused(out, "a non-floating dtype", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0, .dtype = pluss::ScalarType::Int64}); });
  refused(out, "a float64 design on the MPS", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0, .platform = pluss::Platform::MPS}); });
}
