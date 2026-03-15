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

constexpr double kLog2Pi = 1.83787706640934533908;
constexpr double kEps = 1e-14;

struct VecmDesign {
  Series design = Series::undefined();
  Series target = Series::undefined();
  Series level_lag = Series::undefined();
  std::int64_t n = 0;
  std::int64_t t = 0;
  std::int64_t k = 0;
  std::int64_t p = 0;
  std::int64_t d = 0;
};

struct CoeffLayout {
  std::int64_t intercept = -1;
  std::int64_t trend = -1;
  std::int64_t impact_row0 = -1;
  std::int64_t gamma_row0 = -1;
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

Series i64_range_(std::int64_t start, std::int64_t end, Config cfg) {
  return Series::arange(start, end, 1, ScalarType::Int64).to(i64_cfg_(cfg).platform, i64_cfg_(cfg).dtype);
}

double scalar_(const Series& x) {
  return x.to(ScalarType::Float64).reshape({1}).item<double>();
}

double clamp01_(double p) {
  return std::clamp(p, 0.0, 1.0);
}

CoeffLayout coeff_layout_(const VecmOptions& options, std::int64_t k) {
  CoeffLayout layout;
  std::int64_t row = 0;
  if (options.include_intercept) {
    layout.intercept = row++;
  }
  if (options.include_trend) {
    layout.trend = row++;
  }
  layout.impact_row0 = row;
  row += k;
  layout.gamma_row0 = row;
  return layout;
}

Series stabilize_cov_(Series covariance, Config cfg) {
  Series cov = covariance.to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype).contiguous();
  const auto sizes = cov.sizes();
  PLUSS_CHECK(sizes.size() == 2 && sizes[0] == sizes[1], "subsp::stats::ts::Vecm: covariance must be square");
  const std::int64_t k = sizes[0];
  const Series eye = pluss::eye(k, k, f64_cfg_(cfg));
  double jitter = 1e-12;
  for (std::int64_t i = 0; i < 8; ++i) {
    const Series slogdet = pluss::subsp::math::linalg::slogdet(cov).reshape({2});
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

VecmDesign build_design_(const Series& y_in,
                         std::int64_t lag_order,
                         bool include_intercept,
                         bool include_trend,
                         Config cfg,
                         const char* op_name) {
  PLUSS_CHECK(lag_order > 0, std::string("subsp::stats::ts::") + op_name + ": lag_order must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series y = as_matrix_(y_in, op_name).to(out_cfg.platform, out_cfg.dtype);
  const auto ys = y.sizes();
  const std::int64_t n = ys[0];
  const std::int64_t k = ys[1];

  PLUSS_CHECK(n > lag_order + 1,
              std::string("subsp::stats::ts::") + op_name + ": expected observations > lag_order + 1");
  PLUSS_CHECK(k > 0, std::string("subsp::stats::ts::") + op_name + ": expected at least one series");

  const Series y_curr = y.index_select(0, i64_range_(1, n, out_cfg));
  const Series y_prev = y.index_select(0, i64_range_(0, n - 1, out_cfg));
  const Series dy = y_curr.sub(y_prev);

  const std::int64_t t = n - lag_order;
  const Series target = dy.index_select(0, i64_range_(lag_order - 1, n - 1, out_cfg));
  const Series level_lag = y.index_select(0, i64_range_(lag_order - 1, n - 1, out_cfg));

  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>((include_intercept ? 1 : 0) + (include_trend ? 1 : 0) + lag_order));
  if (include_intercept) {
    cols.push_back(pluss::ones({t, 1}, out_cfg));
  }
  if (include_trend) {
    cols.push_back(Series::arange(1, t + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).reshape({t, 1}));
  }
  cols.push_back(level_lag);
  for (std::int64_t i = 1; i < lag_order; ++i) {
    cols.push_back(dy.index_select(0, i64_range_(lag_order - 1 - i, n - 1 - i, out_cfg)));
  }

  const Series design = (cols.size() == 1) ? cols.front()
                                           : cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
  return VecmDesign{
      .design = design,
      .target = target,
      .level_lag = level_lag,
      .n = n,
      .t = t,
      .k = k,
      .p = lag_order,
      .d = design.sizes()[1],
  };
}

Series coeff_from_fit_(const VecmFit& fit, Config cfg, const char* op_name) {
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t k = fit.impact.sizes()[0];
  const std::int64_t p = fit.lag_order;

  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>((fit.include_intercept ? 1 : 0) + (fit.include_trend ? 1 : 0) + 1 + (p > 1 ? 1 : 0)));
  if (fit.include_intercept) {
    rows.push_back(fit.constant.reshape({1, k}).to(out_cfg.platform, out_cfg.dtype));
  }
  if (fit.include_trend) {
    rows.push_back(fit.trend.reshape({1, k}).to(out_cfg.platform, out_cfg.dtype));
  }
  rows.push_back(fit.impact.to(out_cfg.platform, out_cfg.dtype));
  if (p > 1) {
    const auto gs = fit.gamma.sizes();
    PLUSS_CHECK(gs.size() == 3 && gs[0] == p - 1 && gs[1] == k && gs[2] == k,
                std::string("subsp::stats::ts::") + op_name + ": gamma must be [p-1,k,k]");
    for (std::int64_t i = 0; i < p - 1; ++i) {
      rows.push_back(fit.gamma.select(0, i).to(out_cfg.platform, out_cfg.dtype));
    }
  }
  return (rows.size() == 1) ? rows.front()
                            : rows.front().unite(std::vector<Series>(rows.begin() + 1, rows.end()), 0);
}

Series svd_diag_(const Series& singular_values, std::int64_t rank, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  if (rank <= 0) {
    return pluss::zeros({0, 0}, out_cfg);
  }
  const Series s = singular_values.index_select(0, i64_range_(0, rank, out_cfg)).reshape({rank});
  return pluss::eye(rank, rank, out_cfg).mul(s.reshape({1, rank}));
}

Series inv_sqrt_spd_(const Series& a, Config cfg, const char* op_name) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto svd = pluss::subsp::math::linalg::svd(a.to(out_cfg.platform, out_cfg.dtype), false);
  const auto sizes = a.sizes();
  const std::int64_t k = sizes[0];
  const Series u = svd.U.to(out_cfg.platform, out_cfg.dtype);
  const Series s = svd.S.to(out_cfg.platform, out_cfg.dtype).clamp_min(kEps).sqrt();
  const Series d_inv = pluss::eye(k, k, out_cfg).div(s.reshape({1, k}));
  const Series out = u.matmul(d_inv).matmul(u.transpose(0, 1));
  PLUSS_CHECK(out.isfinite().all().item<double>() == 1.0,
              std::string("subsp::stats::ts::") + op_name + ": inverse sqrt produced non-finite values");
  return out;
}

