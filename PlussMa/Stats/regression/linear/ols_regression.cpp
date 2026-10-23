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

void check_alpha_(double alpha, const char* op) {
  PLUSS_CHECK(std::isfinite(alpha) && alpha > 0.0 && alpha < 1.0,
              std::string(op) + ": alpha must be in (0,1)");
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

double f_cdf_(double x, double dof1, double dof2, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, cfg);
  return clamp01_(pluss::subsp::stats::distr::FDistribution{dof1, dof2}.cdf(q).item<double>());
}

double chi2_cdf_(double x, std::int64_t dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, cfg);
  return clamp01_(pluss::subsp::stats::distr::ChiSquare{dof}.cdf(q).item<double>());
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

CleanData sanitize_inputs_(const Series& y, const Series& x, const OLSOptions& options, Config cfg) {
  const char* op = "subsp::stats::regression::ols_fit";
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
  PLUSS_CHECK(all_ok || options.drop_missing, std::string(op) + ": non-finite values found and drop_missing=false");

  const Series idx = row_ok.nonzero().reshape({n_clean}).to(out_cfg.platform, ScalarType::Int64);
  return CleanData{
      .y = yv.index_select(0, idx),
      .x = xm.index_select(0, idx),
      .n = n_clean,
      .p = p,
      .dropped = n - n_clean,
  };
}

std::vector<std::string> build_coef_names_(const OLSOptions& options,
                                           std::int64_t p,
                                           bool has_intercept,
                                           const char* op) {
  if (!options.predictor_names.empty()) {
    PLUSS_CHECK(static_cast<std::int64_t>(options.predictor_names.size()) == p,
                std::string(op) + ": predictor_names size must equal number of design columns before intercept handling");
  }

  std::vector<std::string> names;
  names.reserve(static_cast<std::size_t>(p + (has_intercept ? 1 : 0)));
  if (has_intercept) {
    names.emplace_back("const");
  }
  if (!options.predictor_names.empty()) {
    names.insert(names.end(), options.predictor_names.begin(), options.predictor_names.end());
  } else {
    for (std::int64_t j = 0; j < p; ++j) {
      names.push_back("x" + std::to_string(j + 1));
    }
  }
  return names;
}

Series build_design_(const Series& x, bool add_intercept, Config cfg) {
  if (!add_intercept) {
    return x;
  }
  const Series ones = pluss::series::ones({x.sizes()[0], 1}, cfg);
  return ones.unite({x}, 1);
}

} // namespace

