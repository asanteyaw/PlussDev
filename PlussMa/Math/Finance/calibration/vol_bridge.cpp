#include "pluss/subsp/math/finance/vol_bridge.h"

#include "CodeM/core/Errors.h"

#include <cmath>
#include <string>

namespace pluss::subsp::math::finance {
namespace {

void validate_weights_(const JointLikelihoodWeights& weights) {
  PLUSS_CHECK(std::isfinite(weights.returns_weight),
              "subsp::math::finance::weighted_joint_loglik: returns_weight must be finite");
  PLUSS_CHECK(std::isfinite(weights.options_weight),
              "subsp::math::finance::weighted_joint_loglik: options_weight must be finite");
  PLUSS_CHECK(weights.returns_weight >= 0.0,
              "subsp::math::finance::weighted_joint_loglik: returns_weight must be >= 0");
  PLUSS_CHECK(weights.options_weight >= 0.0,
              "subsp::math::finance::weighted_joint_loglik: options_weight must be >= 0");
}

template <typename Fit>
ReturnsMetrics returns_metrics_impl_(const pluss::codem::Series& returns,
                                     const Fit& fit,
                                     pluss::codem::Config cfg) {
  ReturnsMetrics out;
  out.loglik = fit.loglik(returns);
  out.loglik_per_obs = fit.loglik_per_obs(returns, cfg);
  out.variance = fit.infer(returns, cfg);
  return out;
}

template <typename Fit>
pluss::codem::Series variance_forecast_impl_(std::int64_t horizon,
                                             const pluss::codem::Series& returns,
                                             const Fit& fit,
                                             pluss::codem::Config cfg) {
  return fit.forecast(horizon, returns, cfg);
}

} // namespace

ReturnsMetrics returns_metrics(const pluss::codem::Series& returns,
                               const pluss::subsp::stats::ts::GarchFit& fit,
                               pluss::codem::Config cfg) {
  return returns_metrics_impl_(returns, fit, cfg);
}

ReturnsMetrics returns_metrics(const pluss::codem::Series& returns,
                               const pluss::subsp::stats::ts::EgarchFit& fit,
                               pluss::codem::Config cfg) {
  return returns_metrics_impl_(returns, fit, cfg);
}

ReturnsMetrics returns_metrics(const pluss::codem::Series& returns,
                               const pluss::subsp::stats::ts::GjrFit& fit,
                               pluss::codem::Config cfg) {
  return returns_metrics_impl_(returns, fit, cfg);
}

pluss::codem::Series variance_forecast(std::int64_t horizon,
                                       const pluss::codem::Series& returns,
                                       const pluss::subsp::stats::ts::GarchFit& fit,
                                       pluss::codem::Config cfg) {
  return variance_forecast_impl_(horizon, returns, fit, cfg);
}

pluss::codem::Series variance_forecast(std::int64_t horizon,
                                       const pluss::codem::Series& returns,
                                       const pluss::subsp::stats::ts::EgarchFit& fit,
                                       pluss::codem::Config cfg) {
  return variance_forecast_impl_(horizon, returns, fit, cfg);
}

pluss::codem::Series variance_forecast(std::int64_t horizon,
                                       const pluss::codem::Series& returns,
                                       const pluss::subsp::stats::ts::GjrFit& fit,
                                       pluss::codem::Config cfg) {
  return variance_forecast_impl_(horizon, returns, fit, cfg);
}

double weighted_joint_loglik(double returns_loglik,
                             double options_loglik,
                             const JointLikelihoodWeights& weights) {
  validate_weights_(weights);
  PLUSS_CHECK(std::isfinite(returns_loglik),
              "subsp::math::finance::weighted_joint_loglik: returns_loglik must be finite");
  PLUSS_CHECK(std::isfinite(options_loglik),
              "subsp::math::finance::weighted_joint_loglik: options_loglik must be finite");
  return weights.returns_weight * returns_loglik + weights.options_weight * options_loglik;
}

double weighted_joint_nll(double returns_loglik,
                          double options_loglik,
                          const JointLikelihoodWeights& weights) {
  return -weighted_joint_loglik(returns_loglik, options_loglik, weights);
}

} // namespace pluss::subsp::math::finance
