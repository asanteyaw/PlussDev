#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const actuarial::Basis basis{
      actuarial::mortality::from_qx(pluss::make_series({0.10, 0.20, 1.00})),
      actuarial::discount::flat(0.05),
  };
  const actuarial::Valuation value = actuarial::operators::value(basis);
  const actuarial::Cashflow due =
      actuarial::cashflow::annuity(3, 100.0, actuarial::PaymentTiming::Start);
  const actuarial::Cashflow immediate =
      actuarial::cashflow::annuity(3, 100.0, actuarial::PaymentTiming::End);

  out.writeln("annuity due =", value(due).item<double>());
  out.writeln("annuity immediate =", value(immediate).item<double>());
}
