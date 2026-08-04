#include "pluss/subsp/stats/regression.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/stats/distr.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

namespace pluss::subsp::stats::regression {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kEps = 1e-14;
constexpr double kMinMu = 1e-10;
constexpr double kMaxMu = 1.0 - 1e-10;

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

std::string now_date_() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%a, %d %b %Y");
  return oss.str();
}

std::string now_time_() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  return oss.str();
}

struct CleanData {
  Series y = Series::undefined();
  Series x = Series::undefined();
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
  PLUSS_CHECK(n > 0 && p > 0, std::string(op) + ": empty y or X");

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

std::string family_name_(GLMFamily f) {
  switch (f) {
    case GLMFamily::Normal:
      return "normal";
    case GLMFamily::Binomial:
      return "binomial";
    case GLMFamily::Poisson:
      return "poisson";
  }
  return "normal";
}

std::string link_name_(GLMLink l) {
  switch (l) {
    case GLMLink::Canonical:
      return "canonical";
    case GLMLink::Identity:
      return "identity";
    case GLMLink::Log:
      return "log";
    case GLMLink::Logit:
      return "logit";
  }
  return "identity";
}

GLMLink resolve_link_(GLMFamily family, GLMLink requested) {
  if (requested != GLMLink::Canonical) {
    return requested;
  }
  switch (family) {
    case GLMFamily::Normal:
      return GLMLink::Identity;
    case GLMFamily::Binomial:
      return GLMLink::Logit;
    case GLMFamily::Poisson:
      return GLMLink::Log;
  }
  return GLMLink::Identity;
}

Series inv_link_(const Series& eta, GLMLink link, Config cfg) {
  if (link == GLMLink::Identity) {
    return eta;
  }
  if (link == GLMLink::Log) {
    return eta.exp().maximum(kMinMu);
  }
  const Series one = pluss::series::ones(eta.sizes(), cfg);
  return one.div(one.add(eta.neg().exp())).maximum(kMinMu).minimum(kMaxMu);
}

Series link_derivative_mu_(const Series& mu_in, GLMLink link, Config cfg) {
  const Series one = pluss::series::ones(mu_in.sizes(), cfg);
  if (link == GLMLink::Identity) {
    return one;
  }
  if (link == GLMLink::Log) {
    return one.div(mu_in.maximum(kMinMu));
  }
  const Series mu = mu_in.maximum(kMinMu).minimum(kMaxMu);
  return one.div(mu.mul(one.sub(mu)).maximum(kMinMu));
}

Series variance_mu_(const Series& mu_in, GLMFamily family, Config cfg) {
  const Series one = pluss::series::ones(mu_in.sizes(), cfg);
  if (family == GLMFamily::Normal) {
    return one;
  }
  if (family == GLMFamily::Poisson) {
    return mu_in.maximum(kMinMu);
  }
  const Series mu = mu_in.maximum(kMinMu).minimum(kMaxMu);
  return mu.mul(one.sub(mu)).maximum(kMinMu);
}

double deviance_value_(GLMFamily family, const Series& y, const Series& mu) {
  if (family == GLMFamily::Normal) {
    const Series r = y.sub(mu);
    return scalar_(r.mul(r).sum());
  }
  if (family == GLMFamily::Poisson) {
    const Series yi = y.maximum(0.0);
    const Series mui = mu.maximum(kMinMu);
    const Series log_ratio = yi.maximum(kMinMu).div(mui).log();
    const Series core = yi.gt(0.0).where(yi.mul(log_ratio).sub(yi.sub(mui)), mui);
    return 2.0 * scalar_(core.sum());
  }
  const Series yi = y.clamp(kMinMu, kMaxMu);
  const Series mui = mu.clamp(kMinMu, kMaxMu);
  const Series one = pluss::series::ones(yi.sizes(), f64_cfg_like_(yi, {}));
  const Series term = yi.mul(yi.div(mui).log())
                          .add(one.sub(yi).mul(one.sub(yi).div(one.sub(mui)).log()));
  return 2.0 * scalar_(term.sum());
}

double loglik_value_(GLMFamily family, const Series& y, const Series& mu) {
  const std::int64_t n = y.numel();
  if (family == GLMFamily::Normal) {
    const Series r = y.sub(mu);
    const double sse = scalar_(r.mul(r).sum());
    if (sse <= kEps) {
      return std::numeric_limits<double>::infinity();
    }
    const double sigma2_mle = sse / static_cast<double>(n);
    return -0.5 * static_cast<double>(n) * (std::log(2.0 * std::numbers::pi * sigma2_mle) + 1.0);
  }
  if (family == GLMFamily::Poisson) {
    const Series yi = y.maximum(0.0);
    const Series mui = mu.maximum(kMinMu);
    return scalar_(yi.mul(mui.log()).sub(mui).sub(yi.add(1.0).lgamma()).sum());
  }
  const Series yi = y.clamp(kMinMu, kMaxMu);
  const Series mui = mu.clamp(kMinMu, kMaxMu);
  const Series one = pluss::series::ones(yi.sizes(), f64_cfg_like_(yi, {}));
  return scalar_(yi.mul(mui.log()).add(one.sub(yi).mul(one.sub(mui).log())).sum());
}

} // namespace