OLSFitResult ols_fit(const Series& y, const Series& x, const OLSOptions& options, Config cfg) {
  const char* op = "subsp::stats::regression::ols_fit";
  check_alpha_(options.alpha, op);
  PLUSS_CHECK(std::isfinite(options.rcond) && options.rcond > 0.0, std::string(op) + ": rcond must be > 0");

  const auto clean = sanitize_inputs_(y, x, options, cfg);
  PLUSS_CHECK(clean.p > 0 || options.add_intercept, "ols_fit: a model needs at least one predictor or an intercept");
  const auto out_cfg = f64_cfg_like_(clean.x, cfg);

  const Series X = build_design_(clean.x, options.add_intercept, out_cfg);
  const Series ycol = clean.y.reshape({clean.n, 1});

  const std::int64_t n = X.sizes()[0];
  const std::int64_t k = X.sizes()[1];
  PLUSS_CHECK(n >= k, std::string(op) + ": requires n >= number of parameters");

  const auto svd_x = pluss::subsp::math::linalg::svd(X, false);
  const Series sing = svd_x.S.reshape({svd_x.S.numel()}).to(out_cfg.platform, out_cfg.dtype);
  const double smax = scalar_(sing.max());
  const double smin = scalar_(sing.min());
  const double rank_tol = options.rcond * std::max<std::int64_t>(n, k) * std::max(smax, 1.0);
  const std::int64_t rank = static_cast<std::int64_t>(std::llround(scalar_(sing.gt(rank_tol).sum())));

  const Series pinv_x = pluss::subsp::math::linalg::pinverse(X, options.rcond);
  const Series beta_col = pinv_x.matmul(ycol);
  const Series fitted_col = X.matmul(beta_col);
  const Series resid_col = ycol.sub(fitted_col);

  const double sse = scalar_(resid_col.mul(resid_col).sum());
  const double y_mean = scalar_(ycol.mean());
  const double sst = scalar_(ycol.sub(y_mean).mul(ycol.sub(y_mean)).sum());
  const double ssr = std::max(0.0, sst - sse);

  const std::int64_t df_resid = std::max<std::int64_t>(0, n - rank);
  const std::int64_t df_model = options.add_intercept ? std::max<std::int64_t>(0, rank - 1) : rank;

  const double mse = (df_resid > 0) ? (sse / static_cast<double>(df_resid)) : std::numeric_limits<double>::quiet_NaN();
  const double rmse = std::sqrt(std::max(0.0, mse));

  double rsq = std::numeric_limits<double>::quiet_NaN();
  if (sst > kEps) {
    rsq = 1.0 - sse / sst;
  } else if (sse <= kEps) {
    rsq = 1.0;
  }

  const double n_adj = options.add_intercept ? static_cast<double>(n - 1) : static_cast<double>(n);
  const double adj_rsq = (df_resid > 0 && std::isfinite(rsq))
                             ? (1.0 - (1.0 - rsq) * n_adj / static_cast<double>(df_resid))
                             : std::numeric_limits<double>::quiet_NaN();

  double f_stat = std::numeric_limits<double>::quiet_NaN();
  double f_pvalue = std::numeric_limits<double>::quiet_NaN();
  if (df_model > 0 && df_resid > 0 && mse > kEps) {
    f_stat = (ssr / static_cast<double>(df_model)) / mse;
    f_pvalue = pluss::subsp::stats::tails::f_sf(f_stat, static_cast<double>(df_model), static_cast<double>(df_resid));
    f_pvalue = clamp01_(f_pvalue);
  }

  double llf = std::numeric_limits<double>::quiet_NaN();
  double aic = std::numeric_limits<double>::quiet_NaN();
  double bic = std::numeric_limits<double>::quiet_NaN();
  if (n > 0 && sse >= 0.0) {
    if (sse <= kEps) {
      llf = std::numeric_limits<double>::infinity();
      aic = -std::numeric_limits<double>::infinity();
      bic = -std::numeric_limits<double>::infinity();
    } else {
      const double sigma2_mle = sse / static_cast<double>(n);
      llf = -0.5 * static_cast<double>(n) * (std::log(2.0 * std::numbers::pi * sigma2_mle) + 1.0);
      const double k_eff = static_cast<double>(std::max<std::int64_t>(1, rank));
      aic = -2.0 * llf + 2.0 * k_eff;
      bic = -2.0 * llf + std::log(static_cast<double>(n)) * k_eff;
    }
  }

  const Series xtx_pinv = pinv_x.matmul(pinv_x.transpose(0, 1));
  const Series cov = xtx_pinv.mul(mse);
  const Series coef = beta_col.reshape({k});
  const Series std_err = cov.diagonal().reshape({k}).maximum(0.0).sqrt();

  const double tcrit = (df_resid > 0)
                           ? t_quantile_(1.0 - options.alpha * 0.5, static_cast<double>(df_resid), out_cfg)
                           : std::numeric_limits<double>::quiet_NaN();
  const Series t_v = coef.div(std_err.maximum(std::sqrt(kEps)));
  Series p_v = pluss::series::full({k}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  if (df_resid > 0) {
    // Two-sided tail probability evaluated directly, so a very large t keeps a small nonzero p-value.
    const auto tv = t_v.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::vector<double> pv(tv.size());
    for (std::size_t i = 0; i < tv.size(); ++i) {
      pv[i] = pluss::subsp::stats::tails::student_t_two_sided(tv[i], static_cast<double>(df_resid));
    }
    p_v = pluss::make_series(pv, {k}, out_cfg);
  }
  Series ci_lo = pluss::series::full({k}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  Series ci_hi = pluss::series::full({k}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  if (std::isfinite(tcrit)) {
    const Series delta = std_err.mul(tcrit);
    ci_lo = coef.sub(delta);
    ci_hi = coef.add(delta);
  }
  const Series ci = ci_lo.reshape({k, 1}).unite({ci_hi.reshape({k, 1})}, 1);

  const Series resid_vec = resid_col.reshape({n});

  double durbin_watson = std::numeric_limits<double>::quiet_NaN();
  if (n > 1 && sse > kEps) {
    const Series idx0 = Series::arange(0, n - 1, 1, ScalarType::Int64).to(out_cfg.platform, ScalarType::Int64);
    const Series idx1 = Series::arange(1, n, 1, ScalarType::Int64).to(out_cfg.platform, ScalarType::Int64);
    const Series d = resid_vec.take(idx1).sub(resid_vec.take(idx0));
    durbin_watson = scalar_(d.mul(d).sum()) / sse;
  }

  const Series resid_center = resid_vec.sub(resid_vec.mean());
  const double m2 = scalar_(resid_center.mul(resid_center).mean());
  const double m3 = scalar_(resid_center.mul(resid_center).mul(resid_center).mean());
  const double m4 = scalar_(resid_center.mul(resid_center).mul(resid_center).mul(resid_center).mean());

  double skew = 0.0;
  double kurt = 3.0;
  double jb = std::numeric_limits<double>::quiet_NaN();
  double jb_p = std::numeric_limits<double>::quiet_NaN();
  if (m2 > kEps) {
    skew = m3 / std::pow(m2, 1.5);
    kurt = m4 / (m2 * m2);
    const double n_d = static_cast<double>(n);
    jb = n_d * (skew * skew + 0.25 * (kurt - 3.0) * (kurt - 3.0)) / 6.0;
    jb_p = clamp01_(pluss::subsp::stats::tails::chi2_sf(jb, 2.0));
  }

  // D'Agostino-Pearson omnibus test: K2 = Z(skewness)^2 + Z(kurtosis)^2, chi-squared with 2 degrees of freedom, using the
  // transformations of D'Agostino (1970) and Anscombe & Glynn (1983). Defined for n >= 8.
  double omnibus = std::numeric_limits<double>::quiet_NaN();
  double omnibus_p = std::numeric_limits<double>::quiet_NaN();
  if (m2 > kEps && n >= 8) {
    const double nd = static_cast<double>(n);
    const double y_s = skew * std::sqrt(((nd + 1.0) * (nd + 3.0)) / (6.0 * (nd - 2.0)));
    const double beta2 = 3.0 * (nd * nd + 27.0 * nd - 70.0) * (nd + 1.0) * (nd + 3.0) / ((nd - 2.0) * (nd + 5.0) * (nd + 7.0) * (nd + 9.0));
    const double w2 = -1.0 + std::sqrt(2.0 * (beta2 - 1.0));
    const double delta = 1.0 / std::sqrt(0.5 * std::log(w2));
    const double alpha_s = std::sqrt(2.0 / (w2 - 1.0));
    const double ys = (y_s == 0.0) ? 1.0 : y_s;
    const double z_skew = delta * std::log(ys / alpha_s + std::sqrt((ys / alpha_s) * (ys / alpha_s) + 1.0));

    const double expected = 3.0 * (nd - 1.0) / (nd + 1.0);
    const double var_b2 = 24.0 * nd * (nd - 2.0) * (nd - 3.0) / ((nd + 1.0) * (nd + 1.0) * (nd + 3.0) * (nd + 5.0));
    const double xk = (kurt - expected) / std::sqrt(var_b2);
    const double sqrt_beta1 = 6.0 * (nd * nd - 5.0 * nd + 2.0) / ((nd + 7.0) * (nd + 9.0)) *
                              std::sqrt(6.0 * (nd + 3.0) * (nd + 5.0) / (nd * (nd - 2.0) * (nd - 3.0)));
    const double a_k = 6.0 + 8.0 / sqrt_beta1 * (2.0 / sqrt_beta1 + std::sqrt(1.0 + 4.0 / (sqrt_beta1 * sqrt_beta1)));
    const double term1 = 1.0 - 2.0 / (9.0 * a_k);
    const double denom = 1.0 + xk * std::sqrt(2.0 / (a_k - 4.0));
    double z_kurt = std::numeric_limits<double>::quiet_NaN();
    if (denom != 0.0) {
      const double term2 = (denom > 0.0 ? 1.0 : -1.0) * std::cbrt((1.0 - 2.0 / a_k) / std::abs(denom));
      z_kurt = (term1 - term2) / std::sqrt(2.0 / (9.0 * a_k));
    }
    omnibus = z_skew * z_skew + z_kurt * z_kurt;
    if (std::isfinite(omnibus)) {
      omnibus_p = clamp01_(pluss::subsp::stats::tails::chi2_sf(omnibus, 2.0));
    }
  }

  const double cond_no = (smin > kEps) ? (smax / smin) : std::numeric_limits<double>::infinity();
  const double smallest_eig = smin * smin;

  std::vector<std::string> warnings;
  if (rank < k) {
    warnings.push_back("The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse.");
  }
  // Same rules as the usual statistical packages: a tiny smallest eigenvalue of X'X, else a condition number above 1000.
  if (smallest_eig < 1e-10) {
    std::ostringstream oss;
    oss << "The smallest eigenvalue is " << fmt_short_(smallest_eig)
        << ". This might indicate strong multicollinearity or a singular design matrix.";
    warnings.push_back(oss.str());
  } else if (cond_no > 1000.0) {
    std::ostringstream oss;
    oss << "The condition number is large, " << fmt_short_(cond_no)
        << ". This might indicate strong multicollinearity or other numerical problems.";
    warnings.push_back(oss.str());
  }
  if (!options.add_intercept) {
    warnings.push_back("Model has no explicit intercept; R-squared and F-statistic interpretations differ from intercept models.");
  }

  OLSFitResult out;
  out.coefficients = coef;
  out.std_errors = std_err;
  out.t_values = t_v;
  out.p_values = p_v;
  out.conf_int = ci;
  out.covariance = cov.to(out_cfg.platform, out_cfg.dtype);
  out.fitted_values = fitted_col.reshape({n});
  out.residuals = resid_vec;

  out.nobs = n;
  out.rank = rank;
  out.k_params = k;
  out.num_predictors = clean.p;
  out.dropped_observations = clean.dropped;
  out.df_model = df_model;
  out.df_resid = df_resid;

  out.alpha = options.alpha;
  out.mse = mse;
  out.rmse = rmse;
  out.sse = sse;
  out.ssr = ssr;
  out.sst = sst;
  out.rsquared = rsq;
  out.adj_rsquared = adj_rsq;
  out.f_stat = f_stat;
  out.f_pvalue = f_pvalue;
  out.log_likelihood = llf;
  out.aic = aic;
  out.bic = bic;
  out.durbin_watson = durbin_watson;
  out.omnibus = omnibus;
  out.omnibus_pvalue = omnibus_p;
  out.jarque_bera = jb;
  out.jarque_bera_pvalue = jb_p;
  out.skewness = skew;
  out.kurtosis = kurt;
  out.condition_number = cond_no;
  out.smallest_eig = smallest_eig;

  out.has_intercept = options.add_intercept;
  out.covariance_type = options.covariance;
  out.response_name = options.response_name;
  out.coefficient_names = build_coef_names_(options, clean.p, options.add_intercept, op);
  out.warnings = std::move(warnings);

  return out;
}

Series ols_predict(const OLSFitResult& fit, const Series& x, Config cfg) {
  const char* op = "subsp::stats::regression::ols_predict";
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
    PLUSS_CHECK(cols == fit.num_predictors,
                std::string(op) + ": expected X with num_predictors columns");
  }

  const Series beta_col = fit.coefficients.reshape({fit.k_params, 1}).to(out_cfg.platform, out_cfg.dtype);
  return design.to(out_cfg.platform, out_cfg.dtype).matmul(beta_col).reshape({n});
}

std::string ols_summary(const OLSFitResult& fit) {
  std::ostringstream oss;
  print_ols_summary(oss, fit);
  return oss.str();
}

std::ostream& print_ols_summary(std::ostream& os, const OLSFitResult& fit) {
  auto row = [&](const std::string& l1, const std::string& v1, const std::string& l2, const std::string& v2) {
    os << std::left << std::setw(18) << l1 << std::right << std::setw(22) << v1
       << "   "
       << std::left << std::setw(18) << l2 << std::right << std::setw(22) << v2 << '\n';
  };

  os << "                            OLS Regression Results                            \n";
  os << "==============================================================================\n";
  row("Dep. Variable:", fit.response_name, "R-squared:", fmt_short_(fit.rsquared));
  row("Model:", "OLS", "Adj. R-squared:", fmt_short_(fit.adj_rsquared));
  row("Method:", "Least Squares", "F-statistic:", fmt_short_(fit.f_stat));
  row("Rank:", std::to_string(fit.rank), "Prob (F-statistic):", fmt_short_(fit.f_pvalue));
  row("Alpha:", fmt_short_(fit.alpha), "Log-Likelihood:", fmt_short_(fit.log_likelihood));
  row("No. Observations:", std::to_string(fit.nobs), "AIC:", fmt_short_(fit.aic));
  row("Df Residuals:", std::to_string(fit.df_resid), "BIC:", fmt_short_(fit.bic));
  row("Df Model:", std::to_string(fit.df_model), "Covariance Type:", "nonrobust");
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
  row("Omnibus:", fmt_short_(fit.omnibus), "Durbin-Watson:", fmt_short_(fit.durbin_watson));
  row("Prob(Omnibus):", fmt_short_(fit.omnibus_pvalue), "Jarque-Bera (JB):", fmt_short_(fit.jarque_bera));
  row("Skew:", fmt_short_(fit.skewness), "Prob(JB):", fmt_short_(fit.jarque_bera_pvalue));
  row("Kurtosis:", fmt_short_(fit.kurtosis), "Cond. No.", fmt_short_(fit.condition_number));
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
