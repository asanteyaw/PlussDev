#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const actuarial::Mortality mortality =
      actuarial::mortality::from_qx(pluss::make_series({0.10, 0.20, 1.00}));
  const actuarial::Discount discount = actuarial::discount::flat(0.05);
  const actuarial::Basis basis{mortality, discount};
  const actuarial::Valuation value = actuarial::operators::value(basis);

  const actuarial::Cashflow annuity =
      actuarial::cashflow::annuity(3, 100.0, actuarial::PaymentTiming::Start);
  const actuarial::Cashflow death = actuarial::cashflow::death(3, 1000.0);
  const actuarial::Cashflow survival = actuarial::cashflow::survival(3, 500.0);
  const actuarial::Cashflow endowment =
      actuarial::cashflow::endowment(3, 1000.0, 500.0);
  const actuarial::Cashflow custom =
      actuarial::cashflow::make(pluss::make_series({25.0, 25.0, 25.0, 0.0}),
                                pluss::make_series({100.0, 150.0, 200.0}));

  out.writeln("discount factors =", discount.factors(3).items<double>());
  out.writeln("annuity EPV =", value(annuity).item<double>());
  out.writeln("death EPV =", value(death).item<double>());
  out.writeln("survival EPV =", value(survival).item<double>());
  out.writeln("endowment EPV =", value(endowment).item<double>());
  out.writeln("custom EPV =", value(custom).item<double>());

  const actuarial::Discount curve =
      actuarial::discount::curve(pluss::make_series({1.0, 0.95, 0.89, 0.82}));
  out.writeln("curve factors =", curve.factors(3).items<double>());
}
