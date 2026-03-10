#pragma once

#include "pluss/subsp/stats/ts.h"

#include <cmath>
#include <limits>

namespace pluss::subsp::math::finance {

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

} // namespace pluss::subsp::math::finance
