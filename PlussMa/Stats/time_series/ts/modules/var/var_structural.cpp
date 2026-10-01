#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/stats/distr.h"

#include <algorithm>
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

struct ModelDims {
  std::int64_t p = 0;
  std::int64_t k = 0;
  std::int64_t m = 0;
};

struct VarDesign {
  Series design = pluss::series::undefined();
  Series target = pluss::series::undefined();
  std::int64_t t = 0;
  std::int64_t d = 0;
  std::int64_t k = 0;
  std::int64_t m = 0;
};

struct GrangerCase {
  std::int64_t cause = -1;
  std::int64_t effect = -1;
  std::vector<std::int64_t> rows{};
};

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config i64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_matrix_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0], 1});
  }
  PLUSS_CHECK(sizes.size() == 2, std::string("subsp::stats::ts::") + name + ": expected 1D or 2D Series");
  return x;
}

double scalar_(const Series& x) {
  return x.to(ScalarType::Float64).reshape({1}).item<double>();
}

double clamp01_(double p) {
  return std::clamp(p, 0.0, 1.0);
}

Series i64_range_(std::int64_t start, std::int64_t end, Config cfg) {
  return Series::arange(start, end, 1, ScalarType::Int64).to(i64_cfg_(cfg).platform, i64_cfg_(cfg).dtype);
}

ModelDims validate_model_dims_(const VarFit& fit, const char* op_name) {
  PLUSS_CHECK(fit.ar.defined(), std::string("subsp::stats::ts::") + op_name + ": AR tensor is undefined");
  const auto ar_sizes = fit.ar.sizes();
  PLUSS_CHECK(ar_sizes.size() == 3, std::string("subsp::stats::ts::") + op_name + ": AR tensor must be [p,k,k]");
  PLUSS_CHECK(ar_sizes[0] > 0, std::string("subsp::stats::ts::") + op_name + ": lag_order must be > 0");
  PLUSS_CHECK(ar_sizes[1] == ar_sizes[2],
              std::string("subsp::stats::ts::") + op_name + ": AR tensor must have square per-lag matrices");

  const std::int64_t p = ar_sizes[0];
  const std::int64_t k = ar_sizes[1];
  const std::int64_t m = fit.beta.defined() ? as_matrix_(fit.beta, op_name).sizes()[0] : 0;

  PLUSS_CHECK(fit.constant.defined(), std::string("subsp::stats::ts::") + op_name + ": constant is undefined");
  PLUSS_CHECK(fit.trend.defined(), std::string("subsp::stats::ts::") + op_name + ": trend is undefined");
  PLUSS_CHECK(fit.constant.numel() == k,
              std::string("subsp::stats::ts::") + op_name + ": constant size must match number of series");
  PLUSS_CHECK(fit.trend.numel() == k,
              std::string("subsp::stats::ts::") + op_name + ": trend size must match number of series");

  if (m > 0) {
    const auto bs = as_matrix_(fit.beta, op_name).sizes();
    PLUSS_CHECK(bs[1] == k, std::string("subsp::stats::ts::") + op_name + ": beta must be [m,k]");
  }

  PLUSS_CHECK(fit.covariance.defined(), std::string("subsp::stats::ts::") + op_name + ": covariance is undefined");
  const auto cs = fit.covariance.sizes();
  PLUSS_CHECK(cs.size() == 2 && cs[0] == k && cs[1] == k,
              std::string("subsp::stats::ts::") + op_name + ": covariance must be [k,k]");
  return ModelDims{.p = p, .k = k, .m = m};
}

