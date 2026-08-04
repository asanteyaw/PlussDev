#include "pluss/subsp/math/finance/vol_bridge.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using bridge::JointLikelihoodWeights;
using bridge::ReturnsMetrics;
using pricing::OptionVolMetrics;
using volatility::Garch11Params;
using volatility::Ngarch11Params;

constexpr double kLog2Pi = 1.83787706640934533908;

struct NgarchCore {
  Series variance = Series::undefined();
  Series mean = Series::undefined();
  Series standardized_innovation = Series::undefined();
  Series loglik_per_obs = Series::undefined();
};

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::") + op + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::math::finance::") + op + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::math::finance::") + op + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

Series scalar_index_(std::int64_t index, Config cfg) {
  const auto idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return pluss::make_series(std::vector<std::int64_t>{index}, {1}, idx_cfg);
}

Series clamp_floor_(const Series& value, double floor, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series floor_s = pluss::series::full({}, floor, out_cfg);
  return value.ge(floor_s).where(value, floor_s);
}

void validate_weights_(const JointLikelihoodWeights& weights) {
  PLUSS_CHECK(std::isfinite(weights.returns_weight),
              "subsp::math::finance::bridge::weighted_joint_loglik: returns_weight must be finite");
  PLUSS_CHECK(std::isfinite(weights.options_weight),
              "subsp::math::finance::bridge::weighted_joint_loglik: options_weight must be finite");
  PLUSS_CHECK(weights.returns_weight >= 0.0,
              "subsp::math::finance::bridge::weighted_joint_loglik: returns_weight must be >= 0");
  PLUSS_CHECK(weights.options_weight >= 0.0,
              "subsp::math::finance::bridge::weighted_joint_loglik: options_weight must be >= 0");
}

void validate_ngarch_params_(const Ngarch11Params& params, const char* op) {
  PLUSS_CHECK(std::isfinite(params.omega), std::string("subsp::math::finance::") + op + ": omega must be finite");
  PLUSS_CHECK(std::isfinite(params.alpha), std::string("subsp::math::finance::") + op + ": alpha must be finite");
  PLUSS_CHECK(std::isfinite(params.beta), std::string("subsp::math::finance::") + op + ": beta must be finite");
  PLUSS_CHECK(std::isfinite(params.gamma), std::string("subsp::math::finance::") + op + ": gamma must be finite");
  PLUSS_CHECK(std::isfinite(params.lambda), std::string("subsp::math::finance::") + op + ": lambda must be finite");
  PLUSS_CHECK(std::isfinite(params.r), std::string("subsp::math::finance::") + op + ": r must be finite");
  PLUSS_CHECK(params.omega > 0.0, std::string("subsp::math::finance::") + op + ": omega must be > 0");
  PLUSS_CHECK(params.alpha >= 0.0, std::string("subsp::math::finance::") + op + ": alpha must be >= 0");
  PLUSS_CHECK(params.beta >= 0.0, std::string("subsp::math::finance::") + op + ": beta must be >= 0");
  PLUSS_CHECK(params.variance_floor > 0.0,
              std::string("subsp::math::finance::") + op + ": variance_floor must be > 0");
  PLUSS_CHECK(params.stationarity_margin > 0.0 && params.stationarity_margin < 1.0,
              std::string("subsp::math::finance::") + op + ": stationarity_margin must be in (0,1)");
  const double persistence = params.beta + params.alpha * (1.0 + params.gamma * params.gamma);
  PLUSS_CHECK(persistence < (1.0 - params.stationarity_margin),
              std::string("subsp::math::finance::") + op +
                  ": requires beta + alpha*(1+gamma^2) < 1 - stationarity_margin");
}

double unconditional_variance_ngarch_(const Ngarch11Params& params, const char* op) {
  validate_ngarch_params_(params, op);
  const double persistence = params.beta + params.alpha * (1.0 + params.gamma * params.gamma);
  const double denom = 1.0 - persistence;
  PLUSS_CHECK(denom > 0.0,
              std::string("subsp::math::finance::") + op +
                  ": unconditional variance undefined (non-stationary persistence)");
  const double unc = params.omega / denom;
  PLUSS_CHECK(std::isfinite(unc) && unc > 0.0,
              std::string("subsp::math::finance::") + op + ": unconditional variance must be finite and > 0");
  return std::max(unc, params.variance_floor);
}

