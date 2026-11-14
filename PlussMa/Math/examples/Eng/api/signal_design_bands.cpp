#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <optional>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::PoleZero pass = sig::butter(
      3, {100.0, 200.0},
      {.band = sig::Band::Pass, .sample_rate = 1000.0});
  const lti::PoleZero stop = sig::butter(
      3, {100.0, 200.0},
      {.band = sig::Band::Stop, .sample_rate = 1000.0});
  const lti::PoleZero analog = sig::butter(
      3, 2.0,
      {.band = sig::Band::Low, .sample_rate = std::nullopt});
  const pluss::Series omega =
      pluss::make_series({0.0, 150.0 * 2.0 * std::acos(-1.0)});

  out.writeln("pass magnitude =", lti::freqresp(pass, omega).abs().items<double>());
  out.writeln("stop magnitude =", lti::freqresp(stop, omega).abs().items<double>());
  out.writeln("analog continuous =", analog.is_cont());
  out.writeln("analog gain =", analog.gain().real().item<double>());
}
