#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const sig::IirOptions low{
      .band = sig::Band::Low,
      .sample_rate = 1000.0,
  };
  try {
    static_cast<void>(sig::butter(3, 500.0, low));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("Nyquist cutoff rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::cheby1(3, 0.0, 100.0, low));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("zero ripple rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::butter(
        3, 100.0,
        {.band = sig::Band::Pass, .sample_rate = 1000.0}));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("scalar passband rejected =",
                message.substr(0, message.find('\n')));
  }
}
