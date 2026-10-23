#include "pluss/subsp/stats/regression.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/stats/distr.h"
#include "pluss/subsp/stats/tails.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <numbers>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace pluss::subsp::stats::regression {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kEps = 1e-14;
constexpr double kMinInfo = 1e-12;

Config f64_cfg_like_(const Series& like, Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(like.platform()).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& y, const char* op) {
  PLUSS_CHECK(y.defined(), std::string(op) + ": expected defined response");
  const auto s = y.sizes();
  PLUSS_CHECK(!s.empty(), std::string(op) + ": expected non-scalar response");
  if (s.size() == 1) {
    return y.reshape({s[0]});
  }
  PLUSS_CHECK(s.size() == 2 && (s[0] == 1 || s[1] == 1),
              std::string(op) + ": expected response as vector or Nx1/1xN matrix");
  return y.reshape({y.numel()});
}

Series as_matrix_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": expected defined design matrix");
  const auto s = x.sizes();
  PLUSS_CHECK(!s.empty(), std::string(op) + ": expected non-scalar design matrix");
  if (s.size() == 1) {
    return x.reshape({s[0], 1});
  }
  PLUSS_CHECK(s.size() == 2, std::string(op) + ": expected design matrix to be 1D or 2D");
  if (s[0] == 1 && s[1] > 1) {
    return x.reshape({s[1], 1});
  }
  return x;
}

double scalar_(const Series& x) {
  return x.to(ScalarType::Float64).reshape({1}).item<double>();
}

double clamp01_(double p) {
  return std::clamp(p, 0.0, 1.0);
}

double norm_cdf_(double x, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, cfg);
  return clamp01_(pluss::subsp::stats::distr::Normal{0.0, 1.0}.cdf(q).item<double>());
}

