#pragma once

#include "pluss/subsp/stats/ts.h"

#include <cmath>
#include <limits>

namespace pluss::subsp::math::finance {

namespace volatility {

struct Ngarch11Params {
  double omega = 0.02;
  double alpha = 0.08;
  double beta = 0.90;
  double gamma = 0.0;
  double lambda = 0.0;
  double r = 0.0;
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
};

struct Garch11Params {
  double omega = 0.02;
  double alpha = 0.08;
  double beta = 0.90;
  double lambda = 0.0;
  double r = 0.0;
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
};

CODEM_API double unconditional_variance(const Ngarch11Params& params);
CODEM_API double unconditional_variance(const Garch11Params& params);

CODEM_API Ngarch11Params to_pricing_params(const pluss::subsp::stats::ts::GarchFit& fit,
                                           double r = 0.0,
                                           double lambda = 0.0);
CODEM_API Ngarch11Params to_pricing_params(const pluss::subsp::stats::ts::Garch11Fit& fit,
                                           double r = 0.0,
                                           double lambda = 0.0);

} // namespace volatility

namespace bridge {

struct JointLikelihoodWeights {
  double returns_weight = 1.0;
  double options_weight = 1.0;
};

struct ReturnsMetrics {
  double loglik = std::numeric_limits<double>::quiet_NaN();
  pluss::codem::Series loglik_per_obs = pluss::codem::Series::undefined();
  pluss::codem::Series variance = pluss::codem::Series::undefined();
};

CODEM_API ReturnsMetrics returns_metrics(const pluss::codem::Series& returns,
                                         const pluss::subsp::stats::ts::GarchFit& fit,
                                         pluss::codem::Config cfg = {});

CODEM_API ReturnsMetrics returns_metrics(const pluss::codem::Series& returns,
                                         const pluss::subsp::stats::ts::EgarchFit& fit,
                                         pluss::codem::Config cfg = {});

CODEM_API ReturnsMetrics returns_metrics(const pluss::codem::Series& returns,
                                         const pluss::subsp::stats::ts::GjrFit& fit,
                                         pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series variance_forecast(std::int64_t horizon,
                                                  const pluss::codem::Series& returns,
                                                  const pluss::subsp::stats::ts::GarchFit& fit,
                                                  pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series variance_forecast(std::int64_t horizon,
                                                  const pluss::codem::Series& returns,
                                                  const pluss::subsp::stats::ts::EgarchFit& fit,
                                                  pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series variance_forecast(std::int64_t horizon,
                                                  const pluss::codem::Series& returns,
                                                  const pluss::subsp::stats::ts::GjrFit& fit,
                                                  pluss::codem::Config cfg = {});

CODEM_API double weighted_joint_loglik(double returns_loglik,
                                       double options_loglik,
                                       const JointLikelihoodWeights& weights = {});

CODEM_API double weighted_joint_nll(double returns_loglik,
                                    double options_loglik,
                                    const JointLikelihoodWeights& weights = {});

} // namespace bridge

namespace pricing {

struct OptionVolMetrics {
  double loglik = std::numeric_limits<double>::quiet_NaN();
  pluss::codem::Series loglik_per_obs = pluss::codem::Series::undefined();
  pluss::codem::Series variance = pluss::codem::Series::undefined();
  pluss::codem::Series mean = pluss::codem::Series::undefined();
  pluss::codem::Series standardized_innovation = pluss::codem::Series::undefined();
};

CODEM_API OptionVolMetrics pricing_metrics(const pluss::codem::Series& returns,
                                           const volatility::Ngarch11Params& params,
                                           pluss::codem::Config cfg = {});

CODEM_API OptionVolMetrics pricing_metrics(const pluss::codem::Series& returns,
                                           const volatility::Garch11Params& params,
                                           pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series pricing_variance_forecast(std::int64_t horizon,
                                                          const pluss::codem::Series& returns,
                                                          const volatility::Ngarch11Params& params,
                                                          pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series pricing_variance_forecast(std::int64_t horizon,
                                                          const pluss::codem::Series& returns,
                                                          const volatility::Garch11Params& params,
                                                          pluss::codem::Config cfg = {});

} // namespace pricing

namespace vol = volatility;
namespace brg = bridge;
namespace prc = pricing;

} // namespace pluss::subsp::math::finance
