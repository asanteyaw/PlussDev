#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

namespace actuarial = pluss::ma::actuarial;

bool approx(double lhs, double rhs, double tolerance = 1e-11) {
  return std::abs(lhs - rhs) <= tolerance;
}

void expect(bool condition, const std::string& message, int* failures) {
  if (!condition) {
    ++(*failures);
    std::cerr << "[FAIL] " << message << '\n';
  }
}

double scalar(const pluss::Series& value) {
  return value.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

} // namespace

int main() {
  int failures = 0;

  const pluss::Series qx = pluss::make_series({0.2, 0.1});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const actuarial::Basis basis{
      .mortality = mortality,
      .discount = actuarial::discount::flat(0.0),
  };
  const actuarial::Valuation value = actuarial::operators::value(basis);

  expect(mortality.horizon() == 2, "mortality horizon", &failures);
  expect(mortality.qx().scalar_type() == pluss::ScalarType::Float64,
         "mortality uses a canonical floating dtype", &failures);
  expect(approx(scalar(value(actuarial::cashflow::annuity(
                    2, 1.0, actuarial::PaymentTiming::Start))),
                scalar(actuarial::annuity_due(qx, 0.0))),
         "annuity-due parity", &failures);
  expect(approx(scalar(value(actuarial::cashflow::annuity(
                    2, 1.0, actuarial::PaymentTiming::End))),
                scalar(actuarial::annuity_immediate(qx, 0.0))),
         "annuity-immediate parity", &failures);
  expect(approx(scalar(value(actuarial::cashflow::death(2))),
                scalar(actuarial::term_insurance(qx, 0.0))),
         "term-insurance parity", &failures);
  expect(approx(scalar(value(actuarial::cashflow::survival(2))),
                scalar(actuarial::pure_endowment(qx, 0.0))),
         "pure-endowment parity", &failures);
  expect(approx(scalar(value(actuarial::cashflow::endowment(2))),
                scalar(actuarial::endowment_insurance(qx, 0.0))),
         "endowment parity", &failures);

  // Payments at times 0, 1 and 2 plus death benefits in years 1 and 2.
  const actuarial::Cashflow varying = actuarial::cashflow::make(
      pluss::make_series({100.0, 50.0, 25.0}),
      pluss::make_series({1000.0, 800.0}));
  const actuarial::Valuation curved = actuarial::operators::value({
      .mortality = mortality,
      .discount = actuarial::discount::curve(
          pluss::make_series({1.0, 0.95, 0.90})),
  });
  const double expected =
      100.0 + 50.0 * 0.8 * 0.95 + 25.0 * 0.72 * 0.90 +
      1000.0 * 0.2 * 0.95 + 800.0 * 0.8 * 0.1 * 0.90;
  expect(approx(scalar(curved(varying)), expected),
         "varying contingent cashflow with discount curve", &failures);

  const actuarial::Mortality from_lives = actuarial::mortality::from_lx(
      pluss::make_series({100.0, 80.0, 72.0}));
  expect(from_lives.qx().allclose(qx), "mortality from lives", &failures);

  const actuarial::Mortality selected = actuarial::mortality::select_ultimate(
      pluss::make_series({0.1}), pluss::make_series({0.2}), 3);
  expect(selected.qx().allclose(pluss::make_series({0.1, 0.2, 0.2})),
         "select and ultimate mortality", &failures);

  const actuarial::MortalityModel gompertz =
      actuarial::mortality::gompertz(0.0001, 1.08);
  const pluss::Series ages = pluss::make_series({40.0, 41.0, 42.0});
  expect(gompertz.key() == "gompertz", "mortality model key", &failures);
  expect(gompertz(ages).qx().allclose(
             actuarial::mortality::gompertz(ages, 0.0001, 1.08).qx()),
         "reusable mortality model", &failures);

  const actuarial::Operator<pluss::Series(const pluss::Series&)> double_values(
      "double", [](const pluss::Series& input) { return input.mul(2.0); });
  expect(double_values.key() == "double", "custom operator key", &failures);
  expect(approx(double_values(pluss::make_series(3.0)).item<double>(), 6.0),
         "custom operator application", &failures);

  bool rejected = false;
  try {
    (void)value(actuarial::cashflow::death(3));
  } catch (const pluss::Error&) {
    rejected = true;
  }
  expect(rejected, "cashflow beyond mortality horizon is rejected", &failures);

  rejected = false;
  try {
    (void)actuarial::discount::curve(pluss::make_series({1.0, 0.0}));
  } catch (const pluss::Error&) {
    rejected = true;
  }
  expect(rejected, "non-positive discount factors are rejected", &failures);

  rejected = false;
  try {
    (void)actuarial::cashflow::make(pluss::make_series({1.0, 1.0}),
                                    pluss::make_series({1.0, 1.0}));
  } catch (const pluss::Error&) {
    rejected = true;
  }
  expect(rejected, "inconsistent cashflow horizons are rejected", &failures);

  return failures == 0 ? 0 : 1;
}