GLMFitResult glm_fit(const Series& y, const Series& x, const GLMOptions& options, Config cfg) {
  const char* op = "subsp::stats::regression::glm_fit";
  check_alpha_(options.alpha, op);
  PLUSS_CHECK(std::isfinite(options.rcond) && options.rcond > 0.0, std::string(op) + ": rcond must be > 0");
  PLUSS_CHECK(options.max_iter > 0, std::string(op) + ": max_iter must be > 0");
  PLUSS_CHECK(std::isfinite(options.tol) && options.tol > 0.0, std::string(op) + ": tol must be > 0");

  const auto clean = sanitize_inputs_(y, x, options.drop_missing, cfg, op);
  const auto out_cfg = f64_cfg_like_(clean.x, cfg);

  const GLMLink resolved_link = resolve_link_(options.family, options.link);
  if (options.family == GLMFamily::Binomial) {
    const bool valid = scalar_(clean.y.ge(0.0).logical_and(clean.y.le(1.0)).all()) > 0.5;
    PLUSS_CHECK(valid, std::string(op) + ": binomial family expects y in [0,1]");
  }
  if (options.family == GLMFamily::Poisson) {
    const bool valid = scalar_(clean.y.ge(0.0).all()) > 0.5;
    PLUSS_CHECK(valid, std::string(op) + ": poisson family expects y >= 0");
  }

  const Series X = build_design_(clean.x, options.add_intercept, out_cfg);
  const Series ycol = clean.y.reshape({clean.n, 1});
  const std::int64_t n = X.sizes()[0];
  const std::int64_t k = X.sizes()[1];
  PLUSS_CHECK(n >= k, std::string(op) + ": requires n >= number of parameters");

  Series beta = pluss::series::zeros({k, 1}, out_cfg);
  if (options.family == GLMFamily::Normal) {
    beta = pluss::subsp::math::linalg::pinverse(X, options.rcond).matmul(ycol);
  } else if (options.add_intercept && k > 0) {
    const double ymean = scalar_(ycol.mean());
    double b0 = 0.0;
    if (resolved_link == GLMLink::Log) {
      b0 = std::log(std::max(kMinMu, ymean));
    } else if (resolved_link == GLMLink::Logit) {
      const double ym = std::clamp(ymean, kMinMu, kMaxMu);
      b0 = std::log(ym / (1.0 - ym));
    } else {
      b0 = ymean;
    }
    const Series head = pluss::series::full({1, 1}, b0, out_cfg);
    if (k > 1) {
      beta = head.unite({pluss::series::zeros({k - 1, 1}, out_cfg)}, 0);
    } else {
      beta = head;
    }
  }

  std::int64_t iters = 0;
  bool converged = false;
  Series eta = X.matmul(beta);
  Series mu = inv_link_(eta, resolved_link, out_cfg);
  Series weights = pluss::series::ones({n, 1}, out_cfg);
  std::vector<std::string> warnings;

  for (std::int64_t it = 0; it < options.max_iter; ++it) {
    iters = it + 1;

    mu = inv_link_(eta, resolved_link, out_cfg);
    const Series var = variance_mu_(mu, options.family, out_cfg);
    const Series gprime = link_derivative_mu_(mu, resolved_link, out_cfg);
    const Series denom = var.mul(gprime).mul(gprime).maximum(kMinMu);
    const Series ones = pluss::series::ones(denom.sizes(), out_cfg);
    weights = ones.div(denom);

    const Series z = eta.add(ycol.sub(mu).mul(gprime));
    const Series sw = weights.sqrt();
    const Series Xw = X.mul(sw);
    const Series zw = z.mul(sw);
    const Series beta_new = pluss::subsp::math::linalg::pinverse(Xw, options.rcond).matmul(zw);

    const double max_abs_delta = scalar_(beta_new.sub(beta).abs().max());
    const double max_abs_beta = scalar_(beta_new.abs().max());

    beta = beta_new;
    eta = X.matmul(beta);
    if (max_abs_delta <= options.tol * (1.0 + max_abs_beta)) {
      converged = true;
      break;
    }
  }
  if (!converged) {
    warnings.push_back("IRLS reached max_iter without satisfying tolerance.");
  }

  mu = inv_link_(eta, resolved_link, out_cfg);
  const Series resid = ycol.sub(mu).reshape({n});

  const double deviance = deviance_value_(options.family, ycol.reshape({n}), mu.reshape({n}));
  const double llf = loglik_value_(options.family, ycol.reshape({n}), mu.reshape({n}));

  const double mean_y = scalar_(ycol.mean());
  Series mu_null = pluss::series::full({n, 1}, mean_y, out_cfg);
  if (options.family == GLMFamily::Binomial) {
    mu_null = mu_null.maximum(kMinMu).minimum(kMaxMu);
  } else if (options.family == GLMFamily::Poisson) {
    mu_null = mu_null.maximum(kMinMu);
  }
  const double null_deviance = deviance_value_(options.family, ycol.reshape({n}), mu_null.reshape({n}));
  const double pseudo_r2 =
      (std::isfinite(null_deviance) && null_deviance > kEps) ? (1.0 - deviance / null_deviance) : std::numeric_limits<double>::quiet_NaN();

  const auto svd_x = pluss::subsp::math::linalg::svd(X, false);
  const Series sing = svd_x.S.reshape({svd_x.S.numel()}).to(out_cfg.platform, out_cfg.dtype);
  const double smax = scalar_(sing.max());
  const double smin = scalar_(sing.min());
  const double rank_tol = options.rcond * std::max<std::int64_t>(n, k) * std::max(smax, 1.0);
  const std::int64_t rank = static_cast<std::int64_t>(std::llround(scalar_(sing.gt(rank_tol).sum())));
  const std::int64_t df_resid = std::max<std::int64_t>(0, n - rank);
  const std::int64_t df_model = options.add_intercept ? std::max<std::int64_t>(0, rank - 1) : rank;

  const Series var = variance_mu_(mu, options.family, out_cfg);
  const Series gprime = link_derivative_mu_(mu, resolved_link, out_cfg);
  const Series denom = var.mul(gprime).mul(gprime).maximum(kMinMu);
  const Series ones = pluss::series::ones(denom.sizes(), out_cfg);
  weights = ones.div(denom);

  const Series sw = weights.sqrt();
  const Series Xw = X.mul(sw);
  const Series pinv_xw = pluss::subsp::math::linalg::pinverse(Xw, options.rcond);
  const Series xtx_inv = pinv_xw.matmul(pinv_xw.transpose(0, 1));

  double dispersion = 1.0;
  if (options.family == GLMFamily::Normal) {
    dispersion = (df_resid > 0) ? (deviance / static_cast<double>(df_resid)) : std::numeric_limits<double>::quiet_NaN();
  }
  const Series cov = xtx_inv.mul(dispersion);
  const Series std_err = cov.diagonal().reshape({k}).maximum(0.0).sqrt();

  const Series coef = beta.reshape({k});
  const Series z_v = coef.div(std_err.maximum(std::sqrt(kEps)));
  const Series p_v = pluss::series::ones({k}, out_cfg)
                         .sub(pluss::subsp::stats::distr::Normal{0.0, 1.0}.cdf(z_v.abs()))
                         .mul(2.0)
                         .clamp(0.0, 1.0);
  const double zcrit = norm_quantile_(1.0 - options.alpha * 0.5, out_cfg);
  const Series delta = std_err.mul(zcrit);
  const Series ci = coef.sub(delta).reshape({k, 1}).unite({coef.add(delta).reshape({k, 1})}, 1);

  const double k_eff = static_cast<double>(std::max<std::int64_t>(1, rank));
  const double aic = -2.0 * llf + 2.0 * k_eff;
  const double bic = -2.0 * llf + std::log(static_cast<double>(n)) * k_eff;
  const double cond_no = (smin > kEps) ? (smax / smin) : std::numeric_limits<double>::infinity();

  if (rank < k) {
    warnings.push_back("The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse.");
  }

  GLMFitResult out;
  out.coefficients = coef;
  out.std_errors = std_err;
  out.z_values = z_v;
  out.p_values = p_v;
  out.conf_int = ci;
  out.covariance = cov.to(out_cfg.platform, out_cfg.dtype);
  out.fitted_values = mu.reshape({n});
  out.linear_predictor = eta.reshape({n});
  out.residuals = resid;

  out.nobs = n;
  out.rank = rank;
  out.k_params = k;
  out.num_predictors = clean.p;
  out.dropped_observations = clean.dropped;
  out.df_model = df_model;
  out.df_resid = df_resid;
  out.iterations = iters;

  out.alpha = options.alpha;
  out.dispersion = dispersion;
  out.deviance = deviance;
  out.null_deviance = null_deviance;
  out.log_likelihood = llf;
  out.aic = aic;
  out.bic = bic;
  out.pseudo_rsquared = pseudo_r2;
  out.condition_number = cond_no;
  out.converged = converged;

  out.has_intercept = options.add_intercept;
  out.family = options.family;
  out.link = resolved_link;
  out.response_name = options.response_name;
  out.coefficient_names = build_coef_names_(options.predictor_names, clean.p, options.add_intercept, op);
  out.warnings = std::move(warnings);
  return out;
}

