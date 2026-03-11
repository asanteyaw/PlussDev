#include "ts_plotting.h"

#include "CodeM/core/Errors.h"
#include "pluss/pl/studio.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace pluss::subsp::stats::ts::plot {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::plot::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::plot::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::stats::ts::plot::") + name + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

// Acklam inverse-normal approximation for confidence bands.
double inv_norm_cdf_(double p) {
  constexpr std::array<double, 6> a = {
      -3.969683028665376e+01,
      2.209460984245205e+02,
      -2.759285104469687e+02,
      1.383577518672690e+02,
      -3.066479806614716e+01,
      2.506628277459239e+00,
  };
  constexpr std::array<double, 5> b = {
      -5.447609879822406e+01,
      1.615858368580409e+02,
      -1.556989798598866e+02,
      6.680131188771972e+01,
      -1.328068155288572e+01,
  };
  constexpr std::array<double, 6> c = {
      -7.784894002430293e-03,
      -3.223964580411365e-01,
      -2.400758277161838e+00,
      -2.549732539343734e+00,
      4.374664141464968e+00,
      2.938163982698783e+00,
  };
  constexpr std::array<double, 4> d = {
      7.784695709041462e-03,
      3.224671290700398e-01,
      2.445134137142996e+00,
      3.754408661907416e+00,
  };

  const double pp = std::clamp(p, 1e-12, 1.0 - 1e-12);
  constexpr double plow = 0.02425;
  constexpr double phigh = 1.0 - plow;

  if (pp < plow) {
    const double q = std::sqrt(-2.0 * std::log(pp));
    return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
           ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
  }
  if (pp > phigh) {
    const double q = std::sqrt(-2.0 * std::log(1.0 - pp));
    return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
           ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
  }

  const double q = pp - 0.5;
  const double r = q * q;
  return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
         (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
}

double z_for_confidence_(double confidence_level) {
  const double cl = std::clamp(confidence_level, 0.5, 0.999999);
  const double p = 0.5 + 0.5 * cl;
  return inv_norm_cdf_(p);
}

Series trim_zero_lag_(const Series& corr, bool include_zero_lag) {
  if (include_zero_lag || corr.numel() <= 1) {
    return corr;
  }
  return corr.take(Series::arange(1, corr.numel(), 1, ScalarType::Int64));
}

CorrPlotPayload corr_payload_(const Series& x,
                              const CorrPlotOptions& options,
                              bool partial,
                              Config cfg,
                              const char* op_name) {
  const auto out_cfg = f64_cfg_(cfg);
  Series xv = as_vector_(x, op_name).to(out_cfg.platform, out_cfg.dtype);
  xv = xv.masked_select(xv.isfinite());
  PLUSS_CHECK(xv.numel() >= 3, std::string("subsp::stats::ts::plot::") + op_name + ": requires at least 3 finite observations");

  const AcfOptions core_acf{.max_lag = options.max_lag, .norm = options.norm, .demean = options.demean};
  const PacfOptions core_pacf{.max_lag = options.max_lag, .norm = options.norm, .demean = options.demean};
  Series corr = partial ? ts::pacf(xv, core_pacf, out_cfg)
                        : ts::acf(xv, core_acf, out_cfg);
  corr = trim_zero_lag_(corr, options.include_zero_lag);

  const std::int64_t lag_start = options.include_zero_lag ? 0 : 1;
  const Series lags = Series::arange(lag_start,
                                     lag_start + corr.numel(),
                                     1,
                                     ScalarType::Int64)
                          .to(out_cfg.platform, out_cfg.dtype);

  const double conf = options.show_confidence_band
                          ? (z_for_confidence_(options.confidence_level) /
                             std::sqrt(static_cast<double>(xv.numel())))
                          : 0.0;

  return CorrPlotPayload{
      .lags = lags,
      .values = corr,
      .conf_low = -conf,
      .conf_high = conf,
  };
}

} // namespace

CorrPlotPayload acf_payload(const Series& x, const CorrPlotOptions& options, Config cfg) {
  return corr_payload_(x, options, false, cfg, "acf_payload");
}

CorrPlotPayload pacf_payload(const Series& x, const CorrPlotOptions& options, Config cfg) {
  return corr_payload_(x, options, true, cfg, "pacf_payload");
}

ResidualPlotPayload residuals_payload(const Series& residuals, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series res = as_vector_(residuals, "residuals_payload").to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(res.numel() > 0, "subsp::stats::ts::plot::residuals_payload: requires non-empty residual series");
  const Series idx = Series::arange(0, res.numel(), 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype);
  return ResidualPlotPayload{
      .index = idx,
      .residuals = res,
  };
}

pluss::pl::Studio& acf(pluss::pl::Studio& studio,
                       const Series& x,
                       const CorrPlotOptions& options,
                       Config cfg) {
  const auto payload = acf_payload(x, options, cfg);
  auto& pane = studio.pane2();
  pane.grid(pluss::pl::GridStyle::Major).x_label("lag").y_label("autocorrelation").y_limits({-1.0, 1.0});
  auto& spikes = pane.spike(payload.lags, payload.values, 0.0);
  spikes.width(options.trace_width).label("acf");
  pane.rule_y(0.0, "zero");
  if (options.show_confidence_band) {
    pane.rule_y(payload.conf_high, "+CI");
    pane.rule_y(payload.conf_low, "-CI");
  }
  return studio;
}

pluss::pl::Studio& pacf(pluss::pl::Studio& studio,
                        const Series& x,
                        const CorrPlotOptions& options,
                        Config cfg) {
  const auto payload = pacf_payload(x, options, cfg);
  auto& pane = studio.pane2();
  pane.grid(pluss::pl::GridStyle::Major).x_label("lag").y_label("partial autocorrelation").y_limits({-1.0, 1.0});
  auto& spikes = pane.spike(payload.lags, payload.values, 0.0);
  spikes.width(options.trace_width).label("pacf");
  pane.rule_y(0.0, "zero");
  if (options.show_confidence_band) {
    pane.rule_y(payload.conf_high, "+CI");
    pane.rule_y(payload.conf_low, "-CI");
  }
  return studio;
}

pluss::pl::Studio& residuals(pluss::pl::Studio& studio,
                             const Series& residuals,
                             const ResidualPlotOptions& options,
                             Config cfg) {
  const auto payload = residuals_payload(residuals, cfg);
  auto& pane = studio.pane2();
  pane.grid(pluss::pl::GridStyle::Major).x_label("t").y_label("residual");
  auto& trace = pane.trend(payload.index, payload.residuals);
  trace.width(options.trace_width).label("residuals");
  if (options.show_zero_line) {
    pane.rule_y(0.0, "zero");
  }
  return studio;
}

} // namespace pluss::subsp::stats::ts::plot
