#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  const sig::IirOptions options{
      .band = sig::Band::Low,
      .sample_rate = 1000.0,
  };
  const lti::PoleZero butter = sig::butter(4, 100.0, options);
  const lti::PoleZero cheby1 = sig::cheby1(4, 1.0, 100.0, options);
  const lti::PoleZero cheby2 = sig::cheby2(4, 40.0, 100.0, options);

  out.writeln("Butterworth poles =", butter.poles().numel());
  out.writeln("Butterworth stable =", butter.is_stable());
  out.writeln("Chebyshev I stable =", cheby1.is_stable());
  out.writeln("Chebyshev II stable =", cheby2.is_stable());
  out.writeln("SOS shape =", lti::to_sos(butter).sizes());
}
