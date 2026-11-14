#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"
#include "pluss/subsp/actuarial/operator.h"

namespace pluss::subsp::actuarial {

struct CODEM_API Experience final {
  pluss::codem::Series losses = pluss::series::undefined();
  pluss::codem::Series exposure = pluss::series::undefined();
  pluss::codem::Series current_premium = pluss::series::undefined();
};

struct CODEM_API Loads final {
  double fixed_expense = 0.0;
  double variable_expense_ratio = 0.0;
  double profit_ratio = 0.0;
};

struct CODEM_API PricingResult final {
  pluss::codem::Series pure_premium = pluss::series::undefined();
  pluss::codem::Series indicated_premium = pluss::series::undefined();
  pluss::codem::Series rate_change = pluss::series::undefined();
};

struct CODEM_API CredibilityInput final {
  pluss::codem::Series experience = pluss::series::undefined();
  pluss::codem::Series manual = pluss::series::undefined();
  pluss::codem::Series exposure = pluss::series::undefined();
};

using Pricing = Operator<PricingResult(const Experience&)>;
using Credibility = Operator<pluss::codem::Series(const CredibilityInput&)>;

namespace operators {
[[nodiscard]] CODEM_API Pricing price(Loads loads = {});
[[nodiscard]] CODEM_API Credibility credibility(double full_credibility);
} // namespace operators

} // namespace pluss::subsp::actuarial
