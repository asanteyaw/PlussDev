#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kLog2Pi = 1.83787706640934533908;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_matrix_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + op + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + op + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0], 1});
  }
  PLUSS_CHECK(sizes.size() == 2, std::string("subsp::stats::ts::") + op + ": expected 1D or 2D Series");
  PLUSS_CHECK(sizes[0] > 0 && sizes[1] > 0,
              std::string("subsp::stats::ts::") + op + ": expected positive matrix dimensions");
  return x;
}

Series column_(const Series& x, std::int64_t j) {
  return x.select(1, j).reshape({x.sizes()[0]});
}

Series floor_positive_(const Series& x, double eps, Config cfg) {
  const Series s = x.to(cfg.platform, cfg.dtype);
  const Series floor_s = pluss::series::full({}, eps, cfg);
  return s.ge(floor_s).where(s, floor_s);
}

Series stabilize_corr_(const Series& corr_in, double jitter, Config cfg, const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t k = corr_in.sizes()[0];
  const Series eye = pluss::series::eye(k, out_cfg);
  const Series corr = corr_in.to(out_cfg.platform, out_cfg.dtype);

  Series candidate = corr;
  double lam = std::max(0.0, jitter);
  for (std::int64_t attempt = 0; attempt < 8; ++attempt) {
    try {
      (void)candidate.linearalg().cholesky(false);
      return candidate;
    } catch (const std::exception&) {
      if (lam <= 0.0) {
        lam = 1e-10;
      } else {
        lam *= 10.0;
      }
      candidate = corr.add(eye.mul(lam));
    }
  }
  PLUSS_CHECK(false, std::string("subsp::stats::ts::") + op + ": failed to stabilize correlation matrix");
  return eye;
}

Series estimate_corr_(const Series& z, double shrinkage, double jitter, Config cfg, const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series z64 = as_matrix_(z, op).to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = z64.sizes();
  const std::int64_t n = sizes[0];
  const std::int64_t k = sizes[1];
  const Series eye = pluss::series::eye(k, out_cfg);
  if (k == 1 || n <= 1) {
    return eye;
  }

  const Series zc = z64.sub(z64.mean(0, true));
  const Series cov =
      zc.transpose(0, 1).matmul(zc).div(static_cast<double>(std::max<std::int64_t>(1, n - 1)));
  const Series diag = floor_positive_(cov.mul(eye).sum(1, false), 1e-12, out_cfg);
  const Series std = diag.sqrt();
  const Series den = std.reshape({k, 1}).matmul(std.reshape({1, k}));
  Series corr = cov.div(den);
  corr = corr.add(corr.transpose(0, 1)).mul(0.5);
  corr = corr.mul(pluss::series::ones({k, k}, out_cfg).sub(eye)).add(eye);

  const double sh = std::clamp(shrinkage, 0.0, 1.0);
  if (sh > 0.0) {
    corr = corr.mul(1.0 - sh).add(eye.mul(sh));
  }
  return stabilize_corr_(corr, jitter, out_cfg, op);
}

template <class MarginalFit>
Series infer_matrix_(const Series& y, const std::vector<MarginalFit>& marginals, Config cfg, const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, op).to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = ym.sizes();
  const std::int64_t n = sizes[0];
  const std::int64_t k = sizes[1];
  PLUSS_CHECK(static_cast<std::int64_t>(marginals.size()) == k,
              std::string("subsp::stats::ts::") + op + ": marginal count mismatch");

  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>(k));
  for (std::int64_t j = 0; j < k; ++j) {
    const Series yj = column_(ym, j);
    cols.push_back(marginals[static_cast<std::size_t>(j)].infer(yj, out_cfg).reshape({n, 1}));
  }
  if (k == 1) {
    return cols.front();
  }
  return cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
}