double chisq_cdf_(double value, std::int64_t dof, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series q = pluss::make_series(std::vector<double>{value}, {1}, out_cfg);
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

Series per_obs_loglik_(const Series& residuals, const Series& covariance, Config cfg, const char* op_name) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series res = as_matrix_(residuals, op_name).to(out_cfg.platform, out_cfg.dtype);
  const Series cov = stabilize_cov_(covariance, out_cfg);
  const Series inv_cov = pluss::subsp::math::linalg::pinverse(cov, 1e-12);
  const Series slogdet = pluss::subsp::math::linalg::slogdet(cov).reshape({2});
  const double sign = slogdet.select(0, 0).item<double>();
  const double logdet = slogdet.select(0, 1).item<double>();
  PLUSS_CHECK(sign > 0.0 && std::isfinite(logdet),
              std::string("subsp::stats::ts::") + op_name + ": covariance must be positive definite");

  const double offset = -0.5 * (static_cast<double>(res.sizes()[1]) * kLog2Pi + logdet);
  return res.matmul(inv_cov).mul(res).sum(1, false).mul(-0.5).add(offset);
}

Series reduced_rank_impact_(const Series& pi, std::int64_t rank, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t k = pi.sizes()[0];
  if (rank <= 0) {
    return pluss::zeros({k, k}, out_cfg);
  }
  if (rank >= k) {
    return pi.to(out_cfg.platform, out_cfg.dtype);
  }
  const auto svd = pluss::subsp::math::linalg::svd(pi.to(out_cfg.platform, out_cfg.dtype), false);
  const Series u = svd.U.index_select(1, i64_range_(0, rank, out_cfg));
  const Series v = svd.Vh.transpose(0, 1).index_select(1, i64_range_(0, rank, out_cfg));
  const Series s = svd_diag_(svd.S, rank, out_cfg);
  return u.matmul(s).matmul(v.transpose(0, 1));
}

