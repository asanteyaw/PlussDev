#include "pluss/subsp/actuarial/loss_models.h"

#include "pluss/codem/series_factory.h"
#include "pluss/config.h"
#include "pluss/subsp/stats/distr.h"

#include <cmath>
#include <string>
#include <string_view>

namespace pluss::subsp::actuarial::loss_models {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series scalar_on_(double v, pluss::codem::Platform platform) {
  return pluss::make_series(v, pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(platform));
}

void validate_finite_scalar_(double value, std::string_view op, std::string_view name) {
  PLUSS_CHECK(std::isfinite(value), std::string(op) + ": " + std::string(name) + " must be finite");
}

void validate_nonnegative_scalar_(double value, std::string_view op, std::string_view name) {
  validate_finite_scalar_(value, op, name);
  PLUSS_CHECK(value >= 0.0, std::string(op) + ": " + std::string(name) + " must be >= 0");
}

void validate_positive_scalar_(double value, std::string_view op, std::string_view name) {
  validate_finite_scalar_(value, op, name);
  PLUSS_CHECK(value > 0.0, std::string(op) + ": " + std::string(name) + " must be > 0");
}

void validate_probability_open_closed_(double value, std::string_view op, std::string_view name) {
  validate_finite_scalar_(value, op, name);
  PLUSS_CHECK(value > 0.0 && value <= 1.0,
              std::string(op) + ": " + std::string(name) + " must lie in (0,1]");
}

void validate_probability_open_(double value, std::string_view op, std::string_view name) {
  validate_finite_scalar_(value, op, name);
  PLUSS_CHECK(value > 0.0 && value < 1.0,
              std::string(op) + ": " + std::string(name) + " must lie in (0,1)");
}

void validate_positive_steps_(std::int64_t steps, std::string_view op) {
  PLUSS_CHECK(steps > 0, std::string(op) + ": quadrature_steps must be > 0");
}

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

Series scalar_nonnegative_on_(double value,
                              pluss::codem::Platform platform,
                              std::string_view op,
                              std::string_view name) {
  validate_nonnegative_scalar_(value, op, name);
  return scalar_on_(value, platform);
}

template <typename Dist>
AggregateMoments moments_from_cumulants_(const Dist& dist, pluss::codem::Platform platform) {
  AggregateMoments out{};
  out.mean = dist.cumulant(1).to(platform, ScalarType::Float64).contiguous();
  out.variance = dist.cumulant(2).to(platform, ScalarType::Float64).contiguous();
  return out;
}

template <typename Dist>
Series limited_expected_value_by_survival_(const Dist& dist,
                                           double cap,
                                           std::int64_t quadrature_steps,
                                           pluss::codem::Platform platform,
                                           std::string_view op) {
  validate_nonnegative_scalar_(cap, op, "cap");
  validate_positive_steps_(quadrature_steps, op);
  if (cap == 0.0) {
    return scalar_on_(0.0, platform);
  }
  const double h = cap / static_cast<double>(quadrature_steps);
  const Series idx = Series::arange(0, quadrature_steps, 1, ScalarType::Int64).to(platform, ScalarType::Float64);
  const Series x_mid = idx.add(0.5).mul(h);
  const Series surv = dist.cdf(x_mid).mul(-1.0).add(1.0).maximum(0.0);
  return surv.sum().mul(h);
}

} // namespace

AggregateMoments poisson_frequency_moments(double lambda, pluss::codem::Platform platform) {
  validate_nonnegative_scalar_(lambda, "poisson_frequency_moments", "lambda");
  const pluss::subsp::stats::distr::Poisson freq(scalar_on_(lambda, platform));
  return moments_from_cumulants_(freq, platform);
}

AggregateMoments negative_binomial_frequency_moments(double r, double p, pluss::codem::Platform platform) {
  validate_positive_scalar_(r, "negative_binomial_frequency_moments", "r");
  validate_probability_open_(p, "negative_binomial_frequency_moments", "p");
  const pluss::subsp::stats::distr::NegativeBinomial freq(scalar_on_(r, platform), scalar_on_(p, platform));
  return moments_from_cumulants_(freq, platform);
}