template <class MarginalFit>
Series forecast_matrix_(std::int64_t horizon,
                        const Series& y,
                        const std::vector<MarginalFit>& marginals,
                        Config cfg,
                        const char* op) {
  PLUSS_CHECK(horizon > 0, std::string("subsp::stats::ts::") + op + ": horizon must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, op).to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = ym.sizes();
  const std::int64_t k = sizes[1];
  PLUSS_CHECK(static_cast<std::int64_t>(marginals.size()) == k,
              std::string("subsp::stats::ts::") + op + ": marginal count mismatch");

  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>(k));
  for (std::int64_t j = 0; j < k; ++j) {
    const Series yj = column_(ym, j);
    cols.push_back(marginals[static_cast<std::size_t>(j)].forecast(horizon, yj, out_cfg).reshape({horizon, 1}));
  }
  if (k == 1) {
    return cols.front();
  }
  return cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
}

template <class MarginalFit>
Series loglik_per_obs_matrix_(const Series& y,
                              const std::vector<MarginalFit>& marginals,
                              const Series& mean,
                              const Series& corr,
                              Config cfg,
                              const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, op).to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = ym.sizes();
  const std::int64_t n = sizes[0];
  const std::int64_t k = sizes[1];

  PLUSS_CHECK(static_cast<std::int64_t>(marginals.size()) == k,
              std::string("subsp::stats::ts::") + op + ": marginal count mismatch");
  PLUSS_CHECK(mean.defined(), std::string("subsp::stats::ts::") + op + ": mean is undefined");
  PLUSS_CHECK(corr.defined(), std::string("subsp::stats::ts::") + op + ": corr is undefined");
  PLUSS_CHECK(mean.sizes().size() == 1 && mean.sizes()[0] == k,
              std::string("subsp::stats::ts::") + op + ": mean must have shape [k]");
  PLUSS_CHECK(corr.sizes().size() == 2 && corr.sizes()[0] == k && corr.sizes()[1] == k,
              std::string("subsp::stats::ts::") + op + ": corr must have shape [k,k]");

  const Series h = infer_matrix_(ym, marginals, out_cfg, op);
  const Series h_floor = floor_positive_(h, 1e-12, out_cfg);
  const Series mean_row = mean.to(out_cfg.platform, out_cfg.dtype).reshape({1, k});
  const Series resid = ym.sub(mean_row);
  const Series z = resid.div(h_floor.sqrt());

  const Series corr_pd = stabilize_corr_(corr.to(out_cfg.platform, out_cfg.dtype), 1e-9, out_cfg, op);
  const Series corr_inv = corr_pd.linearalg().inverse();
  const Series q = z.matmul(corr_inv).mul(z).sum(1, false);
  const Series logh = h_floor.log().sum(1, false);
  const Series c = pluss::series::full({}, static_cast<double>(k) * kLog2Pi, out_cfg).add(corr_pd.linearalg().logdet());
  return q.add(logh).add(c).mul(-0.5);
}

template <class MarginalFit, class MarginalOptions, class FitFn>
std::pair<std::vector<MarginalFit>, Series> fit_marginals_(const Series& y,
                                                            const MarginalOptions& options,
                                                            Config cfg,
                                                            FitFn&& fit_fn,
                                                            const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, op).to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = ym.sizes();
  const std::int64_t k = sizes[1];

  std::vector<MarginalFit> fits;
  fits.reserve(static_cast<std::size_t>(k));
  std::vector<double> means(static_cast<std::size_t>(k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    const Series yj = column_(ym, j);
    auto fit = fit_fn(yj, options, out_cfg);
    means[static_cast<std::size_t>(j)] = fit.mean;
    fits.push_back(std::move(fit));
  }
  const Series mean = pluss::make_series(means, {k}, out_cfg);
  return {std::move(fits), mean};
}

} // namespace

MvGarchFit MvGarch::fit_mle(const Series& y, const MvGarchOptions& options, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, "MvGarch::fit_mle").to(out_cfg.platform, out_cfg.dtype);
  const auto [marginals, mean] =
      fit_marginals_<GarchFit>(ym, options.marginal, out_cfg, Garch::fit_mle, "MvGarch::fit_mle");
  const Series h = infer_matrix_(ym, marginals, out_cfg, "MvGarch::fit_mle");
  const Series z = ym.sub(mean.reshape({1, ym.sizes()[1]})).div(floor_positive_(h, 1e-12, out_cfg).sqrt());
  const Series corr = estimate_corr_(z, options.corr_shrinkage, options.corr_jitter, out_cfg, "MvGarch::fit_mle");
  return MvGarchFit{.marginals = marginals, .mean = mean, .corr = corr};
}