std::pair<Series, Series> alpha_beta_from_impact_(const Series& impact, std::int64_t rank, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t k = impact.sizes()[0];
  if (rank <= 0) {
    return {pluss::empty({k, 0}, out_cfg), pluss::empty({k, 0}, out_cfg)};
  }
  const auto svd = pluss::subsp::math::linalg::svd(impact.to(out_cfg.platform, out_cfg.dtype), false);
  const Series beta = svd.Vh.transpose(0, 1).index_select(1, i64_range_(0, rank, out_cfg));
  const Series alpha = impact.matmul(beta).matmul(
      pluss::subsp::math::linalg::pinverse(beta.transpose(0, 1).matmul(beta), 1e-12));
  return {alpha, beta};
}

} // namespace

JohansenResult johansen_test(const Series& y, const JohansenOptions& options, Config cfg) {
  PLUSS_CHECK(options.lag_order > 0, "subsp::stats::ts::johansen_test: lag_order must be > 0");
  PLUSS_CHECK(options.alpha > 0.0 && options.alpha < 1.0, "subsp::stats::ts::johansen_test: alpha must be in (0,1)");

  const auto out_cfg = f64_cfg_(cfg);
  const auto design = build_design_(y, options.lag_order, options.include_intercept, options.include_trend, out_cfg, "johansen_test");
  const std::int64_t k = design.k;
  const std::int64_t t = design.t;

  const CoeffLayout layout = coeff_layout_(
      VecmOptions{
          .lag_order = options.lag_order,
          .include_intercept = options.include_intercept,
          .include_trend = options.include_trend,
      },
      k);

  Series w = Series::undefined();
  if (layout.impact_row0 > 0) {
    w = design.design.index_select(1, i64_range_(0, layout.impact_row0, out_cfg));
  } else {
    w = pluss::empty({t, 0}, out_cfg);
  }

  const Series y0 = design.target;
  const Series y1 = design.level_lag;

  Series r0 = y0;
  Series r1 = y1;
  if (w.sizes()[1] > 0) {
    const Series w_pinv = pluss::subsp::math::linalg::pinverse(w, 1e-12);
    r0 = y0.sub(w.matmul(w_pinv.matmul(y0)));
    r1 = y1.sub(w.matmul(w_pinv.matmul(y1)));
  }

  const Series s00 = r0.transpose(0, 1).matmul(r0).div(static_cast<double>(t));
  const Series s11 = r1.transpose(0, 1).matmul(r1).div(static_cast<double>(t));
  const Series s01 = r0.transpose(0, 1).matmul(r1).div(static_cast<double>(t));
  const Series s10 = s01.transpose(0, 1);

  const Series s00_inv = pluss::subsp::math::linalg::pinverse(stabilize_cov_(s00, out_cfg), 1e-12);
  const Series s11_inv_sqrt = inv_sqrt_spd_(stabilize_cov_(s11, out_cfg), out_cfg, "johansen_test");
  Series c = s11_inv_sqrt.matmul(s10).matmul(s00_inv).matmul(s01).matmul(s11_inv_sqrt);
  c = c.add(c.transpose(0, 1)).mul(0.5);

  const auto svd = pluss::subsp::math::linalg::svd(c, false);
  Series eval = svd.S.to(out_cfg.platform, out_cfg.dtype).reshape({k}).clamp_min(0.0).clamp_max(1.0 - 1e-10);
  Series evec = s11_inv_sqrt.matmul(svd.U.to(out_cfg.platform, out_cfg.dtype));

  std::vector<double> lambda(k, 0.0);
  for (std::int64_t i = 0; i < k; ++i) {
    lambda[static_cast<std::size_t>(i)] = eval.select(0, i).item<double>();
  }

  std::vector<double> stat(k, 0.0);
  std::vector<double> pval(k, 1.0);
  std::vector<double> critical(k, 0.0);
  std::vector<std::int64_t> reject(k, 0);
  std::vector<std::int64_t> dof(k, 1);

  std::int64_t selected_rank = 0;
  for (std::int64_t r = 0; r < k; ++r) {
    double stat_r = 0.0;
    if (options.statistic == JohansenStat::MaxEigen) {
      stat_r = -static_cast<double>(t) * std::log(std::max(1e-12, 1.0 - lambda[static_cast<std::size_t>(r)]));
      dof[static_cast<std::size_t>(r)] = std::max<std::int64_t>(1, k - r);
    } else {
      double acc = 0.0;
      for (std::int64_t i = r; i < k; ++i) {
        acc += std::log(std::max(1e-12, 1.0 - lambda[static_cast<std::size_t>(i)]));
      }
      stat_r = -static_cast<double>(t) * acc;
      const std::int64_t m = k - r;
      dof[static_cast<std::size_t>(r)] = std::max<std::int64_t>(1, m * m);
    }

    const std::int64_t d = dof[static_cast<std::size_t>(r)];
    const double p = 1.0 - chisq_cdf_(stat_r, d, out_cfg);
    const double cval = chisq_quantile_(1.0 - options.alpha, d, out_cfg);
    const bool rej = stat_r > cval;

    stat[static_cast<std::size_t>(r)] = stat_r;
    pval[static_cast<std::size_t>(r)] = clamp01_(p);
    critical[static_cast<std::size_t>(r)] = cval;
    reject[static_cast<std::size_t>(r)] = rej ? 1 : 0;

    if (rej) {
      selected_rank = r + 1;
    }
  }
  selected_rank = std::min<std::int64_t>(selected_rank, k - 1);

  Series alpha = pluss::empty({k, 0}, out_cfg);
  Series beta = pluss::empty({k, 0}, out_cfg);
  if (selected_rank > 0) {
    beta = evec.index_select(1, i64_range_(0, selected_rank, out_cfg));
    const Series denom = beta.transpose(0, 1).matmul(s11).matmul(beta);
    alpha = s01.matmul(beta).matmul(pluss::subsp::math::linalg::pinverse(denom, 1e-12));
  }

  return JohansenResult{
      .eigenvalues = pluss::make_series(lambda, {k}, out_cfg),
      .stat = pluss::make_series(stat, {k}, out_cfg),
      .pvalue = pluss::make_series(pval, {k}, out_cfg),
      .critical = pluss::make_series(critical, {k}, out_cfg),
      .reject = pluss::make_series(reject, {k}, i64_cfg_(out_cfg)),
      .dof = pluss::make_series(dof, {k}, i64_cfg_(out_cfg)),
      .alpha = alpha,
      .beta = beta,
      .selected_rank = selected_rank,
  };
}

