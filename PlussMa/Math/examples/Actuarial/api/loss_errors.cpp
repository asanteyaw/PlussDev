#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(actuarial::operators::deductible(-10.0));
  } catch (const std::exception &error) {
    out.writeln("deductible rejected =", error.what());
  }
  try {
    static_cast<void>(actuarial::operators::coinsurance(1.20));
  } catch (const std::exception &error) {
    out.writeln("share rejected =", error.what());
  }
}
