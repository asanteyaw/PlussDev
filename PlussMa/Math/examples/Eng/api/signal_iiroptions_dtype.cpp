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
  const lti::PoleZero wide = sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero narrow = sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0, .dtype = pluss::ScalarType::Float32});
  out.writeln("poles dtype", wide.poles().scalar_type(), "against", narrow.poles().scalar_type());
  out.writeln("gain at the cutoff", gain_db(wide, 100.0), "dB against", gain_db(narrow, 100.0), "dB");
}
