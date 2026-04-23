#include "pluss/subsp/actuarial/actuarial_models.h"

#include "pluss/config.h"

#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::subsp::actuarial::actuarial_models {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series sanitize_nonnegative_(const Series& x,
                             std::string_view op,
                             std::string_view name,
                             bool require_1d = false) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": " + std::string(name) + " must be defined");
  if (require_1d) {
    PLUSS_CHECK(x.dim() == 1, std::string(op) + ": " + std::string(name) + " must be 1D");
  }
  Series out = x.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(out.isfinite().all().item<bool>(), std::string(op) + ": " + std::string(name) + " must be finite");
  PLUSS_CHECK(out.ge(0.0).all().item<bool>(), std::string(op) + ": " + std::string(name) + " must be >= 0");
  return out;
}

Series sanitize_ratio_input_pair_(const Series& num,
                                  const Series& den,
                                  std::string_view op,
                                  std::string_view den_name) {
  const Series n = sanitize_nonnegative_(num, op, "numerator", /*require_1d=*/true);
  const Series d = sanitize_nonnegative_(den, op, den_name, /*require_1d=*/true);
  PLUSS_CHECK(n.numel() == d.numel(),
              std::string(op) + ": numerator and " + std::string(den_name) + " must have same length");
  const double denom = d.sum().item<double>();
  PLUSS_CHECK(denom > 0.0, std::string(op) + ": aggregate " + std::string(den_name) + " must be > 0");
  return n.sum().div(denom);
}

std::pair<Series, Series> sanitize_nonnegative_pair_same_shape_(const Series& lhs,
                                                                const Series& rhs,
                                                                std::string_view op,
                                                                std::string_view lhs_name,
                                                                std::string_view rhs_name) {
  const Series a = sanitize_nonnegative_(lhs, op, lhs_name);
  const Series b = sanitize_nonnegative_(rhs, op, rhs_name);
  PLUSS_CHECK(a.numel() == b.numel(),
              std::string(op) + ": " + std::string(lhs_name) + " and " + std::string(rhs_name) + " must have same length");
  return {a, b};
}

Series zero_safe_ratio_(const Series& numerator,
                        const Series& denominator,
                        std::string_view op,
                        std::string_view den_name) {
  const auto [num, den] = sanitize_nonnegative_pair_same_shape_(numerator, denominator, op, "numerator", den_name);
  const Series mask = den.gt(0.0).to(ScalarType::Float64);
  return num.div(den.maximum(1e-30)).mul(mask);
}

Series sanitize_percent_reported_(const Series& percent_reported, std::string_view op) {
  const Series out = sanitize_nonnegative_(percent_reported, op, "percent_reported");
  PLUSS_CHECK(out.le(1.0).all().item<bool>(), std::string(op) + ": percent_reported must be <= 1");
  return out;
}

Series sanitize_cumulative_triangle_(const Series& cumulative_triangle,
                                     std::string_view op,
                                     bool require_two_development_periods = false) {
  PLUSS_CHECK(cumulative_triangle.defined(),
              std::string(op) + ": cumulative_triangle must be defined");
  PLUSS_CHECK(cumulative_triangle.dim() == 2,
              std::string(op) + ": cumulative_triangle must be 2D [origin, development]");
  PLUSS_CHECK(cumulative_triangle.sizes()[0] > 0 && cumulative_triangle.sizes()[1] > 0,
              std::string(op) + ": cumulative_triangle must be non-empty");
  if (require_two_development_periods) {
    PLUSS_CHECK(cumulative_triangle.sizes()[1] >= 2,
                std::string(op) + ": cumulative_triangle must have at least 2 development periods");
  }
  Series tri = cumulative_triangle.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(tri.isfinite().all().item<bool>(),
              std::string(op) + ": cumulative_triangle must be finite");
  PLUSS_CHECK(tri.ge(0.0).all().item<bool>(),
              std::string(op) + ": cumulative_triangle must be >= 0");

  const std::int64_t dev = tri.sizes()[1];
  if (dev >= 2) {
    const Series curr = tri.slice(/*dim=*/1, /*start=*/0, /*end=*/dev - 1, /*step=*/1);
    const Series next = tri.slice(/*dim=*/1, /*start=*/1, /*end=*/dev, /*step=*/1);
    const Series observed_pair = curr.gt(0.0).logical_and(next.gt(0.0));
    const bool monotone_ok = observed_pair.logical_not().logical_or(next.ge(curr)).all().item<bool>();
    PLUSS_CHECK(monotone_ok,
                std::string(op) + ": cumulative_triangle must be non-decreasing along observed development cells");
  }
  return tri;
}