VecmFit Vecm::fit_mle(const Series& y, const VecmOptions& options, Config cfg) {
  PLUSS_CHECK(options.lag_order > 0, "subsp::stats::ts::Vecm::fit_mle: lag_order must be > 0");
  PLUSS_CHECK(options.alpha > 0.0 && options.alpha < 1.0, "subsp::stats::ts::Vecm::fit_mle: alpha must be in (0,1)");
  PLUSS_CHECK(options.ridge >= 0.0, "subsp::stats::ts::Vecm::fit_mle: ridge must be >= 0");
  PLUSS_CHECK(options.pinv_rcond > 0.0, "subsp::stats::ts::Vecm::fit_mle: pinv_rcond must be > 0");

  const auto out_cfg = f64_cfg_(cfg);
  const auto design = build_design_(
      y, options.lag_order, options.include_intercept, options.include_trend, out_cfg, "Vecm::fit_mle");
  const std::int64_t k = design.k;
  PLUSS_CHECK(options.rank <= k, "subsp::stats::ts::Vecm::fit_mle: rank must be <= number of series");

  Series x_fit = design.design;
  Series y_fit = design.target;
  if (options.ridge > 0.0) {
    Series penalty = pluss::eye(design.d, design.d, out_cfg).mul(std::sqrt(options.ridge));
    const CoeffLayout layout = coeff_layout_(options, k);
    const std::int64_t protected_rows = layout.impact_row0;
    if (protected_rows > 0) {
      const Series idx = i64_range_(0, protected_rows, out_cfg);
      const Series diag_idx = idx.reshape({protected_rows, 1});
      penalty = pluss::index_put(
          penalty,
          std::vector<Series>{diag_idx, diag_idx},
          pluss::zeros({protected_rows}, out_cfg),
          false);
    }
    x_fit = x_fit.unite({penalty}, 0);
    y_fit = y_fit.unite({pluss::zeros({design.d, k}, out_cfg)}, 0);
  }

  const Series coeff = pluss::subsp::math::linalg::pinverse(x_fit, options.pinv_rcond).matmul(y_fit);
  const CoeffLayout layout = coeff_layout_(options, k);

  std::int64_t rank = options.rank;
  if (rank < 0) {
    JohansenOptions jopt;
    jopt.lag_order = options.lag_order;
    jopt.include_intercept = options.include_intercept;
    jopt.include_trend = options.include_trend;
    jopt.statistic = JohansenStat::Trace;
    jopt.alpha = options.alpha;
    rank = johansen_test(y, jopt, out_cfg).selected_rank;
  }
  rank = std::clamp<std::int64_t>(rank, 0, k);

  Series constant = options.include_intercept ? coeff.select(0, layout.intercept).reshape({k})
                                              : pluss::zeros({k}, out_cfg);
  Series trend = options.include_trend ? coeff.select(0, layout.trend).reshape({k})
                                       : pluss::zeros({k}, out_cfg);
  Series impact_full = coeff.index_select(0, i64_range_(layout.impact_row0, layout.impact_row0 + k, out_cfg));
  Series impact = reduced_rank_impact_(impact_full, rank, out_cfg);

  Series gamma = pluss::empty({0, k, k}, out_cfg);
  if (options.lag_order > 1) {
    const Series gflat = coeff.index_select(
        0, i64_range_(layout.gamma_row0, layout.gamma_row0 + (options.lag_order - 1) * k, out_cfg));
    gamma = gflat.reshape({options.lag_order - 1, k, k});
  }

  auto [alpha, beta] = alpha_beta_from_impact_(impact, rank, out_cfg);
  const Series resid = design.target.sub(design.design.matmul(coeff_from_fit_(
      VecmFit{
          .lag_order = options.lag_order,
          .rank = rank,
          .include_intercept = options.include_intercept,
          .include_trend = options.include_trend,
          .alpha = alpha,
          .beta = beta,
          .impact = impact,
          .gamma = gamma,
          .constant = constant,
          .trend = trend,
      },
      out_cfg,
      "Vecm::fit_mle")));
  Series covariance = resid.transpose(0, 1).matmul(resid).div(static_cast<double>(std::max<std::int64_t>(1, design.t)));
  covariance = stabilize_cov_(covariance, out_cfg);

  return VecmFit{
      .lag_order = options.lag_order,
      .rank = rank,
      .include_intercept = options.include_intercept,
      .include_trend = options.include_trend,
      .alpha = alpha.to(out_cfg.platform, out_cfg.dtype),
      .beta = beta.to(out_cfg.platform, out_cfg.dtype),
      .impact = impact.to(out_cfg.platform, out_cfg.dtype),
      .gamma = gamma.to(out_cfg.platform, out_cfg.dtype),
      .constant = constant.to(out_cfg.platform, out_cfg.dtype),
      .trend = trend.to(out_cfg.platform, out_cfg.dtype),
      .covariance = covariance.to(out_cfg.platform, out_cfg.dtype),
      .sample_size = design.t,
  };
}