Series stabilize_cov_(Series covariance, Config cfg) {
  Series cov = covariance.to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype).contiguous();
  const auto sizes = cov.sizes();
  PLUSS_CHECK(sizes.size() == 2 && sizes[0] == sizes[1], "subsp::stats::ts::Var: covariance must be square");
  const std::int64_t k = sizes[0];
  const Series eye = pluss::series::eye(k, k, f64_cfg_(cfg));
  double jitter = 1e-12;
  for (std::int64_t i = 0; i < 8; ++i) {
    const Series slogdet = pluss::subsp::math::linalg::slogdet(cov).to(f64_cfg_(cfg).dtype).reshape({2});
    const double sign = slogdet.select(0, 0).item<double>();
    const double logdet = slogdet.select(0, 1).item<double>();
    if (sign > 0.0 && std::isfinite(logdet)) {
      return cov;
    }
    cov = cov.add(eye.mul(jitter));
    jitter *= 10.0;
  }
  return cov;
}

Series coeff_matrix_(const VarFit& fit, const ModelDims& dims, Config cfg, const char* op_name) {
  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>((fit.include_intercept ? 1 : 0) + (fit.include_trend ? 1 : 0) + dims.p + (dims.m > 0 ? 1 : 0)));

  if (fit.include_intercept) {
    rows.push_back(fit.constant.reshape({1, dims.k}).to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype));
  }
  if (fit.include_trend) {
    rows.push_back(fit.trend.reshape({1, dims.k}).to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype));
  }
  for (std::int64_t lag = 0; lag < dims.p; ++lag) {
    rows.push_back(fit.ar.select(0, lag).to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype));
  }
  if (dims.m > 0) {
    rows.push_back(as_matrix_(fit.beta, op_name).to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype));
  }
  PLUSS_CHECK(!rows.empty(), std::string("subsp::stats::ts::") + op_name + ": empty coefficient matrix");
  if (rows.size() == 1) {
    return rows.front();
  }
  return rows.front().unite(std::vector<Series>(rows.begin() + 1, rows.end()), 0);
}

VarDesign build_design_(const VarFit& fit,
                        const Series& y_in,
                        const Series* x_in,
                        Config cfg,
                        const char* op_name) {
  const auto dims = validate_model_dims_(fit, op_name);
  const auto out_cfg = f64_cfg_(cfg);

  const Series y = as_matrix_(y_in, op_name).to(out_cfg.platform, out_cfg.dtype);
  const auto ys = y.sizes();
  const std::int64_t n = ys[0];
  PLUSS_CHECK(ys[1] == dims.k, std::string("subsp::stats::ts::") + op_name + ": response dimension mismatch");
  PLUSS_CHECK(n > dims.p, std::string("subsp::stats::ts::") + op_name + ": expected observations > lag_order");

  const std::int64_t t = n - dims.p;
  const Series idx_target = i64_range_(dims.p, n, out_cfg);
  const Series target = y.index_select(0, idx_target);

  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>((fit.include_intercept ? 1 : 0) + (fit.include_trend ? 1 : 0) + dims.p + (dims.m > 0 ? 1 : 0)));
  if (fit.include_intercept) {
    cols.push_back(pluss::series::ones({t, 1}, out_cfg));
  }
  if (fit.include_trend) {
    cols.push_back(Series::arange(1, t + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).reshape({t, 1}));
  }

  for (std::int64_t lag = 1; lag <= dims.p; ++lag) {
    cols.push_back(y.index_select(0, i64_range_(dims.p - lag, n - lag, out_cfg)));
  }

  if (dims.m > 0) {
    PLUSS_CHECK(x_in != nullptr && x_in->defined(),
                std::string("subsp::stats::ts::") + op_name + ": fitted model requires predictor matrix x");
    const Series x = as_matrix_(*x_in, op_name).to(out_cfg.platform, out_cfg.dtype);
    const auto xs = x.sizes();
    PLUSS_CHECK(xs[0] == n, std::string("subsp::stats::ts::") + op_name + ": predictor row count must match y");
    PLUSS_CHECK(xs[1] == dims.m, std::string("subsp::stats::ts::") + op_name + ": predictor column mismatch");
    cols.push_back(x.index_select(0, idx_target));
  } else {
    PLUSS_CHECK(x_in == nullptr || !x_in->defined() || as_matrix_(*x_in, op_name).sizes()[1] == 0,
                std::string("subsp::stats::ts::") + op_name + ": model has no predictor coefficients");
  }

  PLUSS_CHECK(!cols.empty(), std::string("subsp::stats::ts::") + op_name + ": no predictors in design matrix");
  const Series design = (cols.size() == 1) ? cols.front()
                                           : cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
  return VarDesign{
      .design = design,
      .target = target,
      .t = t,
      .d = design.sizes()[1],
      .k = dims.k,
      .m = dims.m,
  };
}