NgarchCore ngarch_core_(const Series& returns, const Ngarch11Params& params, Config cfg, const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series y = as_vector_(returns, op).to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t n = y.numel();
  PLUSS_CHECK(n > 0, std::string("subsp::math::finance::") + op + ": expected at least one observation");

  const double h0 = unconditional_variance_ngarch_(params, op);
  Series variance = pluss::series::full({n}, h0, out_cfg);
  Series mean = pluss::series::zeros({n}, out_cfg);
  Series z = pluss::series::zeros({n}, out_cfg);
  Series ll = pluss::series::zeros({n}, out_cfg);

  for (std::int64_t t = 0; t < n; ++t) {
    const Series idx_t = scalar_index_(t, out_cfg);
    const Series ht = variance.select(0, t);
    const Series sig_t = ht.sqrt();
    const Series mu_t = sig_t.mul(params.lambda).add(params.r).add(ht.mul(0.5));
    const Series z_t = y.select(0, t).sub(mu_t).div(sig_t);
    const Series ll_t = z_t.square().add(ht.log()).add(kLog2Pi).mul(-0.5);

    mean = pluss::index_put(mean, std::vector<Series>{idx_t}, mu_t.reshape({1}), false);
    z = pluss::index_put(z, std::vector<Series>{idx_t}, z_t.reshape({1}), false);
    ll = pluss::index_put(ll, std::vector<Series>{idx_t}, ll_t.reshape({1}), false);

    if (t + 1 < n) {
      const Series h_next = ht.mul(params.beta)
                                .add(params.omega)
                                .add(ht.mul(params.alpha).mul(z_t.add(params.gamma).square()));
      const Series h_safe = clamp_floor_(h_next, params.variance_floor, out_cfg);
      const Series idx_next = scalar_index_(t + 1, out_cfg);
      variance = pluss::index_put(variance, std::vector<Series>{idx_next}, h_safe.reshape({1}), false);
    }
  }

  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::") + op + ": variance recursion produced non-finite values");
  PLUSS_CHECK(z.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::") + op + ": standardized innovations produced non-finite values");
  PLUSS_CHECK(ll.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::") + op + ": loglik_per_obs produced non-finite values");

  return NgarchCore{
      .variance = variance,
      .mean = mean,
      .standardized_innovation = z,
      .loglik_per_obs = ll,
  };
}

Series ngarch_forecast_(std::int64_t horizon,
                        const Series& returns,
                        const Ngarch11Params& params,
                        Config cfg,
                        const char* op) {
  PLUSS_CHECK(horizon > 0, std::string("subsp::math::finance::") + op + ": horizon must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const NgarchCore core = ngarch_core_(returns, params, out_cfg, op);
  const std::int64_t n = core.variance.numel();

  Series out = pluss::series::zeros({horizon}, out_cfg);
  Series h_prev = core.variance.select(0, n - 1);
  Series z_prev = core.standardized_innovation.select(0, n - 1);

  const double persistence = params.beta + params.alpha * (1.0 + params.gamma * params.gamma);
  for (std::int64_t h = 0; h < horizon; ++h) {
    Series h_next = Series::undefined();
    if (h == 0) {
      h_next = h_prev.mul(params.beta)
                   .add(params.omega)
                   .add(h_prev.mul(params.alpha).mul(z_prev.add(params.gamma).square()));
    } else {
      h_next = h_prev.mul(persistence).add(params.omega);
    }
    h_next = clamp_floor_(h_next, params.variance_floor, out_cfg);
    const Series idx = scalar_index_(h, out_cfg);
    out = pluss::index_put(out, std::vector<Series>{idx}, h_next.reshape({1}), false);
    h_prev = h_next;
  }
  PLUSS_CHECK(out.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::") + op + ": forecast produced non-finite values");
  return out;
}

template <typename Fit>
ReturnsMetrics returns_metrics_impl_(const Series& returns, const Fit& fit, Config cfg) {
  ReturnsMetrics out;
  out.loglik = fit.loglik(returns);
  out.loglik_per_obs = fit.loglik_per_obs(returns, cfg);
  out.variance = fit.infer(returns, cfg);
  return out;
}

template <typename Fit>
Series variance_forecast_impl_(std::int64_t horizon,
                               const Series& returns,
                               const Fit& fit,
                               Config cfg) {
  return fit.forecast(horizon, returns, cfg);
}

Ngarch11Params to_ngarch_params_(const Garch11Params& params) {
  return Ngarch11Params{
      .omega = params.omega,
      .alpha = params.alpha,
      .beta = params.beta,
      .gamma = 0.0,
      .lambda = params.lambda,
      .r = params.r,
      .variance_floor = params.variance_floor,
      .stationarity_margin = params.stationarity_margin,
  };
}

} // namespace

bridge::ReturnsMetrics bridge::returns_metrics(const Series& returns,
                                               const pluss::subsp::stats::ts::GarchFit& fit,
                                               Config cfg) {
  return returns_metrics_impl_(returns, fit, cfg);
}

bridge::ReturnsMetrics bridge::returns_metrics(const Series& returns,
                                               const pluss::subsp::stats::ts::EgarchFit& fit,
                                               Config cfg) {
  return returns_metrics_impl_(returns, fit, cfg);
}

bridge::ReturnsMetrics bridge::returns_metrics(const Series& returns,
                                               const pluss::subsp::stats::ts::GjrFit& fit,
                                               Config cfg) {
  return returns_metrics_impl_(returns, fit, cfg);
}

Series bridge::variance_forecast(std::int64_t horizon,
                                 const Series& returns,
                                 const pluss::subsp::stats::ts::GarchFit& fit,
                                 Config cfg) {
  return variance_forecast_impl_(horizon, returns, fit, cfg);
}

Series bridge::variance_forecast(std::int64_t horizon,
                                 const Series& returns,
                                 const pluss::subsp::stats::ts::EgarchFit& fit,
                                 Config cfg) {
  return variance_forecast_impl_(horizon, returns, fit, cfg);
}

Series bridge::variance_forecast(std::int64_t horizon,
                                 const Series& returns,
                                 const pluss::subsp::stats::ts::GjrFit& fit,
                                 Config cfg) {
  return variance_forecast_impl_(horizon, returns, fit, cfg);
}

double volatility::unconditional_variance(const Ngarch11Params& params) {
  return unconditional_variance_ngarch_(params, "unconditional_variance");
}

double volatility::unconditional_variance(const Garch11Params& params) {
  return unconditional_variance_ngarch_(to_ngarch_params_(params), "unconditional_variance");
}

Ngarch11Params volatility::to_pricing_params(const pluss::subsp::stats::ts::GarchFit& fit,
                                             double r,
                                             double lambda) {
  PLUSS_CHECK(fit.arch.size() == 1 && fit.garch.size() == 1,
              "subsp::math::finance::volatility::to_pricing_params: expected GarchFit with arch/garch size 1");
  return Ngarch11Params{
      .omega = fit.omega,
      .alpha = fit.arch[0],
      .beta = fit.garch[0],
      .gamma = 0.0,
      .lambda = lambda,
      .r = r,
      .variance_floor = fit.variance_floor,
      .stationarity_margin = fit.stationarity_margin,
  };
}

Ngarch11Params volatility::to_pricing_params(const pluss::subsp::stats::ts::Garch11Fit& fit,
                                             double r,
                                             double lambda) {
  return Ngarch11Params{
      .omega = fit.omega,
      .alpha = fit.alpha,
      .beta = fit.beta,
      .gamma = 0.0,
      .lambda = lambda,
      .r = r,
      .variance_floor = fit.variance_floor,
      .stationarity_margin = fit.stationarity_margin,
  };
}

OptionVolMetrics pricing::pricing_metrics(const Series& returns, const Ngarch11Params& params, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto core = ngarch_core_(returns, params, out_cfg, "pricing_metrics");
  return OptionVolMetrics{
      .loglik = core.loglik_per_obs.sum().item<double>(),
      .loglik_per_obs = core.loglik_per_obs.to(out_cfg.platform, out_cfg.dtype),
      .variance = core.variance.to(out_cfg.platform, out_cfg.dtype),
      .mean = core.mean.to(out_cfg.platform, out_cfg.dtype),
      .standardized_innovation = core.standardized_innovation.to(out_cfg.platform, out_cfg.dtype),
  };
}

OptionVolMetrics pricing::pricing_metrics(const Series& returns, const Garch11Params& params, Config cfg) {
  return pricing_metrics(returns, to_ngarch_params_(params), cfg);
}

Series pricing::pricing_variance_forecast(std::int64_t horizon,
                                          const Series& returns,
                                          const Ngarch11Params& params,
                                          Config cfg) {
  return ngarch_forecast_(horizon, returns, params, cfg, "pricing_variance_forecast");
}

Series pricing::pricing_variance_forecast(std::int64_t horizon,
                                          const Series& returns,
                                          const Garch11Params& params,
                                          Config cfg) {
  return pricing_variance_forecast(horizon, returns, to_ngarch_params_(params), cfg);
}

double bridge::weighted_joint_loglik(double returns_loglik,
                                     double options_loglik,
                                     const JointLikelihoodWeights& weights) {
  validate_weights_(weights);
  PLUSS_CHECK(std::isfinite(returns_loglik),
              "subsp::math::finance::bridge::weighted_joint_loglik: returns_loglik must be finite");
  PLUSS_CHECK(std::isfinite(options_loglik),
              "subsp::math::finance::bridge::weighted_joint_loglik: options_loglik must be finite");
  return weights.returns_weight * returns_loglik + weights.options_weight * options_loglik;
}

double bridge::weighted_joint_nll(double returns_loglik,
                                  double options_loglik,
                                  const JointLikelihoodWeights& weights) {
  return -weighted_joint_loglik(returns_loglik, options_loglik, weights);
}

} // namespace pluss::subsp::math::finance