double norm_quantile_(double p, Config cfg) {
  if (p <= 0.0) {
    return -std::numeric_limits<double>::infinity();
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = -32.0;
  double hi = 32.0;
  for (std::int64_t i = 0; i < 120; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (norm_cdf_(mid, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double t_cdf_(double x, double dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, cfg);
  return clamp01_(pluss::subsp::stats::distr::StudentT{dof, 0.0, 1.0}.cdf(q).item<double>());
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

void check_options_(const QuantileOptions& options, const char* op) {
  PLUSS_CHECK(std::isfinite(options.tau) && options.tau > 0.0 && options.tau < 1.0,
              std::string(op) + ": tau must be in (0,1)");
  PLUSS_CHECK(std::isfinite(options.alpha) && options.alpha > 0.0 && options.alpha < 1.0,
              std::string(op) + ": alpha must be in (0,1)");
  PLUSS_CHECK(std::isfinite(options.rcond) && options.rcond > 0.0, std::string(op) + ": rcond must be > 0");
  PLUSS_CHECK(options.max_iter > 0, std::string(op) + ": max_iter must be > 0");
  PLUSS_CHECK(std::isfinite(options.tol) && options.tol > 0.0, std::string(op) + ": tol must be > 0");
  PLUSS_CHECK(std::isfinite(options.eps) && options.eps > 0.0, std::string(op) + ": eps must be > 0");
}

std::string fmt_value_(double x, int precision = 4) {
  if (std::isnan(x)) {
    return "nan";
  }
  if (!std::isfinite(x)) {
    return (x > 0.0) ? "inf" : "-inf";
  }
  std::ostringstream oss;
  const double ax = std::abs(x);
  if ((ax != 0.0 && (ax < 1e-3 || ax >= 1e5))) {
    oss << std::scientific << std::setprecision(precision) << x;
  } else {
    oss << std::fixed << std::setprecision(precision) << x;
  }
  return oss.str();
}

std::string fmt_short_(double x) {
  if (std::isnan(x)) {
    return "nan";
  }
  if (!std::isfinite(x)) {
    return (x > 0.0) ? "inf" : "-inf";
  }
  std::ostringstream oss;
  oss << std::setprecision(4) << x;
  return oss.str();
}

struct CleanData {
  Series y = pluss::series::undefined();
  Series x = pluss::series::undefined();
  std::int64_t n = 0;
  std::int64_t p = 0;
  std::int64_t dropped = 0;
};

CleanData sanitize_inputs_(const Series& y, const Series& x, bool drop_missing, Config cfg, const char* op) {
  const auto out_cfg = f64_cfg_like_(x, cfg);
  const Series yv = as_vector_(y, op).to(out_cfg.platform, out_cfg.dtype).contiguous();
  const Series xm = as_matrix_(x, op).to(out_cfg.platform, out_cfg.dtype).contiguous();

  const std::int64_t n = yv.numel();
  PLUSS_CHECK(xm.sizes()[0] == n, std::string(op) + ": row mismatch between y and X");
  const std::int64_t p = xm.sizes()[1];
  PLUSS_CHECK(n > 0 && p >= 0, std::string(op) + ": empty y or X");

  const Series row_ok = yv.isfinite().logical_and(xm.isfinite().all(1, false));
  const std::int64_t n_clean = static_cast<std::int64_t>(std::llround(scalar_(row_ok.to(ScalarType::Float64).sum())));
  PLUSS_CHECK(n_clean > 0, std::string(op) + ": no observations left after filtering missing values");
  const bool all_ok = scalar_(row_ok.all()) > 0.5;
  PLUSS_CHECK(all_ok || drop_missing, std::string(op) + ": non-finite values found and drop_missing=false");

  const Series idx = row_ok.nonzero().reshape({n_clean}).to(out_cfg.platform, ScalarType::Int64);
  return CleanData{
      .y = yv.index_select(0, idx),
      .x = xm.index_select(0, idx),
      .n = n_clean,
      .p = p,
      .dropped = n - n_clean,
  };
}

Series build_design_(const Series& x, bool add_intercept, Config cfg) {
  if (!add_intercept) {
    return x;
  }
  const Series ones = pluss::series::ones({x.sizes()[0], 1}, cfg);
  return ones.unite({x}, 1);
}

std::vector<std::string> build_coef_names_(const std::vector<std::string>& predictor_names,
                                           std::int64_t p,
                                           bool has_intercept,
                                           const char* op) {
  if (!predictor_names.empty()) {
    PLUSS_CHECK(static_cast<std::int64_t>(predictor_names.size()) == p,
                std::string(op) + ": predictor_names size must equal number of design columns before intercept handling");
  }
  std::vector<std::string> names;
  names.reserve(static_cast<std::size_t>(p + (has_intercept ? 1 : 0)));
  if (has_intercept) {
    names.emplace_back("const");
  }
  if (!predictor_names.empty()) {
    names.insert(names.end(), predictor_names.begin(), predictor_names.end());
  } else {
    for (std::int64_t j = 0; j < p; ++j) {
      names.push_back("x" + std::to_string(j + 1));
    }
  }
  return names;
}

double quantile_value_(const Series& values, double tau, Config cfg) {
  const std::int64_t n = values.numel();
  if (n <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const Series sorted = values.reshape({n}).sort(0, false, false).to(f64_cfg_like_(values, cfg).platform, ScalarType::Float64);
  if (tau <= 0.0) {
    return scalar_(sorted.select(0, 0).reshape({1}));
  }
  if (tau >= 1.0) {
    return scalar_(sorted.select(0, n - 1).reshape({1}));
  }
  const double pos = tau * static_cast<double>(n - 1);
  const std::int64_t lo = static_cast<std::int64_t>(std::floor(pos));
  const std::int64_t hi = static_cast<std::int64_t>(std::ceil(pos));
  const double vlo = scalar_(sorted.select(0, lo).reshape({1}));
  if (lo == hi) {
    return vlo;
  }
  const double vhi = scalar_(sorted.select(0, hi).reshape({1}));
  const double w = pos - static_cast<double>(lo);
  return (1.0 - w) * vlo + w * vhi;
}

double quantile_loss_(const Series& resid, double tau, Config cfg) {
  const Series r = resid.reshape({resid.numel(), 1}).to(f64_cfg_like_(resid, cfg).platform, ScalarType::Float64);
  const Series pos = r.ge(0.0).where(r.mul(tau), r.mul(tau - 1.0));
  return scalar_(pos.sum());
}

// Density of the residuals at zero, by an Epanechnikov kernel with the Hall-Sheather bandwidth (Koenker 2005; the estimator used by the
// standard quantile-regression packages). The bandwidth is scaled by min(sd(y), IQR(residuals) / 1.34), so a few wild observations do not
// inflate it and with it the standard errors, as a bandwidth based on the residual standard deviation would.
double density_at_zero_(const Series& resid, const Series& endog, double tau, Config cfg) {
  const std::int64_t n = resid.numel();
  if (n <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const Series r = resid.reshape({n}).to(f64_cfg_like_(resid, cfg).platform, ScalarType::Float64);
  const double dn = static_cast<double>(n);
  const double z = norm_quantile_(tau, cfg);
  const double phi = std::exp(-0.5 * z * z) / std::sqrt(2.0 * std::numbers::pi);
  const double hall_sheather = std::pow(dn, -1.0 / 3.0) * std::pow(norm_quantile_(0.975, cfg), 2.0 / 3.0) *
                               std::pow(1.5 * phi * phi / (2.0 * z * z + 1.0), 1.0 / 3.0);
  const double iqr = quantile_value_(r, 0.75, cfg) - quantile_value_(r, 0.25, cfg);
  const Series y = endog.reshape({n}).to(r.platform(), ScalarType::Float64);
  const double ymean = scalar_(y.mean());
  const double ysd = std::sqrt(std::max(0.0, scalar_(y.sub(ymean).mul(y.sub(ymean)).mean())));
  double bw = hall_sheather;
  while ((tau - bw <= 0.0 || tau + bw >= 1.0) && bw > 1e-12) {
    bw *= 0.5;  // keep tau +- bw inside (0, 1): for an extreme tau and a small sample the rule alone would leave the unit interval
  }
  const double scale = std::min(ysd, iqr / 1.34);
  double h = scale * (norm_quantile_(tau + bw, cfg) - norm_quantile_(tau - bw, cfg));
  if (!std::isfinite(h) || h < 1e-8) {
    h = 1e-8;
  }
  const Series u = r.div(h);
  const Series kernel = u.abs().le(1.0).where(u.mul(u).mul(-1.0).add(1.0).mul(0.75), pluss::series::zeros_like(u));
  return std::max(kMinInfo, scalar_(kernel.sum()) / (dn * h));
}

} // namespace

QuantileFitResult quantile_fit(const Series& y, const Series& x, const QuantileOptions& options, Config cfg) {
  const char* op = "subsp::stats::regression::quantile_fit";
  check_options_(options, op);

  const auto clean = sanitize_inputs_(y, x, options.drop_missing, cfg, op);
  PLUSS_CHECK(clean.p > 0 || options.add_intercept, std::string(op) + ": a model needs at least one predictor or an intercept");
  const auto out_cfg = f64_cfg_like_(clean.x, cfg);

  const Series X = build_design_(clean.x, options.add_intercept, out_cfg);
  const Series ycol = clean.y.reshape({clean.n, 1});
  const std::int64_t n = X.sizes()[0];
  const std::int64_t k = X.sizes()[1];
  PLUSS_CHECK(n >= k, std::string(op) + ": requires n >= number of parameters");

  Series beta = pluss::subsp::math::linalg::pinverse(X, options.rcond).matmul(ycol);
  Series fitted = X.matmul(beta);
  Series resid = ycol.sub(fitted);

  std::int64_t iters = 0;
  bool converged = false;
  std::vector<std::string> warnings;

  for (std::int64_t it = 0; it < options.max_iter; ++it) {
    iters = it + 1;
    const Series rcol = resid.reshape({n, 1});
    const Series base = rcol.ge(0.0).where(
        pluss::series::full({n, 1}, options.tau, out_cfg),
        pluss::series::full({n, 1}, 1.0 - options.tau, out_cfg));
    const Series wcol = base.div(rcol.abs().maximum(options.eps));
    const Series sw = wcol.sqrt();
    const Series Xw = X.mul(sw);
    const Series yw = ycol.mul(sw);
    const Series beta_new = pluss::subsp::math::linalg::pinverse(Xw, options.rcond).matmul(yw);

    const double max_abs_delta = scalar_(beta_new.sub(beta).abs().max());
    const double max_abs_beta = scalar_(beta_new.abs().max());

    beta = beta_new;
    fitted = X.matmul(beta);
    resid = ycol.sub(fitted);

    if (max_abs_delta <= options.tol * (1.0 + max_abs_beta)) {
      converged = true;
      break;
    }
  }
  if (!converged) {
    warnings.push_back("Quantile IRLS reached max_iter without satisfying tolerance.");
  }

  const Series resid_vec = resid.reshape({n});
  const double obj = quantile_loss_(resid_vec, options.tau, out_cfg);
  const double q0 = quantile_value_(ycol.reshape({n}), options.tau, out_cfg);
  const double obj0 = quantile_loss_(ycol.reshape({n}).sub(q0), options.tau, out_cfg);
  const double pseudo_r2 = (obj0 > kEps) ? (1.0 - obj / obj0) : std::numeric_limits<double>::quiet_NaN();

  const auto svd_x = pluss::subsp::math::linalg::svd(X, false);
  const Series sing = svd_x.S.reshape({svd_x.S.numel()}).to(out_cfg.platform, out_cfg.dtype);
  const double smax = scalar_(sing.max());
  const double smin = scalar_(sing.min());
  const double rank_tol = options.rcond * std::max<std::int64_t>(n, k) * std::max(smax, 1.0);
  const std::int64_t rank = static_cast<std::int64_t>(std::llround(scalar_(sing.gt(rank_tol).sum())));
  const std::int64_t df_resid = std::max<std::int64_t>(0, n - rank);
  const std::int64_t df_model = options.add_intercept ? std::max<std::int64_t>(0, rank - 1) : rank;

  const double f0 = density_at_zero_(resid_vec, ycol, options.tau, out_cfg);
  const Series pinv_x = pluss::subsp::math::linalg::pinverse(X, options.rcond);
  const Series xtx_inv = pinv_x.matmul(pinv_x.transpose(0, 1));
  Series cov = xtx_inv.mul(options.tau * (1.0 - options.tau) / std::max(kMinInfo, f0 * f0));
  if (options.covariance == QuantileCovariance::Robust) {
    // heteroskedasticity-robust sandwich: (X'X)^-1 X' D X (X'X)^-1 with D_i = (tau / f0)^2 above the line and ((1 - tau) / f0)^2 below
    const double up = options.tau / std::max(1e-6, f0);
    const double down = (1.0 - options.tau) / std::max(1e-6, f0);
    const Series d = resid.reshape({n, 1}).gt(0.0).where(
        pluss::series::full({n, 1}, up * up, out_cfg),
        pluss::series::full({n, 1}, down * down, out_cfg));
    const Series xtdx = X.transpose(0, 1).matmul(X.mul(d));
    cov = xtx_inv.matmul(xtdx).matmul(xtx_inv);
  }
  const Series std_err = cov.diagonal().reshape({k}).maximum(0.0).sqrt();

  const Series coef = beta.reshape({k});
  const Series t_v = coef.div(std_err.maximum(std::sqrt(kEps)));
  // Student t with the residual degrees of freedom, as for least squares
  Series p_v = pluss::series::full({k}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  double crit = std::numeric_limits<double>::quiet_NaN();
  if (df_resid > 0) {
    const double dof = static_cast<double>(df_resid);
    const auto tv = t_v.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::vector<double> pv(tv.size());
    for (std::size_t i = 0; i < tv.size(); ++i) {
      pv[i] = pluss::subsp::stats::tails::student_t_two_sided(tv[i], dof);
    }
    p_v = pluss::make_series(pv, {k}, out_cfg);
    crit = t_quantile_(1.0 - options.alpha * 0.5, dof, out_cfg);
  }
  const Series delta = std_err.mul(crit);
  const Series ci = coef.sub(delta).reshape({k, 1}).unite({coef.add(delta).reshape({k, 1})}, 1);

  if (rank < k) {
    warnings.push_back("The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse.");
  }

  QuantileFitResult out;
  out.coefficients = coef;
  out.std_errors = std_err;
  out.t_values = t_v;
  out.p_values = p_v;
  out.conf_int = ci;
  out.covariance = cov.to(out_cfg.platform, out_cfg.dtype);
  out.fitted_values = fitted.reshape({n});
  out.residuals = resid.reshape({n});

  out.nobs = n;
  out.rank = rank;
  out.k_params = k;
  out.num_predictors = clean.p;
  out.dropped_observations = clean.dropped;
  out.df_model = df_model;
  out.df_resid = df_resid;
  out.iterations = iters;

  out.tau = options.tau;
  out.alpha = options.alpha;
  out.objective = obj;
  out.objective_null = obj0;
  out.pseudo_rsquared = pseudo_r2;
  out.residual_density0 = f0;
  out.converged = converged;

  out.has_intercept = options.add_intercept;
  out.response_name = options.response_name;
  out.coefficient_names = build_coef_names_(options.predictor_names, clean.p, options.add_intercept, op);
  out.warnings = std::move(warnings);
  return out;
}

Series quantile_predict(const QuantileFitResult& fit, const Series& x, Config cfg) {
  const char* op = "subsp::stats::regression::quantile_predict";
  PLUSS_CHECK(fit.coefficients.defined(), std::string(op) + ": fit.coefficients is undefined");

  const Series xm = as_matrix_(x, op).to(fit.coefficients.platform(), ScalarType::Float64);
  const auto out_cfg = f64_cfg_like_(fit.coefficients, cfg);

  Series design = xm;
  const std::int64_t n = xm.sizes()[0];
  const std::int64_t cols = xm.sizes()[1];

  if (fit.has_intercept) {
    if (cols == fit.num_predictors) {
      const Series ones = pluss::series::ones({n, 1}, out_cfg);
      design = ones.unite({xm}, 1);
    } else {
      PLUSS_CHECK(cols == fit.k_params,
                  std::string(op) + ": expected X with either num_predictors columns or k_params columns");
    }
  } else {
    PLUSS_CHECK(cols == fit.num_predictors, std::string(op) + ": expected X with num_predictors columns");
  }

  const Series beta_col = fit.coefficients.reshape({fit.k_params, 1}).to(out_cfg.platform, out_cfg.dtype);
  return design.to(out_cfg.platform, out_cfg.dtype).matmul(beta_col).reshape({n});
}

std::string quantile_summary(const QuantileFitResult& fit) {
  std::ostringstream oss;
  print_quantile_summary(oss, fit);
  return oss.str();
}

std::ostream& print_quantile_summary(std::ostream& os, const QuantileFitResult& fit) {
  auto row = [&](const std::string& l1, const std::string& v1, const std::string& l2, const std::string& v2) {
    os << std::left << std::setw(18) << l1 << std::right << std::setw(22) << v1
       << "   "
       << std::left << std::setw(18) << l2 << std::right << std::setw(22) << v2 << '\n';
  };

  os << "                         Quantile Regression Results                          \n";
  os << "==============================================================================\n";
  row("Dep. Variable:", fit.response_name, "Pseudo R-squared:", fmt_short_(fit.pseudo_rsquared));
  row("Model:", "Quantile", "Tau:", fmt_short_(fit.tau));
  row("Method:", "IRLS", "No. Iterations:", std::to_string(fit.iterations));
  row("Rank:", std::to_string(fit.rank), "Converged:", fit.converged ? "true" : "false");
  row("Alpha:", fmt_short_(fit.alpha), "Objective:", fmt_short_(fit.objective));
  row("No. Observations:", std::to_string(fit.nobs), "Df Residuals:", std::to_string(fit.df_resid));
  row("Df Model:", std::to_string(fit.df_model), "Residual Density(0):", fmt_short_(fit.residual_density0));
  os << "==============================================================================\n";

  os << std::left << std::setw(16) << ""
     << std::right << std::setw(12) << "coef"
     << std::setw(12) << "std err"
     << std::setw(12) << "t"
     << std::setw(12) << "P>|t|"
     << std::setw(12) << "[0.025"
     << std::setw(12) << "0.975]"
     << '\n';
  os << "------------------------------------------------------------------------------\n";

  const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto se = fit.std_errors.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto tval = fit.t_values.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto pval = fit.p_values.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto ci = fit.conf_int.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

  for (std::size_t i = 0; i < coef.size(); ++i) {
    const std::string name = (i < fit.coefficient_names.size()) ? fit.coefficient_names[i] : ("x" + std::to_string(i + 1));
    os << std::left << std::setw(16) << name
       << std::right << std::setw(12) << fmt_value_(coef[i], 4)
       << std::setw(12) << fmt_value_(se[i], 4)
       << std::setw(12) << fmt_value_(tval[i], 4)
       << std::setw(12) << fmt_value_(pval[i], 4)
       << std::setw(12) << fmt_value_(ci[2 * i], 4)
       << std::setw(12) << fmt_value_(ci[2 * i + 1], 4)
       << '\n';
  }
  os << "==============================================================================\n";

  if (!fit.warnings.empty()) {
    os << "Warnings:\n";
    for (std::size_t i = 0; i < fit.warnings.size(); ++i) {
      os << "[" << (i + 1) << "] " << fit.warnings[i] << '\n';
    }
  }
  return os;
}

} // namespace pluss::subsp::stats::regression
