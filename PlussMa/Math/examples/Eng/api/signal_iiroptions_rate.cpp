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
  const lti::PoleZero digital = sig::butter(2, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero analog = sig::butter(2, 100.0, {.band = sig::Band::Low, .sample_rate = std::nullopt});
  out.writeln("with a sample rate: discrete =", digital.is_disc(), " sample time =", digital.sample_time(), " stable =", digital.is_stable());
  out.writeln("without one: continuous =", analog.is_cont(), " stable =", analog.is_stable());
  const sig::IirOptions defaults;
  out.writeln("the default sample rate is", *defaults.sample_rate, "(a cutoff of 0.5 is then half the Nyquist frequency)");
}
