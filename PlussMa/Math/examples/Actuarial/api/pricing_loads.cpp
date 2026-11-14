#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const actuarial::Experience experience{
      .losses = pluss::make_series({100.0, 150.0}),
      .exposure = pluss::make_series({10.0, 10.0}),
      .current_premium = pluss::make_series({10.0, 15.0}),
  };
  const actuarial::PricingResult pure =
      actuarial::operators::price()(experience);
  const actuarial::PricingResult loaded = actuarial::operators::price({
      .fixed_expense = 2.0,
      .variable_expense_ratio = 0.20,
      .profit_ratio = 0.05,
  })(experience);

  out.writeln("pure indication =", pure.indicated_premium.items<double>());
  out.writeln("loaded indication =", loaded.indicated_premium.items<double>());
}
