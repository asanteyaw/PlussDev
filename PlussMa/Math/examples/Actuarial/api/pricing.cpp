#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const actuarial::Experience experience{
      .losses = pluss::make_series({100.0, 0.0, 50.0}),
      .exposure = pluss::make_series({10.0, 0.0, 25.0}),
      .current_premium = pluss::make_series({12.0, 8.0, 3.0}),
  };
  const actuarial::Loads loads{
      .fixed_expense = 1.0,
      .variable_expense_ratio = 0.20,
      .profit_ratio = 0.10,
  };
  const actuarial::Pricing pricing = actuarial::operators::price(loads);
  const actuarial::PricingResult result = pricing(experience);

  out.writeln("pure premium =", result.pure_premium.items<double>());
  out.writeln("indicated premium =", result.indicated_premium.items<double>());
  out.writeln("rate change =", result.rate_change.items<double>());

  const actuarial::Credibility credibility =
      actuarial::operators::credibility(100.0);
  const pluss::Series blended = credibility({
      .experience = pluss::make_series({12.0, 15.0, 20.0}),
      .manual = pluss::make_series({10.0, 10.0, 10.0}),
      .exposure = pluss::make_series({0.0, 50.0, 200.0}),
  });
  out.writeln("credibility blend =", blended.items<double>());
}
