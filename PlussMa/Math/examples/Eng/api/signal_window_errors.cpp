#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(sig::hann(-1));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative size rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::hamming(
        4, {.dtype = pluss::ScalarType::Int64}));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("integer dtype rejected =",
                message.substr(0, message.find('\n')));
  }
}