Series sanitize_chain_ladder_cdf_(const Series& cdf_to_ultimate,
                                  std::int64_t development_periods,
                                  std::string_view op) {
  PLUSS_CHECK(cdf_to_ultimate.defined(),
              std::string(op) + ": cdf_to_ultimate must be defined");
  PLUSS_CHECK(cdf_to_ultimate.dim() == 1,
              std::string(op) + ": cdf_to_ultimate must be 1D");
  PLUSS_CHECK(cdf_to_ultimate.numel() == development_periods,
              std::string(op) + ": cdf_to_ultimate length must match triangle development count");
  const Series cdf = cdf_to_ultimate.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(cdf.isfinite().all().item<bool>(),
              std::string(op) + ": cdf_to_ultimate must be finite");
  PLUSS_CHECK(cdf.ge(1.0).all().item<bool>(),
              std::string(op) + ": cdf_to_ultimate values must be >= 1");
  if (development_periods >= 2) {
    const Series head = cdf.slice(/*dim=*/0, /*start=*/0, /*end=*/development_periods - 1, /*step=*/1);
    const Series tail = cdf.slice(/*dim=*/0, /*start=*/1, /*end=*/development_periods, /*step=*/1);
    PLUSS_CHECK(head.ge(tail).all().item<bool>(),
                std::string(op) + ": cdf_to_ultimate must be non-increasing by development age");
  }
  PLUSS_CHECK(cdf.select(/*dim=*/0, /*index=*/development_periods - 1).sub(1.0).abs().le(1e-10).item<bool>(),
              std::string(op) + ": last cdf_to_ultimate value must equal 1");
  return cdf;
}

struct LatestObservedByOrigin {
  Series latest_development;
  Series latest_cumulative;
  Series has_observation;
};

LatestObservedByOrigin latest_observed_by_origin_(const Series& cumulative_triangle) {
  const std::int64_t origin = cumulative_triangle.sizes()[0];
  const std::int64_t dev = cumulative_triangle.sizes()[1];
  const Series observed = cumulative_triangle.gt(0.0);
  const Series has_obs = observed.any(/*dim=*/1, /*keepdim=*/false);
  const Series dev_index = Series::arange(0, dev, 1, ScalarType::Int64)
                               .to(cumulative_triangle.platform(), ScalarType::Float64)
                               .reshape({1, dev});
  const Series latest_dev = observed.to(ScalarType::Float64)
                                .mul(dev_index)
                                .amax(/*dim=*/1, /*keepdim=*/false)
                                .to(ScalarType::Int64)
                                .reshape({origin});
  const Series latest_cumulative = cumulative_triangle.gather(/*dim=*/1, latest_dev.reshape({origin, 1}))
                                       .reshape({origin});
  return {
      .latest_development = latest_dev,
      .latest_cumulative = latest_cumulative,
      .has_observation = has_obs,
  };
}

} // namespace

Series expected_claim_cost(const Series& exposure,
                           const Series& claim_frequency,
                           const Series& claim_severity) {
  const Series exp = sanitize_nonnegative_(exposure, "expected_claim_cost", "exposure");
  const Series freq = sanitize_nonnegative_(claim_frequency, "expected_claim_cost", "claim_frequency");
  const Series sev = sanitize_nonnegative_(claim_severity, "expected_claim_cost", "claim_severity");
  return exp.mul(freq).mul(sev);
}

Series pure_premium(const Series& incurred_losses, const Series& exposure) {
  return sanitize_ratio_input_pair_(incurred_losses, exposure, "pure_premium", "exposure");
}

Series loss_ratio(const Series& incurred_losses, const Series& earned_premium) {
  return sanitize_ratio_input_pair_(incurred_losses, earned_premium, "loss_ratio", "earned_premium");
}

Series pure_premium_curve(const Series& incurred_losses, const Series& exposure) {
  return zero_safe_ratio_(incurred_losses, exposure, "pure_premium_curve", "exposure");
}

