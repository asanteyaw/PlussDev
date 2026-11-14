#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(sig::conv(pluss::make_series({1, 2, 3}),
                                   pluss::make_series({1.0, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("integer input rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::filter(
        pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}),
        pluss::make_series({0.0, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("zero denominator rejected =",
                message.substr(0, message.find('\n')));
  }
}
