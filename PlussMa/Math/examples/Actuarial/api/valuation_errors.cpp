#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(actuarial::discount::flat(-1.0));
  } catch (const std::exception &error) {
    out.writeln("invalid rate rejected =", error.what());
  }

  try {
    const actuarial::Valuation value = actuarial::operators::value({
        actuarial::mortality::from_qx(pluss::make_series({0.10, 1.00})),
        actuarial::discount::flat(0.03),
    });
    static_cast<void>(value(actuarial::cashflow::death(3, 1000.0)));
  } catch (const std::exception &error) {
    out.writeln("horizon rejected =", error.what());
  }
}