Series glm_predict(const GLMFitResult& fit, const Series& x, GLMPredictionType type, Config cfg) {
  const char* op = "subsp::stats::regression::glm_predict";
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
  const Series eta = design.to(out_cfg.platform, out_cfg.dtype).matmul(beta_col).reshape({n});
  if (type == GLMPredictionType::Linear) {
    return eta;
  }
  return inv_link_(eta.reshape({n, 1}), fit.link, out_cfg).reshape({n});
}

std::string glm_summary(const GLMFitResult& fit) {
  std::ostringstream oss;
  print_glm_summary(oss, fit);
  return oss.str();
}

std::ostream& print_glm_summary(std::ostream& os, const GLMFitResult& fit) {
  auto row = [&](const std::string& l1, const std::string& v1, const std::string& l2, const std::string& v2) {
    os << std::left << std::setw(18) << l1 << std::right << std::setw(22) << v1
       << "   "
       << std::left << std::setw(18) << l2 << std::right << std::setw(22) << v2 << '\n';
  };

  os << "                            GLM Regression Results                            \n";
  os << "==============================================================================\n";
  row("Dep. Variable:", fit.response_name, "Family:", family_name_(fit.family));
  row("Model:", "GLM", "Link:", link_name_(fit.link));
  row("Method:", "IRLS", "Pseudo R-squared:", fmt_short_(fit.pseudo_rsquared));
  row("Date:", now_date_(), "No. Iterations:", std::to_string(fit.iterations));
  row("Time:", now_time_(), "Converged:", fit.converged ? "true" : "false");
  row("No. Observations:", std::to_string(fit.nobs), "Df Residuals:", std::to_string(fit.df_resid));
  row("Df Model:", std::to_string(fit.df_model), "Log-Likelihood:", fmt_short_(fit.log_likelihood));
  row("Deviance:", fmt_short_(fit.deviance), "AIC:", fmt_short_(fit.aic));
  row("Null Deviance:", fmt_short_(fit.null_deviance), "BIC:", fmt_short_(fit.bic));
  os << "==============================================================================\n";

  os << std::left << std::setw(16) << ""
     << std::right << std::setw(12) << "coef"
     << std::setw(12) << "std err"
     << std::setw(12) << "z"
     << std::setw(12) << "P>|z|"
     << std::setw(12) << "[0.025"
     << std::setw(12) << "0.975]"
     << '\n';
  os << "------------------------------------------------------------------------------\n";

  const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto se = fit.std_errors.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto zval = fit.z_values.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto pval = fit.p_values.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto ci = fit.conf_int.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

  for (std::size_t i = 0; i < coef.size(); ++i) {
    const std::string name = (i < fit.coefficient_names.size()) ? fit.coefficient_names[i] : ("x" + std::to_string(i + 1));
    os << std::left << std::setw(16) << name
       << std::right << std::setw(12) << fmt_value_(coef[i], 4)
       << std::setw(12) << fmt_value_(se[i], 4)
       << std::setw(12) << fmt_value_(zval[i], 4)
       << std::setw(12) << fmt_value_(pval[i], 4)
       << std::setw(12) << fmt_value_(ci[2 * i], 4)
       << std::setw(12) << fmt_value_(ci[2 * i + 1], 4)
       << '\n';
  }
  os << "==============================================================================\n";
  row("Dispersion:", fmt_short_(fit.dispersion), "Cond. No.", fmt_short_(fit.condition_number));
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
