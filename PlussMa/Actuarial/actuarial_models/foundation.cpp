#include "pluss/subsp/actuarial/actuarial_models.h"

#include "pluss/config.h"
#include "pluss/subsp/actuarial/pricing.h"
#include "pluss/subsp/actuarial/reserving.h"

#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::subsp::actuarial::actuarial_models {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series sanitize_nonnegative_(const Series &x, std::string_view op,
                             std::string_view name, bool require_1d = false) {
  PLUSS_CHECK(x.defined(),
              std::string(op) + ": " + std::string(name) + " must be defined");
  if (require_1d) {
    PLUSS_CHECK(x.dim() == 1,
                std::string(op) + ": " + std::string(name) + " must be 1D");
  }
  Series out = x.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(out.isfinite().all().item<bool>(),
              std::string(op) + ": " + std::string(name) + " must be finite");
  PLUSS_CHECK(out.ge(0.0).all().item<bool>(),
              std::string(op) + ": " + std::string(name) + " must be >= 0");
  return out;
}

Series sanitize_ratio_input_pair_(const Series &num, const Series &den,
                                  std::string_view op,
                                  std::string_view den_name) {
  const Series n =
      sanitize_nonnegative_(num, op, "numerator", /*require_1d=*/true);
  const Series d =
      sanitize_nonnegative_(den, op, den_name, /*require_1d=*/true);
  PLUSS_CHECK(n.numel() == d.numel(), std::string(op) + ": numerator and " +
                                          std::string(den_name) +
                                          " must have same length");
  const double denom = d.sum().item<double>();
  PLUSS_CHECK(denom > 0.0, std::string(op) + ": aggregate " +
                               std::string(den_name) + " must be > 0");
  return n.sum().div(denom);
}

std::pair<Series, Series> sanitize_nonnegative_pair_same_shape_(
    const Series &lhs, const Series &rhs, std::string_view op,
    std::string_view lhs_name, std::string_view rhs_name) {
  const Series a = sanitize_nonnegative_(lhs, op, lhs_name);
  const Series b = sanitize_nonnegative_(rhs, op, rhs_name);
  PLUSS_CHECK(a.numel() == b.numel(),
              std::string(op) + ": " + std::string(lhs_name) + " and " +
                  std::string(rhs_name) + " must have same length");
  return {a, b};
}

Series zero_safe_ratio_(const Series &numerator, const Series &denominator,
                        std::string_view op, std::string_view den_name) {
  const auto [num, den] = sanitize_nonnegative_pair_same_shape_(
      numerator, denominator, op, "numerator", den_name);
  const Series mask = den.gt(0.0).to(ScalarType::Float64);
  return num.div(den.maximum(1e-30)).mul(mask);
}

Series sanitize_percent_reported_(const Series &percent_reported,
                                  std::string_view op) {
  const Series out =
      sanitize_nonnegative_(percent_reported, op, "percent_reported");
  PLUSS_CHECK(out.le(1.0).all().item<bool>(),
              std::string(op) + ": percent_reported must be <= 1");
  return out;
}

} // namespace

Series claim_cost(const Series &exposure, const Series &claim_frequency,
                  const Series &claim_severity) {
  const Series exp = sanitize_nonnegative_(exposure, "claim_cost", "exposure");
  const Series freq =
      sanitize_nonnegative_(claim_frequency, "claim_cost", "claim_frequency");
  const Series sev =
      sanitize_nonnegative_(claim_severity, "claim_cost", "claim_severity");
  return exp.mul(freq).mul(sev);
}

Series pure_premium(const Series &incurred_losses, const Series &exposure) {
  return sanitize_ratio_input_pair_(incurred_losses, exposure, "pure_premium",
                                    "exposure");
}

Series loss_ratio(const Series &incurred_losses, const Series &earned_premium) {
  return sanitize_ratio_input_pair_(incurred_losses, earned_premium,
                                    "loss_ratio", "earned_premium");
}

Series premium_curve(const Series &incurred_losses, const Series &exposure) {
  return zero_safe_ratio_(incurred_losses, exposure, "premium_curve",
                          "exposure");
}

Series ratio_curve(const Series &incurred_losses,
                   const Series &earned_premium) {
  return zero_safe_ratio_(incurred_losses, earned_premium, "ratio_curve",
                          "earned_premium");
}

Series credibility_weight(const Series &exposure, double k) {
  PLUSS_CHECK(std::isfinite(k), "credibility_weight: k must be finite");
  PLUSS_CHECK(k > 0.0, "credibility_weight: k must be > 0");
  const Series exp =
      sanitize_nonnegative_(exposure, "credibility_weight", "exposure");
  return exp.div(exp.add(k));
}

Series credibility_blend(const Series &experience_pure_premium,
                         const Series &manual_pure_premium,
                         const Series &exposure, double credibility_k) {
  const auto [exp_pp, manual_pp] = sanitize_nonnegative_pair_same_shape_(
      experience_pure_premium, manual_pure_premium, "credibility_blend",
      "experience_pure_premium", "manual_pure_premium");
  return operators::credibility(credibility_k)({
      .experience = exp_pp,
      .manual = manual_pp,
      .exposure = exposure,
  });
}

