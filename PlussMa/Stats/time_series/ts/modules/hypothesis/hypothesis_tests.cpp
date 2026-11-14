#include "pluss/subsp/stats/ts.h"
#include "pluss/subsp/stats/tails.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/stats/distr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
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

double scalar_(const Series& x) {
  return x.to(ScalarType::Float64).reshape({1}).host<double>().front();
}

double clamp01_(double p) {
  return std::clamp(p, 0.0, 1.0);
}

double lerp_(double x0, double y0, double x1, double y1, double x) {
  if (std::abs(x1 - x0) <= kEps) {
    return 0.5 * (y0 + y1);
  }
  const double w = (x - x0) / (x1 - x0);
  return y0 + w * (y1 - y0);
}

template <std::size_t N>
double interp_piecewise_(const std::array<double, N>& xs, const std::array<double, N>& ys, double x) {
  if (x <= xs.front()) {
    return ys.front();
  }
  if (x >= xs.back()) {
    return ys.back();
  }
  for (std::size_t i = 0; i + 1 < N; ++i) {
    if (x <= xs[i + 1]) {
      return lerp_(xs[i], ys[i], xs[i + 1], ys[i + 1], x);
    }
  }
  return ys.back();
}

double chisq_cdf_(double value, std::int64_t dof, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series q = pluss::make_series(std::vector<double>{value}, {1}, out_cfg);
  return clamp01_(pluss::subsp::stats::distr::ChiSquare{dof}.cdf(q).host<double>().front());
}

double chisq_survival_(double value, std::int64_t dof, Config /*cfg*/) {
  return clamp01_(pluss::subsp::stats::tails::chi2_sf(value, static_cast<double>(dof)));
}