Series loss_ratio_curve(const Series& incurred_losses, const Series& earned_premium) {
  return zero_safe_ratio_(incurred_losses, earned_premium, "loss_ratio_curve", "earned_premium");
}

Series credibility_weight(const Series& exposure, double k) {
  PLUSS_CHECK(std::isfinite(k), "credibility_weight: k must be finite");
  PLUSS_CHECK(k > 0.0, "credibility_weight: k must be > 0");
  const Series exp = sanitize_nonnegative_(exposure, "credibility_weight", "exposure");
  return exp.div(exp.add(k));
}

Series credibility_blended_pure_premium(const Series& experience_pure_premium,
                                        const Series& manual_pure_premium,
                                        const Series& exposure,
                                        double credibility_k) {
  const auto [exp_pp, manual_pp] = sanitize_nonnegative_pair_same_shape_(experience_pure_premium,
                                                                          manual_pure_premium,
                                                                          "credibility_blended_pure_premium",
                                                                          "experience_pure_premium",
                                                                          "manual_pure_premium");
  const Series z = credibility_weight(exposure, credibility_k);
  PLUSS_CHECK(exp_pp.numel() == z.numel(),
              "credibility_blended_pure_premium: premiums and exposure must have same length");
  return z.mul(exp_pp).add(z.mul(-1.0).add(1.0).mul(manual_pp));
}

Series indicated_premium(const Series& pure_premium,
                         double fixed_expense,
                         double variable_expense_ratio,
                         double profit_ratio) {
  PLUSS_CHECK(std::isfinite(fixed_expense), "indicated_premium: fixed_expense must be finite");
  PLUSS_CHECK(fixed_expense >= 0.0, "indicated_premium: fixed_expense must be >= 0");
  PLUSS_CHECK(std::isfinite(variable_expense_ratio),
              "indicated_premium: variable_expense_ratio must be finite");
  PLUSS_CHECK(std::isfinite(profit_ratio), "indicated_premium: profit_ratio must be finite");
  PLUSS_CHECK(variable_expense_ratio >= 0.0,
              "indicated_premium: variable_expense_ratio must be >= 0");
  PLUSS_CHECK(profit_ratio >= 0.0, "indicated_premium: profit_ratio must be >= 0");

  const double denominator = 1.0 - variable_expense_ratio - profit_ratio;
  PLUSS_CHECK(denominator > 0.0,
              "indicated_premium: variable_expense_ratio + profit_ratio must be < 1");

  const Series pure = sanitize_nonnegative_(pure_premium, "indicated_premium", "pure_premium");
  return pure.add(fixed_expense).div(denominator);
}

Series indicated_rate_change(const Series& current_premium, const Series& indicated_premium) {
  const Series current = sanitize_nonnegative_(current_premium, "indicated_rate_change", "current_premium");
  const Series indicated = sanitize_nonnegative_(indicated_premium, "indicated_rate_change", "indicated_premium");
  PLUSS_CHECK(current.numel() == indicated.numel(),
              "indicated_rate_change: current_premium and indicated_premium must have same length");
  PLUSS_CHECK(current.gt(0.0).all().item<bool>(),
              "indicated_rate_change: current_premium must be strictly > 0");
  return indicated.div(current).add(-1.0);
}

Series indicated_rate_change_from_experience(const Series& incurred_losses,
                                             const Series& exposure,
                                             const Series& current_premium,
                                             double fixed_expense,
                                             double variable_expense_ratio,
                                             double profit_ratio) {
  const Series pp_curve = pure_premium_curve(incurred_losses, exposure);
  const Series indicated =
      indicated_premium(pp_curve, fixed_expense, variable_expense_ratio, profit_ratio);
  return indicated_rate_change(current_premium, indicated);
}

Series bornhuetter_ferguson_ultimate(const Series& reported_losses,
                                     const Series& expected_ultimate_losses,
                                     const Series& percent_reported) {
  const auto [reported, expected] = sanitize_nonnegative_pair_same_shape_(reported_losses,
                                                                           expected_ultimate_losses,
                                                                           "bornhuetter_ferguson_ultimate",
                                                                           "reported_losses",
                                                                           "expected_ultimate_losses");
  const Series pct = sanitize_percent_reported_(percent_reported, "bornhuetter_ferguson_ultimate");
  PLUSS_CHECK(reported.numel() == pct.numel(),
              "bornhuetter_ferguson_ultimate: percent_reported must have same length");
  return reported.add(expected.mul(pct.mul(-1.0).add(1.0)));
}