Series indicated_premium(const Series &pure_premium, double fixed_expense,
                         double variable_expense_ratio, double profit_ratio) {
  PLUSS_CHECK(std::isfinite(fixed_expense),
              "indicated_premium: fixed_expense must be finite");
  PLUSS_CHECK(fixed_expense >= 0.0,
              "indicated_premium: fixed_expense must be >= 0");
  PLUSS_CHECK(std::isfinite(variable_expense_ratio),
              "indicated_premium: variable_expense_ratio must be finite");
  PLUSS_CHECK(std::isfinite(profit_ratio),
              "indicated_premium: profit_ratio must be finite");
  PLUSS_CHECK(variable_expense_ratio >= 0.0,
              "indicated_premium: variable_expense_ratio must be >= 0");
  PLUSS_CHECK(profit_ratio >= 0.0,
              "indicated_premium: profit_ratio must be >= 0");

  const double denominator = 1.0 - variable_expense_ratio - profit_ratio;
  PLUSS_CHECK(
      denominator > 0.0,
      "indicated_premium: variable_expense_ratio + profit_ratio must be < 1");

  const Series pure =
      sanitize_nonnegative_(pure_premium, "indicated_premium", "pure_premium");
  return pure.add(fixed_expense).div(denominator);
}

Series rate_change(const Series &current_premium,
                   const Series &indicated_premium) {
  const Series current =
      sanitize_nonnegative_(current_premium, "rate_change", "current_premium");
  const Series indicated = sanitize_nonnegative_(
      indicated_premium, "rate_change", "indicated_premium");
  PLUSS_CHECK(current.numel() == indicated.numel(),
              "rate_change: current_premium and indicated_premium must have "
              "same length");
  PLUSS_CHECK(current.gt(0.0).all().item<bool>(),
              "rate_change: current_premium must be strictly > 0");
  return indicated.div(current).add(-1.0);
}

Series experience_change(const Series &incurred_losses, const Series &exposure,
                         const Series &current_premium, double fixed_expense,
                         double variable_expense_ratio, double profit_ratio) {
  return operators::price({
      .fixed_expense = fixed_expense,
      .variable_expense_ratio = variable_expense_ratio,
      .profit_ratio = profit_ratio,
  })({
         .losses = incurred_losses,
         .exposure = exposure,
         .current_premium = current_premium,
     })
      .rate_change;
}

Series bf_ultimate(const Series &reported_losses,
                   const Series &expected_ultimate_losses,
                   const Series &percent_reported) {
  const auto [reported, expected] = sanitize_nonnegative_pair_same_shape_(
      reported_losses, expected_ultimate_losses, "bf_ultimate",
      "reported_losses", "expected_ultimate_losses");
  const Series pct =
      sanitize_percent_reported_(percent_reported, "bf_ultimate");
  PLUSS_CHECK(reported.numel() == pct.numel(),
              "bf_ultimate: percent_reported must have same length");
  return reported.add(expected.mul(pct.mul(-1.0).add(1.0)));
}

Series bf_reserve(const Series &expected_ultimate_losses,
                  const Series &percent_reported) {
  const Series expected = sanitize_nonnegative_(
      expected_ultimate_losses, "bf_reserve", "expected_ultimate_losses");
  const Series pct = sanitize_percent_reported_(percent_reported, "bf_reserve");
  PLUSS_CHECK(expected.numel() == pct.numel(),
              "bf_reserve: expected_ultimate_losses and percent_reported must "
              "have same length");
  return expected.mul(pct.mul(-1.0).add(1.0));
}

Series age_factors(const Series &cumulative_triangle) {
  return operators::chain_ladder()(triangle::cumulative(cumulative_triangle))
      .age_to_age;
}

Series ladder_cdf(const Series &age_to_age_factors) {
  const Series f =
      sanitize_nonnegative_(age_to_age_factors, "ladder_cdf",
                            "age_to_age_factors", /*require_1d=*/true);
  PLUSS_CHECK(f.ge(1.0).all().item<bool>(),
              "ladder_cdf: age_to_age_factors must be >= 1");
  if (f.numel() == 0) {
    return Series::ones(std::vector<std::int64_t>{1}, ScalarType::Float64)
        .to(f.platform());
  }
  const Series tail = f.flip(/*dim=*/0).cumprod(/*dim=*/0).flip(/*dim=*/0);
  const Series one = tail.slice(/*dim=*/0, /*start=*/0, /*end=*/1, /*step=*/1)
                         .mul(0.0)
                         .add(1.0);
  return tail.unite({one}, /*dim=*/0).to(tail.platform(), ScalarType::Float64);
}

Series ladder_ultimate(const Series &cumulative_triangle,
                       const Series &cdf_to_ultimate) {
  return operators::chain_ladder(cdf_to_ultimate)(
             triangle::cumulative(cumulative_triangle))
      .ultimate;
}

Series ladder_reserve(const Series &cumulative_triangle,
                      const Series &cdf_to_ultimate) {
  return operators::chain_ladder(cdf_to_ultimate)(
             triangle::cumulative(cumulative_triangle))
      .reserve;
}

} // namespace pluss::subsp::actuarial::actuarial_models
