#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const actuarial::Operator<double(double)> empty;

  out.writeln("defined =", empty.defined());
  try {
    static_cast<void>(empty(10.0));
  } catch (const std::exception &error) {
    out.writeln("rejected =", error.what());
  }
}
