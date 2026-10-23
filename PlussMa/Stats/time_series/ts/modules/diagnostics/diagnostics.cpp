#include "pluss/subsp/stats/ts.h"
#include "pluss/subsp/stats/tails.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/distr.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kEps = 1e-14;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config i64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::stats::ts::") + name + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

std::vector<double> centered_values_(const Series& x, bool demean) {
  auto values = x.to(ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
  PLUSS_CHECK(!values.empty(), "subsp::stats::ts::diagnostics: expected non-empty input");
  if (demean) {
    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    std::transform(values.begin(), values.end(), values.begin(), [mean](const double v) { return v - mean; });
  }
  return values;
}

std::int64_t resolve_max_lag_(std::int64_t requested, std::int64_t nobs, const char* name) {
  PLUSS_CHECK(requested >= 0, std::string("subsp::stats::ts::") + name + ": max_lag must be >= 0");
  PLUSS_CHECK(nobs >= 2, std::string("subsp::stats::ts::") + name + ": requires at least 2 observations");
  return std::min<std::int64_t>(requested, nobs - 1);
}

std::vector<double> acf_values_(const std::vector<double>& centered, std::int64_t max_lag, AcfNorm norm) {
  const std::int64_t n = static_cast<std::int64_t>(centered.size());
  std::vector<double> out(static_cast<std::size_t>(max_lag + 1), 0.0);
  out[0] = 1.0;
  const double gamma0_num = std::inner_product(centered.begin(), centered.end(), centered.begin(), 0.0);
  const double gamma0 = gamma0_num / static_cast<double>(n);
  if (std::abs(gamma0) <= kEps) {
    return out;
  }

  for (std::int64_t k = 1; k <= max_lag; ++k) {
    const double cross = std::inner_product(centered.begin() + k, centered.end(), centered.begin(), 0.0);
    const double denom = (norm == AcfNorm::Unbiased) ? static_cast<double>(n - k) : static_cast<double>(n);
    const double rho = (cross / denom) / gamma0;
    out[static_cast<std::size_t>(k)] = std::isfinite(rho) ? rho : std::numeric_limits<double>::quiet_NaN();
  }
  return out;
}

Series to_f64_series_(const std::vector<double>& values, Config cfg) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, f64_cfg_(cfg));
}

std::vector<std::int64_t> normalize_lags_(const std::vector<std::int64_t>& lags, std::int64_t nobs) {
  PLUSS_CHECK(!lags.empty(), "subsp::stats::ts::ljung_box: lags must be non-empty");
  std::vector<std::int64_t> out = lags;
  PLUSS_CHECK(std::all_of(out.begin(), out.end(), [](const std::int64_t lag) { return lag > 0; }),
              "subsp::stats::ts::ljung_box: all lags must be > 0");
  std::transform(out.begin(), out.end(), out.begin(), [nobs](const std::int64_t lag) {
    return std::min<std::int64_t>(lag, nobs - 1);
  });
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  PLUSS_CHECK(!out.empty(), "subsp::stats::ts::ljung_box: no valid lag remains after clipping");
  return out;
}

double chisq_survival_(double stat, std::int64_t dof, Config /*cfg*/) {
  return std::clamp(pluss::subsp::stats::tails::chi2_sf(stat, static_cast<double>(dof)), 0.0, 1.0);
}

} // namespace

Series acf(const Series& x, const AcfOptions& options, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series xv = as_vector_(x, "acf").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t n = xv.numel();
  const std::int64_t max_lag = resolve_max_lag_(options.max_lag, n, "acf");
  return to_f64_series_(acf_values_(centered_values_(xv, options.demean), max_lag, options.norm), out_cfg);
}

