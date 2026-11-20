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
  const std::pair<const char *, sig::Band> bands[] = {{"Pass", sig::Band::Pass}, {"Stop", sig::Band::Stop}};
  for (const auto &[label, band] : bands) {
    const lti::PoleZero design = sig::butter(3, {100.0, 200.0}, {.band = band, .sample_rate = 1000.0});
    out.writeln(label, ": poles", design.poles().numel(), " (a third-order prototype doubles); gain at 50 Hz", gain_db(design, 50.0), "dB, at 150 Hz", gain_db(design, 150.0), "dB, at 400 Hz", gain_db(design, 400.0), "dB");
  }
}