Series bornhuetter_ferguson_reserve(const Series& expected_ultimate_losses,
                                    const Series& percent_reported) {
  const Series expected = sanitize_nonnegative_(expected_ultimate_losses,
                                                "bornhuetter_ferguson_reserve",
                                                "expected_ultimate_losses");
  const Series pct = sanitize_percent_reported_(percent_reported, "bornhuetter_ferguson_reserve");
  PLUSS_CHECK(expected.numel() == pct.numel(),
              "bornhuetter_ferguson_reserve: expected_ultimate_losses and percent_reported must have same length");
  return expected.mul(pct.mul(-1.0).add(1.0));
}

Series chain_ladder_age_to_age(const Series& cumulative_triangle) {
  const Series tri = sanitize_cumulative_triangle_(cumulative_triangle,
                                                   "chain_ladder_age_to_age",
                                                   /*require_two_development_periods=*/true);
  const std::int64_t dev = tri.sizes()[1];
  const Series curr = tri.slice(/*dim=*/1, /*start=*/0, /*end=*/dev - 1, /*step=*/1);
  const Series next = tri.slice(/*dim=*/1, /*start=*/1, /*end=*/dev, /*step=*/1);
  const Series paired = curr.gt(0.0).logical_and(next.gt(0.0)).to(ScalarType::Float64);
  const Series numerator = next.mul(paired).sum(/*dim=*/0, /*keepdim=*/false);
  const Series denominator = curr.mul(paired).sum(/*dim=*/0, /*keepdim=*/false);
  PLUSS_CHECK(denominator.gt(0.0).all().item<bool>(),
              "chain_ladder_age_to_age: each age pair must have at least one observed origin");
  return numerator.div(denominator);
}

Series chain_ladder_cdf(const Series& age_to_age_factors) {
  const Series f = sanitize_nonnegative_(age_to_age_factors, "chain_ladder_cdf", "age_to_age_factors", /*require_1d=*/true);
  PLUSS_CHECK(f.ge(1.0).all().item<bool>(),
              "chain_ladder_cdf: age_to_age_factors must be >= 1");
  if (f.numel() == 0) {
    return Series::ones(std::vector<std::int64_t>{1}, ScalarType::Float64).to(f.platform());
  }
  const Series tail = f.flip(/*dim=*/0).cumprod(/*dim=*/0).flip(/*dim=*/0);
  const Series one = tail.slice(/*dim=*/0, /*start=*/0, /*end=*/1, /*step=*/1).mul(0.0).add(1.0);
  return tail.unite({one}, /*dim=*/0).to(tail.platform(), ScalarType::Float64);
}

Series chain_ladder_ultimate(const Series& cumulative_triangle, const Series& cdf_to_ultimate) {
  const Series tri = sanitize_cumulative_triangle_(cumulative_triangle, "chain_ladder_ultimate");
  const std::int64_t origin = tri.sizes()[0];
  const std::int64_t dev = tri.sizes()[1];
  const Series cdf = sanitize_chain_ladder_cdf_(cdf_to_ultimate, dev, "chain_ladder_ultimate");
  const LatestObservedByOrigin latest = latest_observed_by_origin_(tri);
  const Series factor = cdf.index_select(/*dim=*/0, latest.latest_development.reshape({origin}));
  return latest.has_observation.to(ScalarType::Float64).mul(latest.latest_cumulative.mul(factor));
}

Series chain_ladder_reserve(const Series& cumulative_triangle, const Series& cdf_to_ultimate) {
  const Series tri = sanitize_cumulative_triangle_(cumulative_triangle, "chain_ladder_reserve");
  const Series ultimate = chain_ladder_ultimate(tri, cdf_to_ultimate);
  const LatestObservedByOrigin latest = latest_observed_by_origin_(tri);
  return ultimate.sub(latest.latest_cumulative).clamp_min(0.0);
}

} // namespace pluss::subsp::actuarial::actuarial_models