double chisq_cdf_(double value, std::int64_t dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{value}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::ChiSquare{dof}.cdf(q).item<double>());
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

double f_cdf_(double value, std::int64_t dof1, std::int64_t dof2, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{value}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::FDistribution{static_cast<double>(dof1), static_cast<double>(dof2)}.cdf(q).item<double>());
}

double f_quantile_(double prob, std::int64_t dof1, std::int64_t dof2, Config cfg) {
  if (prob <= 0.0) {
    return 0.0;
  }
  if (prob >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = 0.0;
  double hi = 1.0;
  for (std::int64_t i = 0; i < 80 && f_cdf_(hi, dof1, dof2, cfg) < prob; ++i) {
    hi *= 2.0;
  }
  for (std::int64_t i = 0; i < 90; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (f_cdf_(mid, dof1, dof2, cfg) < prob) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

std::vector<Series> ma_coeffs_(const VarFit& fit, std::int64_t horizon, Config cfg, const char* op_name) {
  PLUSS_CHECK(horizon > 0, std::string("subsp::stats::ts::") + op_name + ": horizon must be > 0");
  const auto dims = validate_model_dims_(fit, op_name);
  const auto out_cfg = f64_cfg_(cfg);

  std::vector<Series> ar_lags;
  ar_lags.reserve(static_cast<std::size_t>(dims.p));
  for (std::int64_t lag = 0; lag < dims.p; ++lag) {
    ar_lags.push_back(fit.ar.select(0, lag).to(out_cfg.platform, out_cfg.dtype));
  }

  std::vector<Series> psi;
  psi.reserve(static_cast<std::size_t>(horizon));
  psi.push_back(pluss::series::eye(dims.k, dims.k, out_cfg));
  for (std::int64_t h = 1; h < horizon; ++h) {
    Series acc = pluss::series::zeros({dims.k, dims.k}, out_cfg);
    const std::int64_t max_lag = std::min<std::int64_t>(dims.p, h);
    for (std::int64_t lag = 1; lag <= max_lag; ++lag) {
      acc = acc.add(psi[static_cast<std::size_t>(h - lag)].matmul(ar_lags[static_cast<std::size_t>(lag - 1)]));
    }
    psi.push_back(acc);
  }
  return psi;
}

Series shock_map_(const Series& covariance, IrfMethod method, Config cfg, const char* op_name) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto cov = stabilize_cov_(covariance, out_cfg);
  const std::int64_t k = cov.sizes()[0];

  if (method == IrfMethod::Orthogonalized) {
    return pluss::subsp::math::linalg::cholesky(cov).transpose(0, 1).to(out_cfg.platform, out_cfg.dtype);
  }

  const Series eye = pluss::series::eye(k, k, out_cfg);
  const Series diag = cov.mul(eye).sum(0, false).reshape({k}).clamp_min(kEps).sqrt();
  const Series d_inv = eye.div(diag.reshape({k, 1}));
  const Series map = d_inv.matmul(cov);
  PLUSS_CHECK(map.isfinite().all().item<double>() == 1.0,
              std::string("subsp::stats::ts::") + op_name + ": generalized shock mapping is not finite");
  return map;
}

std::vector<std::int64_t> normalize_index_set_(const std::vector<std::int64_t>& raw,
                                               std::int64_t k,
                                               const char* op_name,
                                               const char* field) {
  if (raw.empty()) {
    std::vector<std::int64_t> full(static_cast<std::size_t>(k));
    std::iota(full.begin(), full.end(), 0);
    return full;
  }

  std::vector<std::int64_t> out;
  out.reserve(raw.size());
  std::vector<unsigned char> seen(static_cast<std::size_t>(k), 0);
  for (std::int64_t v : raw) {
    PLUSS_CHECK(v >= 0 && v < k,
                std::string("subsp::stats::ts::") + op_name + ": " + field +
                    " indices must be in [0, num_series)");
    if (!seen[static_cast<std::size_t>(v)]) {
      seen[static_cast<std::size_t>(v)] = 1;
      out.push_back(v);
    }
  }
  PLUSS_CHECK(!out.empty(),
              std::string("subsp::stats::ts::") + op_name + ": " + field + " resolves to empty index set");
  return out;
}

std::vector<GrangerCase> granger_cases_(const VarFit& fit,
                                        const std::vector<std::int64_t>& cause,
                                        const std::vector<std::int64_t>& effect,
                                        const GrangerOptions& options,
                                        std::int64_t k,
                                        const char* op_name) {
  const std::int64_t lag_row0 = (fit.include_intercept ? 1 : 0) + (fit.include_trend ? 1 : 0);
  const std::int64_t p = fit.lag_order;
  std::vector<GrangerCase> cases;

  auto append_case = [&](std::int64_t cause_label,
                         std::int64_t effect_eq,
                         const std::vector<std::int64_t>& src_vars) {
    std::vector<std::int64_t> rows;
    rows.reserve(static_cast<std::size_t>(p * static_cast<std::int64_t>(src_vars.size())));
    for (std::int64_t lag = 0; lag < p; ++lag) {
      for (std::int64_t src : src_vars) {
        rows.push_back(lag_row0 + lag * k + src);
      }
    }
    if (!rows.empty()) {
      cases.push_back(GrangerCase{.cause = cause_label, .effect = effect_eq, .rows = std::move(rows)});
    }
  };

  switch (options.type) {
    case GrangerType::LeaveOneOut: {
      const bool default_sets = options.cause.empty() && options.effect.empty();
      for (std::int64_t eff : effect) {
        for (std::int64_t src : cause) {
          if (default_sets && src == eff) {
            continue;
          }
          append_case(src, eff, std::vector<std::int64_t>{src});
        }
      }
      break;
    }
    case GrangerType::ExcludeAll: {
      for (std::int64_t eff : effect) {
        std::vector<std::int64_t> src_vars;
        src_vars.reserve(static_cast<std::size_t>(k - 1));
        for (std::int64_t src = 0; src < k; ++src) {
          if (src != eff) {
            src_vars.push_back(src);
          }
        }
        append_case(-1, eff, src_vars);
      }
      break;
    }
    case GrangerType::BlockWise: {
      PLUSS_CHECK(!cause.empty(), std::string("subsp::stats::ts::") + op_name + ": block-wise requires non-empty cause set");
      PLUSS_CHECK(!effect.empty(), std::string("subsp::stats::ts::") + op_name + ": block-wise requires non-empty effect set");
      for (std::int64_t eff : effect) {
        append_case(-1, eff, cause);
      }
      break;
    }
  }
  return cases;
}

GrangerResult run_granger_(const VarFit& fit,
                           const Series& y,
                           const Series* x,
                           const GrangerOptions& options,
                           Config cfg,
                           const char* op_name) {
  PLUSS_CHECK(options.alpha > 0.0 && options.alpha < 1.0,
              std::string("subsp::stats::ts::") + op_name + ": alpha must be in (0,1)");
  const auto out_cfg = f64_cfg_(cfg);
  const auto dims = validate_model_dims_(fit, op_name);
  const auto cause = normalize_index_set_(options.cause, dims.k, op_name, "cause");
  const auto effect = normalize_index_set_(options.effect, dims.k, op_name, "effect");
  const auto tests = granger_cases_(fit, cause, effect, options, dims.k, op_name);

  if (tests.empty()) {
    return GrangerResult{
        .cause = pluss::series::empty({0}, i64_cfg_(out_cfg)),
        .effect = pluss::series::empty({0}, i64_cfg_(out_cfg)),
        .stat = pluss::series::empty({0}, out_cfg),
        .pvalue = pluss::series::empty({0}, out_cfg),
        .critical = pluss::series::empty({0}, out_cfg),
        .dof1 = pluss::series::empty({0}, i64_cfg_(out_cfg)),
        .dof2 = pluss::series::empty({0}, i64_cfg_(out_cfg)),
        .reject = pluss::series::empty({0}, i64_cfg_(out_cfg)),
    };
  }

  const auto design = build_design_(fit, y, x, out_cfg, op_name);
  const Series coeff = coeff_matrix_(fit, dims, out_cfg, op_name);
  PLUSS_CHECK(coeff.sizes()[0] == design.d && coeff.sizes()[1] == design.k,
              std::string("subsp::stats::ts::") + op_name + ": coefficient/design dimension mismatch");

  const Series residuals = design.target.sub(design.design.matmul(coeff));
  const Series rss = residuals.mul(residuals).sum(0, false).reshape({design.k});

  const std::int64_t dof2 = design.t - design.d;
  PLUSS_CHECK(dof2 > 0, std::string("subsp::stats::ts::") + op_name + ": effective degrees of freedom must be > 0");
  const Series xtx_inv = pluss::subsp::math::linalg::pinverse(
      design.design.transpose(0, 1).matmul(design.design), 1e-12);

  std::vector<std::int64_t> cause_out;
  std::vector<std::int64_t> effect_out;
  std::vector<double> stat_out;
  std::vector<double> pvalue_out;
  std::vector<double> critical_out;
  std::vector<std::int64_t> dof1_out;
  std::vector<std::int64_t> dof2_out;
  std::vector<std::int64_t> reject_out;

  cause_out.reserve(tests.size());
  effect_out.reserve(tests.size());
  stat_out.reserve(tests.size());
  pvalue_out.reserve(tests.size());
  critical_out.reserve(tests.size());
  dof1_out.reserve(tests.size());
  dof2_out.reserve(tests.size());
  reject_out.reserve(tests.size());

  for (const auto& test : tests) {
    const std::int64_t q = static_cast<std::int64_t>(test.rows.size());
    if (q <= 0) {
      continue;
    }

    const Series idx = pluss::make_series(test.rows, {q}, i64_cfg_(out_cfg)).to(i64_cfg_(out_cfg).platform, i64_cfg_(out_cfg).dtype);
    const Series b_eq = coeff.select(1, test.effect).reshape({design.d});
    const Series rb = b_eq.index_select(0, idx).reshape({q, 1});

    const double rss_eq = rss.select(0, test.effect).item<double>();
    const double sigma2 = std::max(kEps, rss_eq / static_cast<double>(dof2));

    const Series v_rr = xtx_inv.index_select(0, idx).index_select(1, idx).mul(sigma2);
    const double wald = std::max(
        0.0,
        scalar_(rb.transpose(0, 1).matmul(pluss::subsp::math::linalg::pinverse(v_rr, 1e-12)).matmul(rb)));

    double stat = 0.0;
    double pvalue = 1.0;
    double critical = 0.0;
    if (options.stat == GrangerStat::F) {
      stat = wald / static_cast<double>(q);
      pvalue = clamp01_(1.0 - f_cdf_(stat, q, dof2, out_cfg));
      critical = f_quantile_(1.0 - options.alpha, q, dof2, out_cfg);
    } else {
      stat = wald;
      pvalue = clamp01_(1.0 - chisq_cdf_(stat, q, out_cfg));
      critical = chisq_quantile_(1.0 - options.alpha, q, out_cfg);
    }

    cause_out.push_back(test.cause);
    effect_out.push_back(test.effect);
    stat_out.push_back(stat);
    pvalue_out.push_back(pvalue);
    critical_out.push_back(critical);
    dof1_out.push_back(q);
    dof2_out.push_back(dof2);
    reject_out.push_back((pvalue < options.alpha) ? 1 : 0);
  }

  const std::int64_t ntests = static_cast<std::int64_t>(stat_out.size());
  return GrangerResult{
      .cause = pluss::make_series(cause_out, {ntests}, i64_cfg_(out_cfg)),
      .effect = pluss::make_series(effect_out, {ntests}, i64_cfg_(out_cfg)),
      .stat = pluss::make_series(stat_out, {ntests}, out_cfg),
      .pvalue = pluss::make_series(pvalue_out, {ntests}, out_cfg),
      .critical = pluss::make_series(critical_out, {ntests}, out_cfg),
      .dof1 = pluss::make_series(dof1_out, {ntests}, i64_cfg_(out_cfg)),
      .dof2 = pluss::make_series(dof2_out, {ntests}, i64_cfg_(out_cfg)),
      .reject = pluss::make_series(reject_out, {ntests}, i64_cfg_(out_cfg)),
  };
}

} // namespace

Series VarFit::irf(const IrfOptions& options, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const auto dims = validate_model_dims_(*this, "VarFit::irf");
  const auto psi = ma_coeffs_(*this, options.horizon, out_cfg, "VarFit::irf");
  const Series map = shock_map_(covariance, options.method, out_cfg, "VarFit::irf");

  std::vector<Series> pages;
  pages.reserve(static_cast<std::size_t>(options.horizon));
  for (const Series& psi_h : psi) {
    pages.push_back(map.matmul(psi_h).reshape({1, dims.k, dims.k}));
  }
  if (pages.size() == 1) {
    return pages.front().to(out_cfg.platform, out_cfg.dtype);
  }
  return pages.front().unite(std::vector<Series>(pages.begin() + 1, pages.end()), 0).to(out_cfg.platform, out_cfg.dtype);
}

Series VarFit::fevd(const FevdOptions& options, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const auto dims = validate_model_dims_(*this, "VarFit::fevd");
  const Series cov = stabilize_cov_(covariance, out_cfg);
  const auto psi = ma_coeffs_(*this, options.horizon, out_cfg, "VarFit::fevd");
  const Series map = shock_map_(cov, options.method, out_cfg, "VarFit::fevd");
  const Series eye = pluss::series::eye(dims.k, dims.k, out_cfg);

  Series num = pluss::series::zeros({dims.k, dims.k}, out_cfg);
  Series den = pluss::series::zeros({dims.k}, out_cfg);
  std::vector<Series> pages;
  pages.reserve(static_cast<std::size_t>(options.horizon));

  for (const Series& psi_h : psi) {
    const Series resp = map.matmul(psi_h);
    num = num.add(resp.mul(resp));

    const Series cov_h = psi_h.transpose(0, 1).matmul(cov).matmul(psi_h);
    den = den.add(cov_h.mul(eye).sum(0, false).reshape({dims.k}));

    const Series fevd_h = num.div(den.reshape({1, dims.k}).clamp_min(kEps));
    pages.push_back(fevd_h.reshape({1, dims.k, dims.k}));
  }

  if (pages.size() == 1) {
    return pages.front().to(out_cfg.platform, out_cfg.dtype);
  }
  return pages.front().unite(std::vector<Series>(pages.begin() + 1, pages.end()), 0).to(out_cfg.platform, out_cfg.dtype);
}

GrangerResult VarFit::granger_test(const Series& y, const GrangerOptions& options, Config cfg) const {
  return run_granger_(*this, y, nullptr, options, cfg, "VarFit::granger_test");
}

GrangerResult VarFit::granger_test(const Series& y, const Series& x, const GrangerOptions& options, Config cfg) const {
  return run_granger_(*this, y, &x, options, cfg, "VarFit::granger_test");
}

} // namespace pluss::subsp::stats::ts
