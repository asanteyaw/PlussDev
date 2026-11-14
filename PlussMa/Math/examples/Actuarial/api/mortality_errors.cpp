#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(
        actuarial::mortality::from_qx(pluss::make_series({0.10, 1.20})));
  } catch (const std::exception &error) {
    out.writeln("invalid qx rejected =", error.what());
  }

  try {
    static_cast<void>(actuarial::mortality::gompertz(0.001, 0.0));
  } catch (const std::exception &error) {
    out.writeln("invalid law rejected =", error.what());
  }
}