Series VecmFit::infer(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const VecmOptions opt{
      .lag_order = lag_order,
      .rank = rank,
      .include_intercept = include_intercept,
      .include_trend = include_trend,
  };
  const auto design = build_design_(y, lag_order, include_intercept, include_trend, out_cfg, "VecmFit::infer");
  const Series coeff = coeff_from_fit_(*this, out_cfg, "VecmFit::infer");
  return design.target.sub(design.design.matmul(coeff)).to(out_cfg.platform, out_cfg.dtype);
}

Series VecmFit::loglik_per_obs(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series resid = infer(y, out_cfg);
  return per_obs_loglik_(resid, covariance, out_cfg, "VecmFit::loglik_per_obs").to(out_cfg.platform, out_cfg.dtype);
}

double VecmFit::loglik(const Series& y) const {
  return loglik_per_obs(y, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)).sum().item<double>();
}

Series VecmFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::VecmFit::forecast: horizon must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  Series y_hist = as_matrix_(y, "VecmFit::forecast").to(out_cfg.platform, out_cfg.dtype);
  const auto ys = y_hist.sizes();
  const std::int64_t k = ys[1];
  PLUSS_CHECK(k == impact.sizes()[0], "subsp::stats::ts::VecmFit::forecast: response dimension mismatch");
  PLUSS_CHECK(ys[0] >= lag_order, "subsp::stats::ts::VecmFit::forecast: expected at least lag_order observations");

  Series out = pluss::zeros({horizon, k}, out_cfg);
  for (std::int64_t h = 0; h < horizon; ++h) {
    const std::int64_t ncur = y_hist.sizes()[0];
    const Series y_prev = y_hist.select(0, ncur - 1).reshape({1, k});
    Series delta = y_prev.matmul(impact);
    if (include_intercept) {
      delta = delta.add(constant.reshape({1, k}));
    }
    if (include_trend) {
      delta = delta.add(trend.reshape({1, k}).mul(static_cast<double>(ncur - lag_order + 1)));
    }
    for (std::int64_t i = 1; i < lag_order; ++i) {
      const Series y_l1 = y_hist.select(0, ncur - i).reshape({1, k});
      const Series y_l2 = y_hist.select(0, ncur - i - 1).reshape({1, k});
      const Series d_lag = y_l1.sub(y_l2);
      delta = delta.add(d_lag.matmul(gamma.select(0, i - 1)));
    }
    const Series next = y_prev.add(delta);
    const Series idx = pluss::make_series(std::vector<std::int64_t>{h}, {1}, i64_cfg_(out_cfg));
    out = pluss::index_put(out, std::vector<Series>{idx}, next.reshape({k}), false);
    y_hist = y_hist.unite({next}, 0);
  }
  return out.to(out_cfg.platform, out_cfg.dtype);
}

