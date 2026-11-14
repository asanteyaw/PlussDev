#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(actuarial::operators::price({
        .variable_expense_ratio = 0.80,
        .profit_ratio = 0.20,
    }));
  } catch (const std::exception &error) {
    out.writeln("loads rejected =", error.what());
  }
  try {
    const actuarial::Pricing price = actuarial::operators::price();
    static_cast<void>(price({
        .losses = pluss::make_series({100.0}),
        .exposure = pluss::make_series({0.0}),
        .current_premium = pluss::make_series({10.0}),
    }));
  } catch (const std::exception &error) {
    out.writeln("exposure rejected =", error.what());
  }
}
