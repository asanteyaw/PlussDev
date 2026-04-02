#pragma once

#include "CodeM/core/Errors.h"
#include "pluss/subsp/math/finance/calibration.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::calibration::detail {

inline std::string op_prefix(const char* op) {
  return std::string("subsp::math::finance::calibration::") + op + ": ";
}

inline void validate_option_quotes(const std::vector<OptionQuote>& quotes, const char* op) {
  const std::string prefix = op_prefix(op);
  PLUSS_CHECK(!quotes.empty(), prefix + "quotes must be non-empty");
  for (const auto& q : quotes) {
    PLUSS_CHECK(q.type == pricing::OptionType::Call || q.type == pricing::OptionType::Put,
                prefix + "quote option type unsupported");
    PLUSS_CHECK(std::isfinite(q.strike) && q.strike > 0.0, prefix + "quote strike must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.maturity) && q.maturity > 0.0, prefix + "quote maturity must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.market_price) && q.market_price >= 0.0,
                prefix + "quote market_price must be finite and >= 0");
    PLUSS_CHECK(std::isfinite(q.weight) && q.weight >= 0.0, prefix + "quote weight must be finite and >= 0");
  }
}

inline void validate_rate_quotes(const std::vector<RateQuote>& quotes, const char* op) {
  const std::string prefix = op_prefix(op);
  PLUSS_CHECK(!quotes.empty(), prefix + "quotes must be non-empty");
  for (const auto& q : quotes) {
    PLUSS_CHECK(std::isfinite(q.maturity) && q.maturity > 0.0, prefix + "maturity must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.zcb_price) && q.zcb_price > 0.0 && q.zcb_price <= 1.5,
                prefix + "zcb_price must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.weight) && q.weight >= 0.0, prefix + "weight must be finite and >= 0");
  }
}

inline void validate_market_inputs(double spot, double rate, double dividend_yield, const char* op) {
  const std::string prefix = op_prefix(op);
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0, prefix + "spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), prefix + "rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield), prefix + "dividend_yield must be finite");
}

inline void validate_search_controls(std::int64_t random_starts,
                                     std::int64_t max_iterations,
                                     double step_fraction,
                                     double step_decay,
                                     double tolerance,
                                     const char* op) {
  const std::string prefix = op_prefix(op);
  PLUSS_CHECK(random_starts >= 1, prefix + "random_starts must be >= 1");
  PLUSS_CHECK(max_iterations > 0, prefix + "max_iterations must be > 0");
  PLUSS_CHECK(std::isfinite(step_fraction) && step_fraction > 0.0,
              prefix + "step_fraction must be finite and > 0");
  PLUSS_CHECK(std::isfinite(step_decay) && step_decay > 0.0 && step_decay < 1.0,
              prefix + "step_decay must be in (0,1)");
  PLUSS_CHECK(std::isfinite(tolerance) && tolerance > 0.0, prefix + "tolerance must be finite and > 0");
}

} // namespace pluss::subsp::math::finance::calibration::detail