AggregateMoments lognormal_severity_moments(double mean_log,
                                            double stddev_log,
                                            pluss::codem::Platform platform) {
  validate_finite_scalar_(mean_log, "lognormal_severity_moments", "mean_log");
  validate_nonnegative_scalar_(stddev_log, "lognormal_severity_moments", "stddev_log");
  const pluss::subsp::stats::distr::Lognormal sev(
      scalar_on_(mean_log, platform),
      scalar_on_(stddev_log, platform));
  return moments_from_cumulants_(sev, platform);
}

AggregateMoments gamma_severity_moments(double shape,
                                        double scale,
                                        double location,
                                        pluss::codem::Platform platform) {
  validate_positive_scalar_(shape, "gamma_severity_moments", "shape");
  validate_positive_scalar_(scale, "gamma_severity_moments", "scale");
  validate_nonnegative_scalar_(location, "gamma_severity_moments", "location");
  const pluss::subsp::stats::distr::Gamma sev(
      scalar_on_(shape, platform),
      scalar_on_(scale, platform),
      scalar_on_(location, platform));
  return moments_from_cumulants_(sev, platform);
}

AggregateMoments aggregate_moments(const Series& frequency_mean,
                                   const Series& frequency_variance,
                                   const Series& severity_mean,
                                   const Series& severity_variance) {
  const Series m_n = sanitize_nonnegative_(frequency_mean, "aggregate_moments", "frequency_mean");
  const Series v_n = sanitize_nonnegative_(frequency_variance, "aggregate_moments", "frequency_variance");
  const Series m_x = sanitize_nonnegative_(severity_mean, "aggregate_moments", "severity_mean");
  const Series v_x = sanitize_nonnegative_(severity_variance, "aggregate_moments", "severity_variance");

  AggregateMoments out{};
  out.mean = m_n.mul(m_x);
  out.variance = m_n.mul(v_x).add(v_n.mul(m_x.square()));
  return out;
}

AggregateMoments compound_poisson_aggregate_moments(double lambda,
                                                    double severity_mean,
                                                    double severity_variance,
                                                    pluss::codem::Platform platform) {
  const AggregateMoments freq = poisson_frequency_moments(lambda, platform);
  const Series sev_mean = scalar_nonnegative_on_(
      severity_mean,
      platform,
      "compound_poisson_aggregate_moments",
      "severity_mean");
  const Series sev_variance = scalar_nonnegative_on_(
      severity_variance,
      platform,
      "compound_poisson_aggregate_moments",
      "severity_variance");
  return aggregate_moments(freq.mean, freq.variance, sev_mean, sev_variance);
}

AggregateMoments compound_negative_binomial_aggregate_moments(double r,
                                                              double p,
                                                              double severity_mean,
                                                              double severity_variance,
                                                              pluss::codem::Platform platform) {
  const AggregateMoments freq = negative_binomial_frequency_moments(r, p, platform);
  const Series sev_mean = scalar_nonnegative_on_(
      severity_mean,
      platform,
      "compound_negative_binomial_aggregate_moments",
      "severity_mean");
  const Series sev_variance = scalar_nonnegative_on_(
      severity_variance,
      platform,
      "compound_negative_binomial_aggregate_moments",
      "severity_variance");
  return aggregate_moments(freq.mean, freq.variance, sev_mean, sev_variance);
}

Series poisson_frequency_pmf(double lambda, std::int64_t max_count, pluss::codem::Platform platform) {
  validate_nonnegative_scalar_(lambda, "poisson_frequency_pmf", "lambda");
  PLUSS_CHECK(max_count >= 0, "poisson_frequency_pmf: max_count must be >= 0");
  const Series k = Series::arange(0, max_count + 1, 1, ScalarType::Int64).to(platform, ScalarType::Float64);
  const pluss::subsp::stats::distr::Poisson freq(scalar_on_(lambda, platform));
  return freq.pdf(k).to(platform, ScalarType::Float64).contiguous();
}

