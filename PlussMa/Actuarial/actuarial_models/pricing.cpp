#include "pluss/subsp/actuarial/pricing.h"

#include "pluss/subsp/actuarial/actuarial_models.h"

#include <cmath>
#include <string>
#include <string_view>

namespace pluss::subsp::actuarial {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series nonnegative_vector_(const Series &value, std::string_view name) {
  PLUSS_CHECK(value.defined(), std::string(name) + " must be defined");
  PLUSS_CHECK(value.dim() == 1, std::string(name) + " must be 1D");
  const Series out = value.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(out.isfinite().all().item<bool>(),
              std::string(name) + " must be finite");
  PLUSS_CHECK(out.ge(0.0).all().item<bool>(),
              std::string(name) + " must be >= 0");
  return out;
}

} // namespace

namespace operators {

Pricing price(Loads loads) {
  PLUSS_CHECK(std::isfinite(loads.fixed_expense) && loads.fixed_expense >= 0.0,
              "operators::price: fixed_expense must be finite and >= 0");
  PLUSS_CHECK(
      std::isfinite(loads.variable_expense_ratio) &&
          loads.variable_expense_ratio >= 0.0,
      "operators::price: variable_expense_ratio must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(loads.profit_ratio) && loads.profit_ratio >= 0.0,
              "operators::price: profit_ratio must be finite and >= 0");
  PLUSS_CHECK(
      loads.variable_expense_ratio + loads.profit_ratio < 1.0,
      "operators::price: variable_expense_ratio + profit_ratio must be < 1");

  return Pricing("price", [loads](const Experience &experience) {
    PricingResult result;
    result.pure_premium =
        actuarial_models::premium_curve(experience.losses, experience.exposure);
    result.indicated_premium = actuarial_models::indicated_premium(
        result.pure_premium, loads.fixed_expense, loads.variable_expense_ratio,
        loads.profit_ratio);
    result.rate_change = actuarial_models::rate_change(
        experience.current_premium, result.indicated_premium);
    return result;
  });
}

Credibility credibility(double full_credibility) {
  PLUSS_CHECK(
      std::isfinite(full_credibility) && full_credibility > 0.0,
      "operators::credibility: full_credibility must be finite and > 0");
  return Credibility("credibility", [full_credibility](
                                        const CredibilityInput &input) {
    const Series experience =
        nonnegative_vector_(input.experience, "credibility: experience");
    const Series manual =
        nonnegative_vector_(input.manual, "credibility: manual");
    const pluss::codem::Series z =
        actuarial_models::credibility_weight(input.exposure, full_credibility);
    PLUSS_CHECK(experience.numel() == manual.numel() &&
                    experience.numel() == z.numel(),
                "credibility: experience, manual and exposure must have the "
                "same length");
    return z.mul(experience).add(z.mul(-1.0).add(1.0).mul(manual));
  });
}

} // namespace operators
} // namespace pluss::subsp::actuarial