Series pacf(const Series& x, const PacfOptions& options, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series xv = as_vector_(x, "pacf").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t n = xv.numel();
  const std::int64_t max_lag = resolve_max_lag_(options.max_lag, n, "pacf");
  const auto rho = acf_values_(centered_values_(xv, options.demean), max_lag, options.norm);

  std::vector<double> out(static_cast<std::size_t>(max_lag + 1), 0.0);
  out[0] = 1.0;
  if (max_lag == 0) {
    return to_f64_series_(out, out_cfg);
  }

  std::vector<double> phi_prev(static_cast<std::size_t>(max_lag + 1), 0.0);
  std::vector<double> phi_curr(static_cast<std::size_t>(max_lag + 1), 0.0);
  double err = 1.0;

  for (std::int64_t k = 1; k <= max_lag; ++k) {
    double num = rho[static_cast<std::size_t>(k)];
    for (std::int64_t j = 1; j < k; ++j) {
      num -= phi_prev[static_cast<std::size_t>(j)] * rho[static_cast<std::size_t>(k - j)];
    }

    const double den = std::max(err, kEps);
    const double phi_kk = num / den;
    phi_curr[static_cast<std::size_t>(k)] = std::isfinite(phi_kk) ? phi_kk : 0.0;
    for (std::int64_t j = 1; j < k; ++j) {
      phi_curr[static_cast<std::size_t>(j)] =
          phi_prev[static_cast<std::size_t>(j)] -
          phi_curr[static_cast<std::size_t>(k)] * phi_prev[static_cast<std::size_t>(k - j)];
    }

    out[static_cast<std::size_t>(k)] = phi_curr[static_cast<std::size_t>(k)];
    err *= (1.0 - phi_curr[static_cast<std::size_t>(k)] * phi_curr[static_cast<std::size_t>(k)]);
    if (!std::isfinite(err) || err <= kEps) {
      err = kEps;
    }
    std::copy(phi_curr.begin() + 1, phi_curr.begin() + k + 1, phi_prev.begin() + 1);
  }

  return to_f64_series_(out, out_cfg);
}

LjungBoxResult ljung_box(const Series& x, const LjungBoxOptions& options, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto idx_cfg = i64_cfg_(cfg);
  const Series xv = as_vector_(x, "ljung_box").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t n = xv.numel();
  PLUSS_CHECK(n >= 2, "subsp::stats::ts::ljung_box: requires at least 2 observations");
  PLUSS_CHECK(options.model_df >= 0, "subsp::stats::ts::ljung_box: model_df must be >= 0");

  const auto lags = normalize_lags_(options.lags, n);
  const std::int64_t max_lag = lags.back();
  const auto rho = acf_values_(centered_values_(xv, options.demean), max_lag, options.norm);

  std::vector<double> prefix(static_cast<std::size_t>(max_lag + 1), 0.0);
  for (std::int64_t k = 1; k <= max_lag; ++k) {
    const double den = static_cast<double>(n - k);
    prefix[static_cast<std::size_t>(k)] =
        prefix[static_cast<std::size_t>(k - 1)] + (rho[static_cast<std::size_t>(k)] * rho[static_cast<std::size_t>(k)] / den);
  }

  const double scale = static_cast<double>(n) * static_cast<double>(n + 2);
  std::vector<double> stats;
  std::vector<double> pvals;
  std::vector<std::int64_t> dof;
  stats.reserve(lags.size());
  pvals.reserve(lags.size());
  dof.reserve(lags.size());

  for (const auto lag : lags) {
    const double q = std::max(0.0, scale * prefix[static_cast<std::size_t>(lag)]);
    const auto d = std::max<std::int64_t>(1, lag - options.model_df);
    stats.push_back(q);
    pvals.push_back(chisq_survival_(q, d, out_cfg));
    dof.push_back(d);
  }

  LjungBoxResult out;
  out.lags = pluss::make_series(lags, {static_cast<std::int64_t>(lags.size())}, idx_cfg);
  out.stat = pluss::make_series(stats, {static_cast<std::int64_t>(stats.size())}, out_cfg);
  out.pvalue = pluss::make_series(pvals, {static_cast<std::int64_t>(pvals.size())}, out_cfg);
  out.dof = pluss::make_series(dof, {static_cast<std::int64_t>(dof.size())}, idx_cfg);
  return out;
}

} // namespace pluss::subsp::stats::ts