double chisq_quantile_(double prob, std::int64_t dof, Config cfg) {
  if (prob <= 0.0) {
    return 0.0;
  }
  if (prob >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = 0.0;
  double hi = std::max(1.0, static_cast<double>(dof) + 10.0 * std::sqrt(2.0 * static_cast<double>(dof)) + 10.0);
  for (std::int64_t i = 0; i < 64 && chisq_cdf_(hi, dof, cfg) < prob; ++i) {
    hi *= 2.0;
  }
  for (std::int64_t i = 0; i < 80; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (chisq_cdf_(mid, dof, cfg) < prob) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

struct OlsResult {
  Series coeff = pluss::series::undefined();
  Series residuals = pluss::series::undefined();
  Series xtx_inv = pluss::series::undefined();
  std::int64_t n = 0;
  std::int64_t k = 0;
  double mse = 0.0;
};

OlsResult ols_(const Series& x, const Series& y) {
  PLUSS_CHECK(x.dim() == 2, "subsp::stats::ts::hypothesis: OLS requires matrix design");
  PLUSS_CHECK(y.dim() == 2 && y.sizes()[1] == 1, "subsp::stats::ts::hypothesis: OLS requires column response");
  PLUSS_CHECK(x.sizes()[0] == y.sizes()[0], "subsp::stats::ts::hypothesis: OLS row mismatch");

  const std::int64_t n = x.sizes()[0];
  const std::int64_t k = x.sizes()[1];
  PLUSS_CHECK(n > k, "subsp::stats::ts::hypothesis: OLS requires n > k");

  const Series beta = pluss::subsp::math::linalg::pinverse(x, 1e-12).matmul(y);
  const Series residuals = y.sub(x.matmul(beta));
  const double sse = scalar_(residuals.mul(residuals).sum());
  const double mse = sse / static_cast<double>(n - k);
  const Series xtx_inv = pluss::subsp::math::linalg::pinverse(x.transpose(0, 1).matmul(x), 1e-12);

  return OlsResult{.coeff = beta, .residuals = residuals, .xtx_inv = xtx_inv, .n = n, .k = k, .mse = mse};
}

double coeff_se_(const OlsResult& fit, std::int64_t index, Config cfg) {
  PLUSS_CHECK(index >= 0 && index < fit.k, "subsp::stats::ts::hypothesis: coefficient index out of range");
  std::vector<double> e(static_cast<std::size_t>(fit.k), 0.0);
  e[static_cast<std::size_t>(index)] = 1.0;
  const Series basis = pluss::make_series(e, {fit.k, 1}, f64_cfg_(cfg));
  const double var = std::max(kEps, fit.mse * scalar_(basis.transpose(0, 1).matmul(fit.xtx_inv).matmul(basis)));
  return std::sqrt(var);
}

Series lagged_column_(const Series& x, std::int64_t start, std::int64_t length) {
  const Series idx = Series::arange(start, start + length, 1, ScalarType::Int64);
  return x.take(idx).reshape({length, 1});
}

// MacKinnon (1994, 2010) response surfaces for the Dickey-Fuller distribution with one series (N = 1). Regressions: no constant (AR),
// constant (ARD), constant and trend (TS).
struct MacKinnonSurface {
  double tau_max;                    // statistics above this have p-value 1
  double tau_min;                    // statistics below this have p-value 0
  double tau_star;                   // switch between the small-p and the large-p polynomial
  std::array<double, 3> small_p;     // p = Phi(c0 + c1 tau + c2 tau^2)         for tau <= tau_star
  std::array<double, 4> large_p;     // p = Phi(c0 + c1 tau + c2 tau^2 + c3 tau^3) for tau >  tau_star
  std::array<std::array<double, 4>, 3> crit;  // critical values at 1%, 5%, 10%: c0 + c1/T + c2/T^2 + c3/T^3 with T = observations
};

const MacKinnonSurface& mackinnon_surface_(AdfModel model) {
  static const MacKinnonSurface none{std::numeric_limits<double>::infinity(), -19.04, -1.04,
                                     {0.6344, 1.2378, 0.032496},
                                     {0.4797, 0.93557, -0.06999, 0.033066},
                                     {{{-2.56574, -2.2358, -3.627, 0.0}, {-1.941, -0.2686, -3.365, 31.223}, {-1.61682, 0.2656, -2.714, 25.364}}}};
  static const MacKinnonSurface constant{2.74, -18.83, -1.61,
                                         {2.1659, 1.4412, 0.038269},
                                         {1.7339, 0.93202, -0.12745, -0.010368},
                                         {{{-3.43035, -6.5393, -16.786, -79.433}, {-2.86154, -2.8903, -4.234, -40.04}, {-2.56677, -1.5384, -2.809, 0.0}}}};
  static const MacKinnonSurface trend{0.7, -16.18, -2.89,
                                      {3.2512, 1.6047, 0.049588},
                                      {2.5261, 0.61654, -0.37956, -0.060285},
                                      {{{-3.95877, -9.0531, -28.428, -134.155}, {-3.41049, -4.3904, -9.036, -45.374}, {-3.12705, -2.5856, -3.925, -22.38}}}};
  switch (model) {
    case AdfModel::AR:
      return none;
    case AdfModel::ARD:
      return constant;
    case AdfModel::TS:
      return trend;
  }
  return constant;
}

// Finite-sample critical values at 1%, 5% and 10% for `nobs` observations in the Dickey-Fuller regression.
std::array<double, 3> adf_critical_(AdfModel model, std::int64_t nobs) {
  const auto& surface = mackinnon_surface_(model);
  const double inv = 1.0 / static_cast<double>(std::max<std::int64_t>(1, nobs));
  std::array<double, 3> out{};
  for (std::size_t i = 0; i < 3; ++i) {
    const auto& c = surface.crit[i];
    out[i] = c[0] + inv * (c[1] + inv * (c[2] + inv * c[3]));
  }
  return out;
}

// MacKinnon's approximate p-value for the Dickey-Fuller t statistic.
double adf_pvalue_(double stat, AdfModel model) {
  const auto& surface = mackinnon_surface_(model);
  if (stat > surface.tau_max) {
    return 1.0;
  }
  if (stat < surface.tau_min) {
    return 0.0;
  }
  double z = 0.0;
  if (stat <= surface.tau_star) {
    z = surface.small_p[0] + stat * (surface.small_p[1] + stat * surface.small_p[2]);
  } else {
    z = surface.large_p[0] + stat * (surface.large_p[1] + stat * (surface.large_p[2] + stat * surface.large_p[3]));
  }
  return clamp01_(pluss::subsp::stats::tails::normal_sf(-z));
}

double kpss_pvalue_approx_(double stat, const std::array<double, 4>& criticals) {
  constexpr std::array<double, 4> sig = {0.010, 0.025, 0.050, 0.100};
  if (stat >= criticals[0]) {
    return sig[0];
  }
  if (stat <= criticals[3]) {
    return sig[3];
  }
  for (std::size_t i = 0; i + 1 < criticals.size(); ++i) {
    if (stat <= criticals[i] && stat >= criticals[i + 1]) {
      return lerp_(criticals[i], sig[i], criticals[i + 1], sig[i + 1], stat);
    }
  }
  return sig[3];
}

double normal_cdf_(double z, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{z}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::Normal{}.cdf(q).items<double>().front());
}

double normal_quantile_(double p, Config cfg) {
  if (p <= 0.0) {
    return -std::numeric_limits<double>::infinity();
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = -12.0;
  double hi = 12.0;
  for (std::int64_t i = 0; i < 120; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (normal_cdf_(mid, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double t_cdf_(double x, double dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::StudentT{dof, 0.0, 1.0}.cdf(q).host<double>().front());
}

double t_quantile_(double p, double dof, Config cfg) {
  if (p <= 0.0) {
    return -std::numeric_limits<double>::infinity();
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = -128.0;
  double hi = 128.0;
  for (std::int64_t i = 0; i < 140; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (t_cdf_(mid, dof, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double p_from_cdf_(double cdf, DmTail tail) {
  const double c = clamp01_(cdf);
  switch (tail) {
    case DmTail::Both:
      return clamp01_(2.0 * std::min(c, 1.0 - c));
    case DmTail::Left:
      return c;
    case DmTail::Right:
      return 1.0 - c;
  }
  return 1.0;
}

Series dm_loss_diff_(const Series& err_a, const Series& err_b, const DmOptions& options) {
  switch (options.loss) {
    case DmLoss::Squared:
      return err_a.square().sub(err_b.square());
    case DmLoss::Absolute:
      return err_a.abs().sub(err_b.abs());
    case DmLoss::Power:
      return err_a.abs().pow(options.loss_power).sub(err_b.abs().pow(options.loss_power));
  }
  return err_a.square().sub(err_b.square());
}

} // namespace

ArchLmResult arch_lm(const Series& x, const ArchLmOptions& options, Config cfg) {
  PLUSS_CHECK(options.lags > 0, "subsp::stats::ts::arch_lm: lags must be > 0");
  PLUSS_CHECK(options.alpha > 0.0 && options.alpha < 1.0,
              "subsp::stats::ts::arch_lm: alpha must be in (0,1)");

  const auto out_cfg = f64_cfg_(cfg);
  Series residuals = as_vector_(x, "arch_lm").to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(residuals.numel() >= 3, "subsp::stats::ts::arch_lm: requires at least 3 observations");
  if (options.demean) {
    residuals = residuals.sub(residuals.mean());
  }

  const std::int64_t n = residuals.numel();
  PLUSS_CHECK(options.lags <= n - 2,
              "subsp::stats::ts::arch_lm: lags must be <= number of observations - 2");
  const std::int64_t lags = options.lags;
  const std::int64_t t = n - lags;

  const Series sq = residuals.mul(residuals);
  const Series y = lagged_column_(sq, lags, t);

  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>(lags + 1));
  cols.push_back(pluss::series::ones({t, 1}, out_cfg));
  for (std::int64_t j = 1; j <= lags; ++j) {
    cols.push_back(lagged_column_(sq, lags - j, t));
  }
  const Series xreg = cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);

  const auto fit = ols_(xreg, y);
  const Series y_hat = xreg.matmul(fit.coeff);
  const Series yc = y.sub(y.mean());
  const Series yhc = y_hat.sub(y_hat.mean());

  const double denom = scalar_(yc.mul(yc).sum());
  const double r2 = (denom <= kEps) ? 0.0 : std::clamp(scalar_(yhc.mul(yhc).sum()) / denom, 0.0, 1.0);
  const double stat = std::max(0.0, static_cast<double>(t) * r2);
  const double pvalue = chisq_survival_(stat, lags, out_cfg);
  const double critical = chisq_quantile_(1.0 - options.alpha, lags, out_cfg);

  return ArchLmResult{
      .stat = stat,
      .pvalue = pvalue,
      .critical = critical,
      .reject = (pvalue < options.alpha),
      .dof = lags,
      .effective_n = t,
  };
}

AdfResult adf_test(const Series& y, const AdfOptions& options, Config cfg) {
  PLUSS_CHECK(options.lags >= 0, "subsp::stats::ts::adf_test: lags must be >= 0");
  PLUSS_CHECK(options.alpha > 0.0 && options.alpha < 1.0,
              "subsp::stats::ts::adf_test: alpha must be in (0,1)");

  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "adf_test").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t n = yv.numel();
  PLUSS_CHECK(n >= 4, "subsp::stats::ts::adf_test: requires at least 4 observations");
  PLUSS_CHECK(options.lags <= n - 3,
              "subsp::stats::ts::adf_test: lags must be <= number of observations - 3");

  const std::int64_t lags = options.lags;
  const Series y_prev = yv.take(Series::arange(0, n - 1, 1, ScalarType::Int64));
  const Series y_curr = yv.take(Series::arange(1, n, 1, ScalarType::Int64));
  const Series dy = y_curr.sub(y_prev);

  const std::int64_t t = n - 1 - lags;
  Series response = dy.take(Series::arange(lags, n - 1, 1, ScalarType::Int64)).reshape({t, 1});

  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>(lags + 3));
  std::int64_t delta_idx = 0;

  if (options.model == AdfModel::ARD || options.model == AdfModel::TS) {
    cols.push_back(pluss::series::ones({t, 1}, out_cfg));
    delta_idx = 1;
  }
  if (options.model == AdfModel::TS) {
    cols.push_back(Series::arange(1, t + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).reshape({t, 1}));
    delta_idx = 2;
  }

  cols.push_back(y_prev.take(Series::arange(lags, n - 1, 1, ScalarType::Int64)).reshape({t, 1}));

  for (std::int64_t i = 1; i <= lags; ++i) {
    cols.push_back(dy.take(Series::arange(lags - i, n - 1 - i, 1, ScalarType::Int64)).reshape({t, 1}));
  }

  const Series xreg = (cols.size() == 1) ? cols.front()
                                          : cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
  PLUSS_CHECK(t > xreg.sizes()[1], "subsp::stats::ts::adf_test: effective sample size too small for requested model");

  const auto fit = ols_(xreg, response);
  const double delta = fit.coeff.reshape({fit.k}).select(0, delta_idx).item<double>();
  const double se = coeff_se_(fit, delta_idx, out_cfg);
  const double stat = delta / std::max(se, std::sqrt(kEps));

  constexpr std::array<double, 3> sig = {0.01, 0.05, 0.10};
  const auto criticals = adf_critical_(options.model, t);
  const double alpha = std::clamp(options.alpha, sig.front(), sig.back());
  const double critical = interp_piecewise_(sig, criticals, alpha);
  const double pvalue = adf_pvalue_(stat, options.model);

  return AdfResult{
      .stat = stat,
      .pvalue = pvalue,
      .critical = critical,
      .reject = (stat < critical),
      .lags = lags,
      .effective_n = t,
  };
}

KpssResult kpss_test(const Series& y, const KpssOptions& options, Config cfg) {
  PLUSS_CHECK(options.lags >= 0, "subsp::stats::ts::kpss_test: lags must be >= 0");
  PLUSS_CHECK(options.alpha > 0.0 && options.alpha < 1.0,
              "subsp::stats::ts::kpss_test: alpha must be in (0,1)");

  const auto out_cfg = f64_cfg_(cfg);
  Series yv = as_vector_(y, "kpss_test").to(out_cfg.platform, out_cfg.dtype);
  yv = yv.masked_select(yv.isfinite());

  const std::int64_t n = yv.numel();
  PLUSS_CHECK(n >= 1, "subsp::stats::ts::kpss_test: no valid observations remain after filtering non-finite values");
  PLUSS_CHECK(!options.trend || n >= 2,
              "subsp::stats::ts::kpss_test: trend=true requires at least two finite observations");

  const std::int64_t lags = std::min<std::int64_t>(options.lags, std::max<std::int64_t>(0, n - 1));

  std::vector<Series> cols;
  cols.reserve(options.trend ? 2 : 1);
  cols.push_back(pluss::series::ones({n, 1}, out_cfg));
  if (options.trend) {
    cols.push_back(Series::arange(1, n + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).reshape({n, 1}));
  }
  const Series xreg = (cols.size() == 1) ? cols.front()
                                          : cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);

  const auto fit = ols_(xreg, yv.reshape({n, 1}));
  const Series resid = fit.residuals.reshape({n});

  std::vector<double> gamma(static_cast<std::size_t>(lags + 1), 0.0);
  for (std::int64_t j = 0; j <= lags; ++j) {
    const Series a = resid.take(Series::arange(j, n, 1, ScalarType::Int64));
    const Series b = resid.take(Series::arange(0, n - j, 1, ScalarType::Int64));
    gamma[static_cast<std::size_t>(j)] = scalar_(a.mul(b).sum()) / static_cast<double>(n);
  }

  double nw_sum = 0.0;
  for (std::int64_t j = 1; j <= lags; ++j) {
    const double w = 1.0 - static_cast<double>(j) / static_cast<double>(lags + 1);
    nw_sum += w * gamma[static_cast<std::size_t>(j)];
  }
  const double long_run_var = std::max(gamma.front() + 2.0 * nw_sum, kEps);

  const Series csum = resid.cumsum(0);
  const double stat = scalar_(csum.mul(csum).sum()) /
                      (long_run_var * static_cast<double>(n) * static_cast<double>(n));

  constexpr std::array<double, 4> sig = {0.010, 0.025, 0.050, 0.100};
  const std::array<double, 4> criticals = options.trend
                                              ? std::array<double, 4>{0.216, 0.176, 0.146, 0.119}
                                              : std::array<double, 4>{0.739, 0.574, 0.463, 0.347};
  const double alpha = std::clamp(options.alpha, sig.front(), sig.back());
  const double critical = interp_piecewise_(sig, criticals, alpha);
  const double pvalue = kpss_pvalue_approx_(stat, criticals);

  return KpssResult{
      .stat = stat,
      .pvalue = pvalue,
      .critical = critical,
      .reject = (stat > critical),
      .long_run_var = long_run_var,
      .lags = lags,
      .effective_n = n,
  };
}

DmResult dm_test(const Series& err_a, const Series& err_b, const DmOptions& options, Config cfg) {
  PLUSS_CHECK(options.horizon >= 1, "subsp::stats::ts::dm_test: horizon must be >= 1");
  PLUSS_CHECK(std::isfinite(options.alpha) && options.alpha > 0.0 && options.alpha < 1.0,
              "subsp::stats::ts::dm_test: alpha must be in (0,1)");
  if (options.loss == DmLoss::Power) {
    PLUSS_CHECK(std::isfinite(options.loss_power) && options.loss_power > 0.0,
                "subsp::stats::ts::dm_test: loss_power must be > 0 for DmLoss::Power");
  }

  const auto out_cfg = f64_cfg_(cfg);
  const Series a_raw = as_vector_(err_a, "dm_test").to(out_cfg.platform, out_cfg.dtype).reshape({err_a.numel()});
  const Series b_raw = as_vector_(err_b, "dm_test").to(out_cfg.platform, out_cfg.dtype).reshape({err_b.numel()});
  PLUSS_CHECK(a_raw.numel() == b_raw.numel(), "subsp::stats::ts::dm_test: inputs must have the same length");

  const Series finite = a_raw.isfinite().logical_and(b_raw.isfinite());
  const Series a = a_raw.masked_select(finite);
  const Series b = b_raw.masked_select(finite);
  const std::int64_t n = a.numel();
  PLUSS_CHECK(n >= 3, "subsp::stats::ts::dm_test: need at least 3 finite paired observations");
  PLUSS_CHECK(options.horizon <= n, "subsp::stats::ts::dm_test: horizon cannot exceed effective sample size");

  const std::int64_t requested_lags = (options.max_lag >= 0) ? options.max_lag : (options.horizon - 1);
  PLUSS_CHECK(requested_lags >= 0, "subsp::stats::ts::dm_test: max_lag must be >= -1");
  const std::int64_t lags = std::min<std::int64_t>(requested_lags, n - 1);

  const Series d = dm_loss_diff_(a, b, options);
  const double mean_diff = scalar_(d.mean());
  const Series centered = d.sub(mean_diff);

  const double gamma0 = scalar_(centered.mul(centered).sum()) / static_cast<double>(n);
  double acov_sum = 0.0;
  for (std::int64_t lag = 1; lag <= lags; ++lag) {
    const Series left = centered.take(Series::arange(lag, n, 1, ScalarType::Int64));
    const Series right = centered.take(Series::arange(0, n - lag, 1, ScalarType::Int64));
    const double gamma = scalar_(left.mul(right).sum()) / static_cast<double>(n);
    acov_sum += gamma;
  }

  const double lrv = gamma0 + 2.0 * acov_sum;
  PLUSS_CHECK(std::isfinite(lrv) && lrv > kEps,
              "subsp::stats::ts::dm_test: long-run variance is not positive; test is undefined");

  double stat = mean_diff / std::sqrt(lrv / static_cast<double>(n));
  if (options.hln_adjust) {
    const double h = static_cast<double>(options.horizon);
    const double nd = static_cast<double>(n);
    const double ratio = (nd + 1.0 - 2.0 * h + h * (h - 1.0) / nd) / nd;
    PLUSS_CHECK(ratio > 0.0,
                "subsp::stats::ts::dm_test: HLN adjustment undefined for current horizon and sample size");
    stat *= std::sqrt(ratio);
  }

  const double dof = static_cast<double>(n - 1);
  const double cdf = options.hln_adjust ? t_cdf_(stat, dof, out_cfg) : normal_cdf_(stat, out_cfg);
  const double pvalue = p_from_cdf_(cdf, options.tail);

  double critical = 0.0;
  if (options.tail == DmTail::Both) {
    critical = options.hln_adjust ? t_quantile_(1.0 - options.alpha * 0.5, dof, out_cfg)
                                  : normal_quantile_(1.0 - options.alpha * 0.5, out_cfg);
  } else if (options.tail == DmTail::Right) {
    critical = options.hln_adjust ? t_quantile_(1.0 - options.alpha, dof, out_cfg)
                                  : normal_quantile_(1.0 - options.alpha, out_cfg);
  } else {
    critical = options.hln_adjust ? t_quantile_(options.alpha, dof, out_cfg)
                                  : normal_quantile_(options.alpha, out_cfg);
  }

  return DmResult{
      .stat = stat,
      .pvalue = pvalue,
      .critical = critical,
      .reject = (pvalue < options.alpha),
      .horizon = options.horizon,
      .lags = lags,
      .effective_n = n,
      .mean_diff = mean_diff,
      .lrv = lrv,
  };
}

} // namespace pluss::subsp::stats::ts