Series negative_binomial_frequency_pmf(double r,
                                       double p,
                                       std::int64_t max_count,
                                       pluss::codem::Platform platform) {
  validate_positive_scalar_(r, "negative_binomial_frequency_pmf", "r");
  validate_probability_open_(p, "negative_binomial_frequency_pmf", "p");
  PLUSS_CHECK(max_count >= 0, "negative_binomial_frequency_pmf: max_count must be >= 0");
  const Series k = Series::arange(0, max_count + 1, 1, ScalarType::Int64).to(platform, ScalarType::Float64);
  const pluss::subsp::stats::distr::NegativeBinomial freq(
      scalar_on_(r, platform),
      scalar_on_(p, platform));
  return freq.pdf(k).to(platform, ScalarType::Float64).contiguous();
}

Series excess_loss(const Series& losses, double attachment) {
  validate_nonnegative_scalar_(attachment, "excess_loss", "attachment");
  const Series x = sanitize_nonnegative_(losses, "excess_loss", "losses", /*require_1d=*/true);
  return x.sub(attachment).maximum(0.0);
}

Series limited_excess_loss(const Series& losses, double attachment, double limit) {
  validate_nonnegative_scalar_(limit, "limited_excess_loss", "limit");
  return excess_loss(losses, attachment).minimum(limit);
}

Series layer_premium(const Series& losses, double attachment, double limit) {
  const Series layer = limited_excess_loss(losses, attachment, limit);
  if (layer.numel() == 0) {
    return scalar_on_(0.0, layer.platform());
  }
  return layer.mean();
}

Series lognormal_limited_expected_value(double mean_log,
                                        double stddev_log,
                                        double cap,
                                        std::int64_t quadrature_steps,
                                        pluss::codem::Platform platform) {
  validate_finite_scalar_(mean_log, "lognormal_limited_expected_value", "mean_log");
  validate_nonnegative_scalar_(stddev_log, "lognormal_limited_expected_value", "stddev_log");
  const pluss::subsp::stats::distr::Lognormal sev(
      scalar_on_(mean_log, platform),
      scalar_on_(stddev_log, platform));
  return limited_expected_value_by_survival_(
      sev,
      cap,
      quadrature_steps,
      platform,
      "lognormal_limited_expected_value");
}

Series gamma_limited_expected_value(double shape,
                                    double scale,
                                    double location,
                                    double cap,
                                    std::int64_t quadrature_steps,
                                    pluss::codem::Platform platform) {
  validate_positive_scalar_(shape, "gamma_limited_expected_value", "shape");
  validate_positive_scalar_(scale, "gamma_limited_expected_value", "scale");
  validate_nonnegative_scalar_(location, "gamma_limited_expected_value", "location");
  const pluss::subsp::stats::distr::Gamma sev(
      scalar_on_(shape, platform),
      scalar_on_(scale, platform),
      scalar_on_(location, platform));
  return limited_expected_value_by_survival_(
      sev,
      cap,
      quadrature_steps,
      platform,
      "gamma_limited_expected_value");
}

Series lognormal_layer_premium(double mean_log,
                               double stddev_log,
                               double attachment,
                               double limit,
                               std::int64_t quadrature_steps,
                               pluss::codem::Platform platform) {
  validate_nonnegative_scalar_(attachment, "lognormal_layer_premium", "attachment");
  validate_nonnegative_scalar_(limit, "lognormal_layer_premium", "limit");
  return lognormal_limited_expected_value(mean_log, stddev_log, attachment + limit, quadrature_steps, platform)
      .sub(lognormal_limited_expected_value(mean_log, stddev_log, attachment, quadrature_steps, platform));
}

Series gamma_layer_premium(double shape,
                           double scale,
                           double location,
                           double attachment,
                           double limit,
                           std::int64_t quadrature_steps,
                           pluss::codem::Platform platform) {
  validate_nonnegative_scalar_(attachment, "gamma_layer_premium", "attachment");
  validate_nonnegative_scalar_(limit, "gamma_layer_premium", "limit");
  return gamma_limited_expected_value(shape, scale, location, attachment + limit, quadrature_steps, platform)
      .sub(gamma_limited_expected_value(shape, scale, location, attachment, quadrature_steps, platform));
}

Series stop_loss_premium(const Series& losses, double deductible) {
  const Series ex = excess_loss(losses, deductible);
  if (ex.numel() == 0) {
    return scalar_on_(0.0, ex.platform());
  }
  return ex.mean();
}

} // namespace pluss::subsp::actuarial::loss_models
