#include "pluss/subsp/stats/regression.h"

#include "../design_rank.h"
#include "../predict_input.h"
#include "../summary_labels.h"

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
  return x.to(ScalarType::Float64).reshape({1}).host<double>().front();
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
  return clamp01_(pluss::subsp::stats::distr::Normal{0.0, 1.0}.cdf(q).host<double>().front());
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

struct CleanData {
  Series y = pluss::series::undefined();
  Series x = pluss::series::undefined();
  Series w = pluss::series::undefined();
  Series off = pluss::series::undefined();
  std::int64_t n = 0;
  std::int64_t p = 0;
  std::int64_t dropped = 0;
};

CleanData sanitize_inputs_(const Series& y, const Series& x, const GLMOptions& options, Config cfg, const char* op) {
  const bool drop_missing = options.drop_missing;
  const auto out_cfg = f64_cfg_like_(x, cfg);
  const Series yv = as_vector_(y, op).to(out_cfg.platform, out_cfg.dtype).contiguous();
  const Series xm = as_matrix_(x, op).to(out_cfg.platform, out_cfg.dtype).contiguous();

  const std::int64_t n = yv.numel();
  PLUSS_CHECK(xm.sizes()[0] == n, std::string(op) + ": row mismatch between y and X");
  const std::int64_t p = xm.sizes()[1];
  PLUSS_CHECK(n > 0 && p >= 0, std::string(op) + ": empty y or X");

  const Series wv = options.weights.defined() ? as_vector_(options.weights, op).to(out_cfg.platform, out_cfg.dtype).contiguous()
                                              : pluss::series::ones({n}, out_cfg);
  const Series ov = options.offset.defined() ? as_vector_(options.offset, op).to(out_cfg.platform, out_cfg.dtype).contiguous()
                                             : pluss::series::zeros({n}, out_cfg);
  PLUSS_CHECK(wv.numel() == n, std::string(op) + ": weights must have one value per row");
  PLUSS_CHECK(ov.numel() == n, std::string(op) + ": offset must have one value per row");
  PLUSS_CHECK(scalar_(wv.lt(0.0).logical_and(wv.isfinite()).logical_not().all()) > 0.5, std::string(op) + ": weights must be non-negative");

  const Series finite_ok = yv.isfinite().logical_and(xm.isfinite().all(1, false)).logical_and(wv.isfinite()).logical_and(ov.isfinite());
  const bool all_ok = scalar_(finite_ok.all()) > 0.5;
  PLUSS_CHECK(all_ok || drop_missing, std::string(op) + ": non-finite values found and drop_missing=false");
  const Series row_ok = finite_ok.logical_and(wv.gt(0.0));  // a row of weight zero does not take part in the fit
  const std::int64_t n_clean = static_cast<std::int64_t>(std::llround(scalar_(row_ok.to(ScalarType::Float64).sum())));
  PLUSS_CHECK(n_clean > 0, std::string(op) + ": no observations left after filtering missing values");

  const Series idx = row_ok.nonzero().reshape({n_clean}).to(out_cfg.platform, ScalarType::Int64);
  return CleanData{
      .y = yv.index_select(0, idx),
      .x = xm.index_select(0, idx),
      .w = wv.index_select(0, idx),
      .off = ov.index_select(0, idx),
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

double deviance_value_(GLMFamily family, const Series& y, const Series& mu, const Series& w) {
  if (family == GLMFamily::Normal) {
    const Series r = y.sub(mu);
    return scalar_(r.mul(r).mul(w).sum());
  }
  if (family == GLMFamily::Poisson) {
    const Series yi = y.maximum(0.0);
    const Series mui = mu.maximum(kMinMu);
    const Series log_ratio = yi.maximum(kMinMu).div(mui).log();
    const Series core = yi.gt(0.0).where(yi.mul(log_ratio).sub(yi.sub(mui)), mui);
    return 2.0 * scalar_(core.mul(w).sum());
  }
  const Series yi = y.clamp(kMinMu, kMaxMu);
  const Series mui = mu.clamp(kMinMu, kMaxMu);
  const Series one = pluss::series::ones(yi.sizes(), f64_cfg_like_(yi, {}));
  const Series term = yi.mul(yi.div(mui).log())
                          .add(one.sub(yi).mul(one.sub(yi).div(one.sub(mui)).log()));
  return 2.0 * scalar_(term.mul(w).sum());
}

double loglik_value_(GLMFamily family, const Series& y, const Series& mu, const Series& w) {
  const std::int64_t n = y.numel();
  if (family == GLMFamily::Normal) {
    const Series r = y.sub(mu);
    const double sse = scalar_(r.mul(r).mul(w).sum());
    if (sse <= kEps) {
      return std::numeric_limits<double>::infinity();
    }
    const double sigma2_mle = sse / static_cast<double>(n);
    return -0.5 * (static_cast<double>(n) * (std::log(2.0 * std::numbers::pi * sigma2_mle) + 1.0) - scalar_(w.log().sum()));
  }
  if (family == GLMFamily::Poisson) {
    const Series yi = y.maximum(0.0);
    const Series mui = mu.maximum(kMinMu);
    return scalar_(yi.mul(mui.log()).sub(mui).sub(yi.add(1.0).lgamma()).mul(w).sum());
  }
  const Series yi = y.clamp(kMinMu, kMaxMu);
  const Series mui = mu.clamp(kMinMu, kMaxMu);
  const Series one = pluss::series::ones(yi.sizes(), f64_cfg_like_(yi, {}));
  return scalar_(yi.mul(mui.log()).add(one.sub(yi).mul(one.sub(mui).log())).mul(w).sum());
}

} // namespace

GLMFitResult glm_fit(const Series& y, const Series& x, const GLMOptions& options, Config cfg) {
  const char* op = "subsp::stats::regression::glm_fit";
  check_alpha_(options.alpha, op);
  PLUSS_CHECK(std::isfinite(options.rcond) && options.rcond > 0.0, std::string(op) + ": rcond must be > 0");
  PLUSS_CHECK(options.max_iter > 0, std::string(op) + ": max_iter must be > 0");
  PLUSS_CHECK(std::isfinite(options.tol) && options.tol > 0.0, std::string(op) + ": tol must be > 0");

  const auto clean = sanitize_inputs_(y, x, options, cfg, op);
  PLUSS_CHECK(clean.p > 0 || options.add_intercept, std::string(op) + ": a model needs at least one predictor or an intercept");
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
  if (resolved_link == GLMLink::Logit) {
    // the logit link keeps the mean strictly between 0 and 1, so a response outside [0,1] could never be matched
    const bool valid = scalar_(clean.y.ge(0.0).logical_and(clean.y.le(1.0)).all()) > 0.5;
    PLUSS_CHECK(valid, std::string(op) + ": the logit link expects y in [0,1]");
  }

  const Series X = build_design_(clean.x, options.add_intercept, out_cfg);
  const Series ycol = clean.y.reshape({clean.n, 1});
  const std::int64_t n = X.sizes()[0];
  const std::int64_t k = X.sizes()[1];
  PLUSS_CHECK(n >= k, std::string(op) + ": requires n >= number of parameters");

  // One cutoff defines both the rank and the pseudoinverse that produces the coefficients.
  const auto drank = detail::design_rank(X, options.rcond);
  const double smax = drank.largest;
  const double smin = drank.smallest;
  const std::int64_t rank = drank.rank;

  const Series pw = clean.w.reshape({n, 1});
  const Series off = clean.off.reshape({n, 1});
  const double total_weight = scalar_(pw.sum());
  const double weighted_offset = scalar_(off.mul(pw).sum()) / total_weight;

  // The starting point: the constant-mean model's coefficient on the link scale, less the mean offset.
  auto start_for = [&](std::int64_t columns, bool intercept) {
    Series b = pluss::series::zeros({columns, 1}, out_cfg);
    if (intercept && columns > 0) {
      const double ymean = scalar_(ycol.mul(pw).sum()) / total_weight;
      double b0 = 0.0;
      if (resolved_link == GLMLink::Log) {
        b0 = std::log(std::max(kMinMu, ymean));
      } else if (resolved_link == GLMLink::Logit) {
        const double ym = std::clamp(ymean, kMinMu, kMaxMu);
        b0 = std::log(ym / (1.0 - ym));
      } else {
        b0 = ymean;
      }
      b0 -= weighted_offset;
      const Series head = pluss::series::full({1, 1}, b0, out_cfg);
      b = columns > 1 ? head.unite({pluss::series::zeros({columns - 1, 1}, out_cfg)}, 0) : head;
    }
    return b;
  };

  struct Irls {
    Series beta = pluss::series::undefined();
    std::int64_t iterations = 0;
    bool converged = false;
  };
  // Iteratively reweighted least squares on the design `D`: each step regresses the working response on D with the weights
  // (case weight) / (variance * link-derivative^2).
  auto irls = [&](const Series& D, Series beta0, double pinv_rcond) {
    Irls r;
    Series b = beta0;
    Series e = D.matmul(b).add(off);
    for (std::int64_t it = 0; it < options.max_iter; ++it) {
      r.iterations = it + 1;
      const Series m = inv_link_(e, resolved_link, out_cfg);
      const Series var = variance_mu_(m, options.family, out_cfg);
      const Series gprime = link_derivative_mu_(m, resolved_link, out_cfg);
      const Series denom = var.mul(gprime).mul(gprime).maximum(kMinMu);
      const Series working_weight = pw.div(denom);
      const Series z = e.sub(off).add(ycol.sub(m).mul(gprime));
      const Series sw = working_weight.sqrt();
      const Series b_new = pluss::subsp::math::linalg::pinverse(D.mul(sw), pinv_rcond).matmul(z.mul(sw));
      const double max_abs_delta = scalar_(b_new.sub(b).abs().max());
      const double max_abs_beta = scalar_(b_new.abs().max());
      b = b_new;
      e = D.matmul(b).add(off);
      if (max_abs_delta <= options.tol * (1.0 + max_abs_beta)) {
        r.converged = true;
        break;
      }
    }
    r.beta = b;
    return r;
  };

  Series beta0 = start_for(k, options.add_intercept);
  if (options.family == GLMFamily::Normal) {
    // for the normal family with the identity link the first step is the weighted least-squares solution
    const Series sw0 = pw.sqrt();
    beta0 = pluss::subsp::math::linalg::pinverse(X.mul(sw0), drank.pinv_rcond).matmul(ycol.sub(off).mul(sw0));
  }
  const Irls fitted = irls(X, beta0, drank.pinv_rcond);
  Series beta = fitted.beta;
  const std::int64_t iters = fitted.iterations;
  const bool converged = fitted.converged;
  Series eta = X.matmul(beta).add(off);
  Series mu = inv_link_(eta, resolved_link, out_cfg);
  Series weights = pw;
  std::vector<std::string> warnings;
  if (!converged) {
    warnings.push_back("IRLS reached max_iter without satisfying tolerance.");
  }

  mu = inv_link_(eta, resolved_link, out_cfg);
  const Series resid = ycol.sub(mu).reshape({n});

  const Series w_vec = pw.reshape({n});
  const double deviance = deviance_value_(options.family, ycol.reshape({n}), mu.reshape({n}), w_vec);
  const double llf = loglik_value_(options.family, ycol.reshape({n}), mu.reshape({n}), w_vec);

  // The null model has a constant linear predictor (plus the offset, if there is one).
  Series mu_null = pluss::series::undefined();
  if (options.offset.defined() && options.family != GLMFamily::Normal) {
    const Series ones_design = pluss::series::ones({n, 1}, out_cfg);
    const Irls null_fit = irls(ones_design, start_for(1, true), options.rcond);
    mu_null = inv_link_(ones_design.matmul(null_fit.beta).add(off), resolved_link, out_cfg);
  } else {
    const double mean_y = scalar_(ycol.mul(pw).sum()) / total_weight - (options.family == GLMFamily::Normal ? weighted_offset : 0.0);
    mu_null = pluss::series::full({n, 1}, mean_y, out_cfg);
    if (options.family == GLMFamily::Normal) {
      mu_null = mu_null.add(off);
    }
    if (options.family == GLMFamily::Binomial) {
      mu_null = mu_null.maximum(kMinMu).minimum(kMaxMu);
    } else if (options.family == GLMFamily::Poisson) {
      mu_null = mu_null.maximum(kMinMu);
    }
  }
  const double null_deviance = deviance_value_(options.family, ycol.reshape({n}), mu_null.reshape({n}), w_vec);
  const double pseudo_r2 =
      (std::isfinite(null_deviance) && null_deviance > kEps) ? (1.0 - deviance / null_deviance) : std::numeric_limits<double>::quiet_NaN();

  const std::int64_t df_resid = std::max<std::int64_t>(0, n - rank);
  const std::int64_t df_model = options.add_intercept ? std::max<std::int64_t>(0, rank - 1) : rank;

  const Series var = variance_mu_(mu, options.family, out_cfg);
  const Series gprime = link_derivative_mu_(mu, resolved_link, out_cfg);
  const Series denom = var.mul(gprime).mul(gprime).maximum(kMinMu);
  weights = pw.div(denom);

  const Series sw = weights.sqrt();
  const Series Xw = X.mul(sw);
  const Series pinv_xw = pluss::subsp::math::linalg::pinverse(Xw, drank.pinv_rcond);
  const Series xtx_inv = pinv_xw.matmul(pinv_xw.transpose(0, 1));

  double dispersion = 1.0;
  if (options.family == GLMFamily::Normal) {
    dispersion = (df_resid > 0) ? (deviance / static_cast<double>(df_resid)) : std::numeric_limits<double>::quiet_NaN();
  }
  const Series cov = xtx_inv.mul(dispersion);
  const Series std_err = cov.diagonal().reshape({k}).maximum(0.0).sqrt();

  const Series coef = beta.reshape({k});
  const Series z_v = coef.div(std_err.maximum(std::sqrt(kEps)));
  const auto zv = z_v.to(pluss::pCPU, ScalarType::Float64).host<double>();
  std::vector<double> pv(zv.size());
  for (std::size_t i = 0; i < zv.size(); ++i) {
    pv[i] = std::min(1.0, 2.0 * pluss::subsp::stats::tails::normal_sf(std::abs(zv[i])));
  }
  const Series p_v = pluss::make_series(pv, {k}, out_cfg);
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
  return glm_predict(fit, x, pluss::series::undefined(), type, cfg);
}

Series glm_predict(const GLMFitResult& fit, const Series& x, const Series& offset, GLMPredictionType type, Config cfg) {
  const char* op = "subsp::stats::regression::glm_predict";
  PLUSS_CHECK(fit.coefficients.defined(), std::string(op) + ": fit.coefficients is undefined");

  const Series xm = detail::prediction_matrix(x, fit.num_predictors, fit.k_params, op).to(fit.coefficients.platform(), ScalarType::Float64);
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
  Series eta = design.to(out_cfg.platform, out_cfg.dtype).matmul(beta_col).reshape({n});
  if (offset.defined()) {
    const Series shift = offset.to(out_cfg.platform, out_cfg.dtype).reshape({offset.numel()});
    PLUSS_CHECK(shift.numel() == n, std::string(op) + ": offset must have one value per row of X");
    eta = eta.add(shift);
  }
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
  PLUSS_CHECK(fit.coefficients.defined(), "subsp::stats::regression::print_glm_summary: fit.coefficients is undefined");
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
  row("Rank:", std::to_string(fit.rank), "No. Iterations:", std::to_string(fit.iterations));
  row("Alpha:", fmt_short_(fit.alpha), "Converged:", fit.converged ? "true" : "false");
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
     << std::setw(12) << detail::ci_lower_label(fit.alpha)
     << std::setw(12) << detail::ci_upper_label(fit.alpha)
     << '\n';
  os << "------------------------------------------------------------------------------\n";

  const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();
  const auto se = fit.std_errors.to(pluss::pCPU, ScalarType::Float64).host<double>();
  const auto zval = fit.z_values.to(pluss::pCPU, ScalarType::Float64).host<double>();
  const auto pval = fit.p_values.to(pluss::pCPU, ScalarType::Float64).host<double>();
  const auto ci = fit.conf_int.to(pluss::pCPU, ScalarType::Float64).host<double>();

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