Series VecmFit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  PLUSS_CHECK(num_observations > lag_order + 1,
              "subsp::stats::ts::VecmFit::simulate: num_observations must be > lag_order + 1");
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t k = impact.sizes()[0];

  Series y = pluss::zeros({num_observations, k}, out_cfg);
  const Series cov = stabilize_cov_(covariance, out_cfg);
  const Series chol_t = pluss::subsp::math::linalg::cholesky(cov).transpose(0, 1);
  const Series z = pluss::randn({num_observations, k}, out_cfg, seed);
  const Series eps = z.matmul(chol_t);

  for (std::int64_t t = 1; t < num_observations; ++t) {
    const Series y_prev = y.select(0, t - 1).reshape({1, k});
    Series delta = y_prev.matmul(impact);
    if (include_intercept) {
      delta = delta.add(constant.reshape({1, k}));
    }
    if (include_trend) {
      delta = delta.add(trend.reshape({1, k}).mul(static_cast<double>(t)));
    }
    for (std::int64_t i = 1; i < lag_order; ++i) {
      if (t - i - 1 < 0) {
        continue;
      }
      const Series y_l1 = y.select(0, t - i).reshape({1, k});
      const Series y_l2 = y.select(0, t - i - 1).reshape({1, k});
      delta = delta.add(y_l1.sub(y_l2).matmul(gamma.select(0, i - 1)));
    }
    const Series next = y_prev.add(delta).add(eps.select(0, t).reshape({1, k}));
    const Series idx = pluss::make_series(std::vector<std::int64_t>{t}, {1}, i64_cfg_(out_cfg));
    y = pluss::index_put(y, std::vector<Series>{idx}, next.reshape({k}), false);
  }
  return y.to(out_cfg.platform, out_cfg.dtype);
}

} // namespace pluss::subsp::stats::ts