Series MvGarchFit::infer(const Series& y, Config cfg) const {
  return infer_matrix_(y, marginals, cfg, "MvGarchFit::infer");
}

double MvGarchFit::loglik(const Series& y) const {
  return loglik_per_obs(y).sum().item<double>();
}

Series MvGarchFit::loglik_per_obs(const Series& y, Config cfg) const {
  return loglik_per_obs_matrix_(y, marginals, mean, corr, cfg, "MvGarchFit::loglik_per_obs");
}

Series MvGarchFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  return forecast_matrix_(horizon, y, marginals, cfg, "MvGarchFit::forecast");
}

MvEgarchFit MvEgarch::fit_mle(const Series& y, const MvEgarchOptions& options, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, "MvEgarch::fit_mle").to(out_cfg.platform, out_cfg.dtype);
  const auto [marginals, mean] =
      fit_marginals_<EgarchFit>(ym, options.marginal, out_cfg, Egarch::fit_mle, "MvEgarch::fit_mle");
  const Series h = infer_matrix_(ym, marginals, out_cfg, "MvEgarch::fit_mle");
  const Series z = ym.sub(mean.reshape({1, ym.sizes()[1]})).div(floor_positive_(h, 1e-12, out_cfg).sqrt());
  const Series corr = estimate_corr_(z, options.corr_shrinkage, options.corr_jitter, out_cfg, "MvEgarch::fit_mle");
  return MvEgarchFit{.marginals = marginals, .mean = mean, .corr = corr};
}

Series MvEgarchFit::infer(const Series& y, Config cfg) const {
  return infer_matrix_(y, marginals, cfg, "MvEgarchFit::infer");
}

double MvEgarchFit::loglik(const Series& y) const {
  return loglik_per_obs(y).sum().item<double>();
}

Series MvEgarchFit::loglik_per_obs(const Series& y, Config cfg) const {
  return loglik_per_obs_matrix_(y, marginals, mean, corr, cfg, "MvEgarchFit::loglik_per_obs");
}

Series MvEgarchFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  return forecast_matrix_(horizon, y, marginals, cfg, "MvEgarchFit::forecast");
}

MvGjrFit MvGjr::fit_mle(const Series& y, const MvGjrOptions& options, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ym = as_matrix_(y, "MvGjr::fit_mle").to(out_cfg.platform, out_cfg.dtype);
  const auto [marginals, mean] =
      fit_marginals_<GjrFit>(ym, options.marginal, out_cfg, Gjr::fit_mle, "MvGjr::fit_mle");
  const Series h = infer_matrix_(ym, marginals, out_cfg, "MvGjr::fit_mle");
  const Series z = ym.sub(mean.reshape({1, ym.sizes()[1]})).div(floor_positive_(h, 1e-12, out_cfg).sqrt());
  const Series corr = estimate_corr_(z, options.corr_shrinkage, options.corr_jitter, out_cfg, "MvGjr::fit_mle");
  return MvGjrFit{.marginals = marginals, .mean = mean, .corr = corr};
}

Series MvGjrFit::infer(const Series& y, Config cfg) const {
  return infer_matrix_(y, marginals, cfg, "MvGjrFit::infer");
}

double MvGjrFit::loglik(const Series& y) const {
  return loglik_per_obs(y).sum().item<double>();
}

Series MvGjrFit::loglik_per_obs(const Series& y, Config cfg) const {
  return loglik_per_obs_matrix_(y, marginals, mean, corr, cfg, "MvGjrFit::loglik_per_obs");
}

Series MvGjrFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  return forecast_matrix_(horizon, y, marginals, cfg, "MvGjrFit::forecast");
}

} // namespace pluss::subsp::stats::ts
