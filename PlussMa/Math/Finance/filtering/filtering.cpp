#include "pluss/subsp/math/finance/filtering.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/math/state.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::filtering {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kLog2Pi = 1.8378770664093453;
constexpr double kTiny = 1e-12;

Config f64_cfg_(Config cfg, Platform fallback_platform) {
  return Config{}.with_dtype(ScalarType::Float64)
      .with_platform(cfg.platform == Platform::CPU && fallback_platform != Platform::CPU ? fallback_platform : cfg.platform)
      .with_pinned(cfg.pinned);
}

Series as_matrix_(const Series& x, const char* op, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::filtering::") + op + ": " + name + " must be defined");
  const auto s = x.sizes();
  PLUSS_CHECK(!s.empty(), std::string("subsp::math::finance::filtering::") + op + ": " + name + " must be non-scalar");
  if (s.size() == 1) {
    return x.reshape({s[0], 1});
  }
  PLUSS_CHECK(s.size() == 2,
              std::string("subsp::math::finance::filtering::") + op + ": " + name + " must be vector or matrix");
  return x;
}

Series as_vector_col_(const Series& x, const char* op, const char* name) {
  const Series m = as_matrix_(x, op, name);
  const auto s = m.sizes();
  PLUSS_CHECK(s[1] == 1 || s[0] == 1,
              std::string("subsp::math::finance::filtering::") + op + ": " + name + " must be a vector");
  return (s[1] == 1) ? m : m.transpose(0, 1);
}

Series stabilize_cov_(const Series& cov, double jitter, Config cfg) {
  const Series sym = cov.add(cov.transpose(0, 1)).mul(0.5);
  return sym.add(pluss::series::eye(sym.sizes()[0], cfg).mul(std::max(jitter, 0.0)));
}

Series stack_rows_(const std::vector<Series>& rows) {
  if (rows.empty()) {
    return pluss::series::undefined();
  }
  if (rows.size() == 1) {
    return rows.front();
  }
  return rows.front().unite(std::vector<Series>(rows.begin() + 1, rows.end()), 0);
}

Series row_index_(std::int64_t i, Platform platform) {
  return pluss::series::full({1}, i, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

Series index_from_vector_(const std::vector<std::int64_t>& idx, Platform platform) {
  return pluss::make_series(idx, {static_cast<std::int64_t>(idx.size())},
                            Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

double innovation_loglik_(const Series& innovation, const Series& innovation_cov, double info_tol) {
  const auto m = innovation.numel();
  if (m <= 0) {
    return 0.0;
  }
  const Series s_inv = pluss::subsp::math::linalg::pinverse(innovation_cov, 1e-12);
  const double quad = innovation.transpose(0, 1).matmul(s_inv).matmul(innovation).reshape({1}).host<double>().front();
  const Series slog = pluss::subsp::math::linalg::slogdet(innovation_cov).reshape({2});
  const double sign = slog.select(0, 0).item<double>();
  const double logdet = slog.select(0, 1).item<double>();
  if (sign > 0.0 && std::isfinite(logdet)) {
    return -0.5 * (static_cast<double>(m) * kLog2Pi + logdet + quad);
  }

  const Series singular_values = pluss::subsp::math::linalg::svd(innovation_cov, false).S;
  const std::vector<double> sv = singular_values.to(pluss::pCPU, ScalarType::Float64).reshape({singular_values.numel()}).host<double>();
  const double tol = std::max(info_tol, kTiny);
  std::int64_t rank = 0;
  double pseudo_logdet = 0.0;
  for (double v : sv) {
    if (v > tol && std::isfinite(v)) {
      pseudo_logdet += std::log(v);
      ++rank;
    }
  }
  if (rank == 0) {
    return 0.0;
  }
  const double log_norm = static_cast<double>(rank) * kLog2Pi + pseudo_logdet;
  return -0.5 * (log_norm + quad);
}

std::vector<std::int64_t> informative_obs_idx_(const Series& yt, const Series& innovation_cov, double info_tol) {
  const auto m = yt.numel();
  const std::vector<double> yv = yt.to(pluss::pCPU, ScalarType::Float64).reshape({m}).host<double>();
  const std::vector<double> s_flat =
      innovation_cov.to(pluss::pCPU, ScalarType::Float64).reshape({m * m}).host<double>();
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(m));
  for (std::int64_t i = 0; i < m; ++i) {
    const double sii = s_flat[static_cast<std::size_t>(i * m + i)];
    if (std::isfinite(yv[static_cast<std::size_t>(i)]) && std::isfinite(sii) && sii > info_tol) {
      idx.push_back(i);
    }
  }
  return idx;
}

Series data_used_row_(std::int64_t m, const std::vector<std::int64_t>& obs_idx, Config cfg) {
  std::vector<double> used(static_cast<std::size_t>(m), 0.0);
  for (const auto i : obs_idx) {
    if (i >= 0 && i < m) {
      used[static_cast<std::size_t>(i)] = 1.0;
    }
  }
  return pluss::make_series(used, {1, m}, pluss::cfg::dtype(cfg.dtype), pluss::cfg::platform(cfg.platform));
}

void validate_observations_(const Series& y, const char* op) {
  PLUSS_CHECK(y.defined(), std::string("subsp::math::finance::filtering::") + op + ": observations must be defined");
  const auto s = y.sizes();
  PLUSS_CHECK(!s.empty(), std::string("subsp::math::finance::filtering::") + op + ": observations must be non-scalar");
  PLUSS_CHECK(s.size() <= 2,
              std::string("subsp::math::finance::filtering::") + op + ": observations must be T-vector or [T,m]");
}

Series force_obs_matrix_(const Series& y) {
  const auto s = y.sizes();
  if (s.size() == 1) {
    return y.reshape({s[0], 1});
  }
  return y;
}

Series make_weights_(std::int64_t k, double w0, double wi, Config cfg) {
  Series w = pluss::series::full({k, 1}, wi, cfg);
  if (k > 0) {
    const Series idx0 = row_index_(0, cfg.platform);
    w = pluss::index_put(w, std::vector<Series>{idx0, idx0}, pluss::series::full({1}, w0, cfg), false);
  }
  return w;
}

Series sigma_points_(const Series& mean, const Series& cov, const UkfOptions& opt, Config cfg) {
  const std::int64_t n = mean.numel();
  const double lambda = opt.alpha * opt.alpha * (static_cast<double>(n) + opt.kappa) - static_cast<double>(n);
  const double scale = static_cast<double>(n) + lambda;

  const Series p_scaled = stabilize_cov_(cov.mul(scale), opt.jitter, cfg);
  const Series root = pluss::subsp::math::linalg::cholesky(p_scaled).to(cfg.platform, cfg.dtype);
  const Series x = mean.reshape({n});

  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(2 * n + 1));
  rows.push_back(x.reshape({1, n}));
  for (std::int64_t i = 0; i < n; ++i) {
    const Series delta = root.transpose(0, 1).select(0, i).reshape({n});
    rows.push_back(x.add(delta).reshape({1, n}));
    rows.push_back(x.sub(delta).reshape({1, n}));
  }
  return stack_rows_(rows);
}

Series weighted_mean_(const Series& rows, const Series& w) {
  // rows: [k,d], w: [k,1]
  return rows.transpose(0, 1).matmul(w);
}

Series weighted_cov_(const Series& rows, const Series& mean_col, const Series& w, const Series& add_cov) {
  // rows: [k,d], mean_col: [d,1], w: [k,1]
  const std::int64_t d = mean_col.numel();
  const Series centered = rows.sub(mean_col.reshape({1, d}));
  return centered.transpose(0, 1).matmul(centered.mul(w)).add(add_cov);
}

std::vector<double> normalized_cdf_(const Series& weights, std::int64_t n) {
  const std::vector<double> w = weights.to(pluss::pCPU, ScalarType::Float64).reshape({n}).host<double>();
  std::vector<double> cdf(static_cast<std::size_t>(n), 0.0);
  double run = 0.0;
  for (std::int64_t i = 0; i < n; ++i) {
    run += std::max(0.0, w[static_cast<std::size_t>(i)]);
    cdf[static_cast<std::size_t>(i)] = run;
  }
  const double norm = std::max(kTiny, cdf.back());
  for (double& v : cdf) {
    v /= norm;
  }
  return cdf;
}

Series systematic_resample_idx_(const Series& weights, std::int64_t n, Platform platform, std::uint64_t seed) {
  const std::vector<double> cdf = normalized_cdf_(weights, n);
  std::mt19937_64 gen(seed);
  std::uniform_real_distribution<double> unif(0.0, 1.0 / static_cast<double>(n));
  const double u0 = unif(gen);

  std::vector<std::int64_t> idx(static_cast<std::size_t>(n), 0);
  std::int64_t j = 0;
  for (std::int64_t i = 0; i < n; ++i) {
    const double u = u0 + static_cast<double>(i) / static_cast<double>(n);
    while (j < n - 1 && u > cdf[static_cast<std::size_t>(j)]) {
      ++j;
    }
    idx[static_cast<std::size_t>(i)] = j;
  }
  return pluss::make_series(idx, {n}, pluss::cfg::dtype(ScalarType::Int64), pluss::cfg::platform(platform));
}

Series multinomial_resample_idx_(const Series& weights, std::int64_t n, Platform platform, std::uint64_t seed) {
  const std::vector<double> cdf = normalized_cdf_(weights, n);
  std::mt19937_64 gen(seed);
  std::uniform_real_distribution<double> unif(0.0, 1.0);

  std::vector<std::int64_t> idx(static_cast<std::size_t>(n), 0);
  for (std::int64_t i = 0; i < n; ++i) {
    const double u = unif(gen);
    const auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
    idx[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(std::distance(cdf.begin(), it));
  }
  return pluss::make_series(idx, {n}, pluss::cfg::dtype(ScalarType::Int64), pluss::cfg::platform(platform));
}

Series stratified_resample_idx_(const Series& weights, std::int64_t n, Platform platform, std::uint64_t seed) {
  const std::vector<double> cdf = normalized_cdf_(weights, n);
  std::mt19937_64 gen(seed);
  std::uniform_real_distribution<double> unif(0.0, 1.0);

  std::vector<std::int64_t> idx(static_cast<std::size_t>(n), 0);
  for (std::int64_t i = 0; i < n; ++i) {
    const double u = (static_cast<double>(i) + unif(gen)) / static_cast<double>(n);
    const auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
    idx[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(std::distance(cdf.begin(), it));
  }
  return pluss::make_series(idx, {n}, pluss::cfg::dtype(ScalarType::Int64), pluss::cfg::platform(platform));
}

Series residual_resample_idx_(const Series& weights, std::int64_t n, Platform platform, std::uint64_t seed) {
  const std::vector<double> w = weights.to(pluss::pCPU, ScalarType::Float64).reshape({n}).host<double>();
  std::vector<std::int64_t> idx(static_cast<std::size_t>(n), 0);
  std::vector<double> residual(static_cast<std::size_t>(n), 0.0);

  std::int64_t filled = 0;
  for (std::int64_t i = 0; i < n; ++i) {
    const double expected = static_cast<double>(n) * std::max(0.0, w[static_cast<std::size_t>(i)]);
    const std::int64_t copies = static_cast<std::int64_t>(std::floor(expected));
    residual[static_cast<std::size_t>(i)] = expected - static_cast<double>(copies);
    for (std::int64_t c = 0; c < copies && filled < n; ++c) {
      idx[static_cast<std::size_t>(filled++)] = i;
    }
  }

  const std::int64_t remaining = n - filled;
  if (remaining > 0) {
    const Series residual_series = pluss::make_series(residual, {n}, pluss::cfg::dtype(ScalarType::Float64), pluss::cfg::platform(pluss::pCPU));
    const std::vector<double> cdf = normalized_cdf_(residual_series, n);
    std::mt19937_64 gen(seed);
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    for (std::int64_t k = 0; k < remaining; ++k) {
      const double u = unif(gen);
      const auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
      idx[static_cast<std::size_t>(filled + k)] = static_cast<std::int64_t>(std::distance(cdf.begin(), it));
    }
  }

  return pluss::make_series(idx, {n}, pluss::cfg::dtype(ScalarType::Int64), pluss::cfg::platform(platform));
}

Series resample_idx_(const Series& weights,
                     std::int64_t n,
                     Platform platform,
                     ParticleResampleMethod method,
                     std::uint64_t seed) {
  switch (method) {
    case ParticleResampleMethod::Systematic:
      return systematic_resample_idx_(weights, n, platform, seed);
    case ParticleResampleMethod::Multinomial:
      return multinomial_resample_idx_(weights, n, platform, seed);
    case ParticleResampleMethod::Stratified:
      return stratified_resample_idx_(weights, n, platform, seed);
    case ParticleResampleMethod::Residual:
      return residual_resample_idx_(weights, n, platform, seed);
  }
  return systematic_resample_idx_(weights, n, platform, seed);
}

bool has_any_finite_(const Series& x) {
  const auto vals = x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
  return std::any_of(vals.begin(), vals.end(), [](double v) { return std::isfinite(v); });
}

} // namespace

KalmanResult kalman_filter(const Series& observations, const KalmanModel& model, Config cfg) {
  const char* op = "kalman_filter";
  validate_observations_(observations, op);
  const Series y = force_obs_matrix_(observations);
  const auto out_cfg = f64_cfg_(cfg, y.platform());
  const Series yv = y.to(out_cfg.platform, out_cfg.dtype);
  pluss::subsp::math::state::LinearModel shared_model{
      .transition = as_matrix_(model.transition, op, "transition")
                        .to(out_cfg.platform, out_cfg.dtype),
      .observation = as_matrix_(model.observation, op, "observation")
                         .to(out_cfg.platform, out_cfg.dtype),
      .process_cov = as_matrix_(model.process_cov, op, "process_cov")
                         .to(out_cfg.platform, out_cfg.dtype),
      .observation_cov =
          as_matrix_(model.observation_cov, op, "observation_cov")
              .to(out_cfg.platform, out_cfg.dtype),
      .initial_mean = as_vector_col_(model.initial_mean, op, "initial_mean")
                          .to(out_cfg.platform, out_cfg.dtype),
      .initial_cov = as_matrix_(model.initial_cov, op, "initial_cov")
                         .to(out_cfg.platform, out_cfg.dtype),
  };
  if (model.control.defined() || model.control_matrix.defined()) {
    shared_model.input = as_matrix_(model.control, op, "control")
                             .to(out_cfg.platform, out_cfg.dtype);
    shared_model.state_input =
        as_matrix_(model.control_matrix, op, "control_matrix")
            .to(out_cfg.platform, out_cfg.dtype);
  }
  const auto shared = pluss::subsp::math::state::filter(
      yv, shared_model,
      {.info_tol = std::max(model.info_tol, 0.0), .jitter = kTiny});
  return KalmanResult{
      .filtered_mean = shared.filtered_mean,
      .filtered_cov = shared.filtered_cov,
      .predicted_mean = shared.predicted_mean,
      .predicted_cov = shared.predicted_cov,
      .innovation = shared.innovation,
      .innovation_cov = shared.innovation_cov,
      .data_used = shared.data_used.to(out_cfg.dtype),
      .loglik = shared.loglik.item<double>(),
  };
}

KalmanResult ekf_filter(const Series& observations, const EkfModel& model, Config cfg) {
  const char* op = "ekf_filter";
  validate_observations_(observations, op);
  PLUSS_CHECK(static_cast<bool>(model.transition), "subsp::math::finance::filtering::ekf_filter: transition function missing");
  PLUSS_CHECK(static_cast<bool>(model.observation), "subsp::math::finance::filtering::ekf_filter: observation function missing");
  PLUSS_CHECK(static_cast<bool>(model.transition_jacobian),
              "subsp::math::finance::filtering::ekf_filter: transition_jacobian missing");
  PLUSS_CHECK(static_cast<bool>(model.observation_jacobian),
              "subsp::math::finance::filtering::ekf_filter: observation_jacobian missing");

  const Series y = force_obs_matrix_(observations);
  const auto out_cfg = f64_cfg_(cfg, y.platform());
  const Series yv = y.to(out_cfg.platform, out_cfg.dtype);
  const auto ys = yv.sizes();
  const std::int64_t t_obs = ys[0];
  const std::int64_t m = ys[1];

  const Series Q = as_matrix_(model.process_cov, op, "process_cov").to(out_cfg.platform, out_cfg.dtype);
  const Series R = as_matrix_(model.observation_cov, op, "observation_cov").to(out_cfg.platform, out_cfg.dtype);
  Series x = as_vector_col_(model.initial_mean, op, "initial_mean").to(out_cfg.platform, out_cfg.dtype);
  Series P = as_matrix_(model.initial_cov, op, "initial_cov").to(out_cfg.platform, out_cfg.dtype);

  const std::int64_t n = x.numel();
  const double info_tol = std::max(model.info_tol, 0.0);
  PLUSS_CHECK(Q.sizes()[0] == n && Q.sizes()[1] == n,
              "subsp::math::finance::filtering::ekf_filter: process_cov shape mismatch");
  PLUSS_CHECK(R.sizes()[0] == m && R.sizes()[1] == m,
              "subsp::math::finance::filtering::ekf_filter: observation_cov shape mismatch");
  PLUSS_CHECK(P.sizes()[0] == n && P.sizes()[1] == n,
              "subsp::math::finance::filtering::ekf_filter: initial_cov shape mismatch");

  const Series I = pluss::series::eye(n, out_cfg);

  std::vector<Series> pred_means;
  std::vector<Series> pred_covs;
  std::vector<Series> filt_means;
  std::vector<Series> filt_covs;
  std::vector<Series> innovs;
  std::vector<Series> innov_covs;
  std::vector<Series> data_used_rows;
  pred_means.reserve(static_cast<std::size_t>(t_obs));
  pred_covs.reserve(static_cast<std::size_t>(t_obs));
  filt_means.reserve(static_cast<std::size_t>(t_obs));
  filt_covs.reserve(static_cast<std::size_t>(t_obs));
  innovs.reserve(static_cast<std::size_t>(t_obs));
  innov_covs.reserve(static_cast<std::size_t>(t_obs));
  data_used_rows.reserve(static_cast<std::size_t>(t_obs));

  double ll = 0.0;
  for (std::int64_t t = 0; t < t_obs; ++t) {
    const Series x_pred = as_vector_col_(model.transition(x, t), op, "transition output").to(out_cfg.platform, out_cfg.dtype);
    const Series F = as_matrix_(model.transition_jacobian(x, t), op, "transition_jacobian output").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(F.sizes()[0] == n && F.sizes()[1] == n,
                "subsp::math::finance::filtering::ekf_filter: transition_jacobian shape mismatch");

    const Series P_pred = stabilize_cov_(F.matmul(P).matmul(F.transpose(0, 1)).add(Q), kTiny, out_cfg);

    const Series y_hat = as_vector_col_(model.observation(x_pred, t), op, "observation output").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(y_hat.numel() == m,
                "subsp::math::finance::filtering::ekf_filter: observation output dimension mismatch");
    const Series H = as_matrix_(model.observation_jacobian(x_pred, t), op, "observation_jacobian output").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(H.sizes()[0] == m && H.sizes()[1] == n,
                "subsp::math::finance::filtering::ekf_filter: observation_jacobian shape mismatch");

    const Series idx_t = row_index_(t, out_cfg.platform);
    const Series yt = yv.index_select(0, idx_t).reshape({m, 1});
    const Series innovation = yt.sub(y_hat);
    const Series S_full = stabilize_cov_(H.matmul(P_pred).matmul(H.transpose(0, 1)).add(R), kTiny, out_cfg);
    const std::vector<std::int64_t> obs_idx = informative_obs_idx_(yt, S_full, info_tol);

    if (!obs_idx.empty()) {
      const Series idx_obs = index_from_vector_(obs_idx, out_cfg.platform);
      const std::int64_t m_used = static_cast<std::int64_t>(obs_idx.size());

      const Series H_used = H.index_select(0, idx_obs).reshape({m_used, n});
      const Series R_used = R.index_select(0, idx_obs).index_select(1, idx_obs).reshape({m_used, m_used});
      const Series y_used = yt.index_select(0, idx_obs).reshape({m_used, 1});
      const Series y_hat_used = y_hat.index_select(0, idx_obs).reshape({m_used, 1});
      const Series innovation_used = y_used.sub(y_hat_used);
      const Series S_used = stabilize_cov_(H_used.matmul(P_pred).matmul(H_used.transpose(0, 1)).add(R_used), kTiny, out_cfg);
      const Series S_inv = pluss::subsp::math::linalg::pinverse(S_used, 1e-12);
      const Series K = P_pred.matmul(H_used.transpose(0, 1)).matmul(S_inv);

      x = x_pred.add(K.matmul(innovation_used));
      const Series KH = K.matmul(H_used);
      P = stabilize_cov_((I.sub(KH)).matmul(P_pred).matmul(I.sub(KH).transpose(0, 1))
                             .add(K.matmul(R_used).matmul(K.transpose(0, 1))),
                         kTiny,
                         out_cfg);
      ll += innovation_loglik_(innovation_used, S_used, info_tol);
    } else {
      x = x_pred;
      P = P_pred;
    }

    pred_means.push_back(x_pred.reshape({1, n}));
    pred_covs.push_back(P_pred.reshape({1, n, n}));
    filt_means.push_back(x.reshape({1, n}));
    filt_covs.push_back(P.reshape({1, n, n}));
    innovs.push_back(innovation.reshape({1, m}));
    innov_covs.push_back(S_full.reshape({1, m, m}));
    data_used_rows.push_back(data_used_row_(m, obs_idx, out_cfg));
  }

  return KalmanResult{
      .filtered_mean = stack_rows_(filt_means),
      .filtered_cov = stack_rows_(filt_covs),
      .predicted_mean = stack_rows_(pred_means),
      .predicted_cov = stack_rows_(pred_covs),
      .innovation = stack_rows_(innovs),
      .innovation_cov = stack_rows_(innov_covs),
      .data_used = stack_rows_(data_used_rows),
      .loglik = ll,
  };
}

KalmanResult ukf_filter(const Series& observations, const UkfModel& model, Config cfg) {
  const char* op = "ukf_filter";
  validate_observations_(observations, op);
  PLUSS_CHECK(static_cast<bool>(model.transition), "subsp::math::finance::filtering::ukf_filter: transition function missing");
  PLUSS_CHECK(static_cast<bool>(model.observation), "subsp::math::finance::filtering::ukf_filter: observation function missing");

  const Series y = force_obs_matrix_(observations);
  const auto out_cfg = f64_cfg_(cfg, y.platform());
  const Series yv = y.to(out_cfg.platform, out_cfg.dtype);
  const auto ys = yv.sizes();
  const std::int64_t t_obs = ys[0];
  const std::int64_t m = ys[1];

  const Series Q = as_matrix_(model.process_cov, op, "process_cov").to(out_cfg.platform, out_cfg.dtype);
  const Series R = as_matrix_(model.observation_cov, op, "observation_cov").to(out_cfg.platform, out_cfg.dtype);
  Series x = as_vector_col_(model.initial_mean, op, "initial_mean").to(out_cfg.platform, out_cfg.dtype);
  Series P = as_matrix_(model.initial_cov, op, "initial_cov").to(out_cfg.platform, out_cfg.dtype);

  const std::int64_t n = x.numel();
  const double info_tol = std::max(model.info_tol, 0.0);
  PLUSS_CHECK(Q.sizes()[0] == n && Q.sizes()[1] == n,
              "subsp::math::finance::filtering::ukf_filter: process_cov shape mismatch");
  PLUSS_CHECK(R.sizes()[0] == m && R.sizes()[1] == m,
              "subsp::math::finance::filtering::ukf_filter: observation_cov shape mismatch");
  PLUSS_CHECK(P.sizes()[0] == n && P.sizes()[1] == n,
              "subsp::math::finance::filtering::ukf_filter: initial_cov shape mismatch");

  const std::int64_t k_sigma = 2 * n + 1;
  PLUSS_CHECK(std::isfinite(model.options.alpha) && model.options.alpha > 0.0 && model.options.alpha <= 1.0,
              "subsp::math::finance::filtering::ukf_filter: alpha must be in (0,1]");
  PLUSS_CHECK(std::isfinite(model.options.beta) && model.options.beta >= 0.0,
              "subsp::math::finance::filtering::ukf_filter: beta must be >= 0");
  PLUSS_CHECK(std::isfinite(model.options.kappa) && model.options.kappa >= 0.0 && model.options.kappa <= 3.0,
              "subsp::math::finance::filtering::ukf_filter: kappa must be in [0,3]");
  PLUSS_CHECK(std::isfinite(model.options.jitter) && model.options.jitter >= 0.0,
              "subsp::math::finance::filtering::ukf_filter: jitter must be >= 0");
  const double lambda = model.options.alpha * model.options.alpha * (static_cast<double>(n) + model.options.kappa) -
                        static_cast<double>(n);
  const double scale = static_cast<double>(n) + lambda;
  PLUSS_CHECK(scale > 0.0, "subsp::math::finance::filtering::ukf_filter: invalid alpha/kappa combination");

  const double w0m = lambda / scale;
  const double wi = 0.5 / scale;
  const double w0c = w0m + (1.0 - model.options.alpha * model.options.alpha + model.options.beta);
  const Series w_m = make_weights_(k_sigma, w0m, wi, out_cfg);
  const Series w_c = make_weights_(k_sigma, w0c, wi, out_cfg);

  std::vector<Series> pred_means;
  std::vector<Series> pred_covs;
  std::vector<Series> filt_means;
  std::vector<Series> filt_covs;
  std::vector<Series> innovs;
  std::vector<Series> innov_covs;
  std::vector<Series> data_used_rows;
  pred_means.reserve(static_cast<std::size_t>(t_obs));
  pred_covs.reserve(static_cast<std::size_t>(t_obs));
  filt_means.reserve(static_cast<std::size_t>(t_obs));
  filt_covs.reserve(static_cast<std::size_t>(t_obs));
  innovs.reserve(static_cast<std::size_t>(t_obs));
  innov_covs.reserve(static_cast<std::size_t>(t_obs));
  data_used_rows.reserve(static_cast<std::size_t>(t_obs));

  double ll = 0.0;
  for (std::int64_t t = 0; t < t_obs; ++t) {
    const Series sigma = sigma_points_(x, P, model.options, out_cfg); // [k,n]

    std::vector<Series> x_rows;
    x_rows.reserve(static_cast<std::size_t>(k_sigma));
    for (std::int64_t i = 0; i < k_sigma; ++i) {
      const Series xi = sigma.select(0, i).reshape({n, 1});
      x_rows.push_back(as_vector_col_(model.transition(xi, t), op, "transition output").reshape({1, n}));
    }
    const Series x_sigma_pred = stack_rows_(x_rows).to(out_cfg.platform, out_cfg.dtype); // [k,n]
    const Series x_pred = weighted_mean_(x_sigma_pred, w_m); // [n,1]
    const Series P_pred = stabilize_cov_(weighted_cov_(x_sigma_pred, x_pred, w_c, Q), kTiny, out_cfg);

    std::vector<Series> z_rows;
    z_rows.reserve(static_cast<std::size_t>(k_sigma));
    for (std::int64_t i = 0; i < k_sigma; ++i) {
      const Series xi = x_sigma_pred.select(0, i).reshape({n, 1});
      z_rows.push_back(as_vector_col_(model.observation(xi, t), op, "observation output").reshape({1, m}));
    }
    const Series z_sigma = stack_rows_(z_rows).to(out_cfg.platform, out_cfg.dtype); // [k,m]
    const Series z_pred = weighted_mean_(z_sigma, w_m); // [m,1]
    const Series S_full = stabilize_cov_(weighted_cov_(z_sigma, z_pred, w_c, R), kTiny, out_cfg);

    const Series x_centered = x_sigma_pred.sub(x_pred.reshape({1, n}));
    const Series z_centered = z_sigma.sub(z_pred.reshape({1, m}));
    const Series Pxz = x_centered.transpose(0, 1).matmul(z_centered.mul(w_c)); // [n,m]

    const Series idx_t = row_index_(t, out_cfg.platform);
    const Series yt = yv.index_select(0, idx_t).reshape({m, 1});
    const Series innovation = yt.sub(z_pred);
    const std::vector<std::int64_t> obs_idx = informative_obs_idx_(yt, S_full, info_tol);

    if (!obs_idx.empty()) {
      const Series idx_obs = index_from_vector_(obs_idx, out_cfg.platform);
      const std::int64_t m_used = static_cast<std::int64_t>(obs_idx.size());
      const Series z_sigma_used = z_sigma.index_select(1, idx_obs).reshape({k_sigma, m_used});
      const Series z_pred_used = z_pred.index_select(0, idx_obs).reshape({m_used, 1});
      const Series R_used = R.index_select(0, idx_obs).index_select(1, idx_obs).reshape({m_used, m_used});
      const Series yt_used = yt.index_select(0, idx_obs).reshape({m_used, 1});
      const Series innovation_used = yt_used.sub(z_pred_used);
      const Series S_used = stabilize_cov_(weighted_cov_(z_sigma_used, z_pred_used, w_c, R_used), kTiny, out_cfg);
      const Series Pxz_used = Pxz.index_select(1, idx_obs).reshape({n, m_used});
      const Series K = Pxz_used.matmul(pluss::subsp::math::linalg::pinverse(S_used, 1e-12));

      x = x_pred.add(K.matmul(innovation_used));
      P = stabilize_cov_(P_pred.sub(K.matmul(S_used).matmul(K.transpose(0, 1))), kTiny, out_cfg);
      ll += innovation_loglik_(innovation_used, S_used, info_tol);
    } else {
      x = x_pred;
      P = P_pred;
    }

    pred_means.push_back(x_pred.reshape({1, n}));
    pred_covs.push_back(P_pred.reshape({1, n, n}));
    filt_means.push_back(x.reshape({1, n}));
    filt_covs.push_back(P.reshape({1, n, n}));
    innovs.push_back(innovation.reshape({1, m}));
    innov_covs.push_back(S_full.reshape({1, m, m}));
    data_used_rows.push_back(data_used_row_(m, obs_idx, out_cfg));
  }

  return KalmanResult{
      .filtered_mean = stack_rows_(filt_means),
      .filtered_cov = stack_rows_(filt_covs),
      .predicted_mean = stack_rows_(pred_means),
      .predicted_cov = stack_rows_(pred_covs),
      .innovation = stack_rows_(innovs),
      .innovation_cov = stack_rows_(innov_covs),
      .data_used = stack_rows_(data_used_rows),
      .loglik = ll,
  };
}

ParticleFilterResult particle_filter(const Series& observations, const ParticleFilterModel& model, Config cfg) {
  const char* op = "particle_filter";
  validate_observations_(observations, op);
  PLUSS_CHECK(static_cast<bool>(model.transition),
              "subsp::math::finance::filtering::particle_filter: transition function missing");
  PLUSS_CHECK(static_cast<bool>(model.loglik),
              "subsp::math::finance::filtering::particle_filter: loglik function missing");

  const Series y = force_obs_matrix_(observations);
  const auto out_cfg = f64_cfg_(cfg, y.platform());
  const Series yv = y.to(out_cfg.platform, out_cfg.dtype);
  const auto ys = yv.sizes();
  const std::int64_t t_obs = ys[0];
  const std::int64_t m = ys[1];

  Series particles = as_matrix_(model.initial_particles, op, "initial_particles").to(out_cfg.platform, out_cfg.dtype);
  const auto ps = particles.sizes();
  PLUSS_CHECK(ps.size() == 2,
              "subsp::math::finance::filtering::particle_filter: initial_particles must be [N,n]");
  const std::int64_t n_particles = ps[0];
  const std::int64_t n_state = ps[1];
  PLUSS_CHECK(n_particles > 1 && n_state > 0,
              "subsp::math::finance::filtering::particle_filter: initial_particles must have N>1 and n>0");

  Series weights = pluss::series::undefined();
  if (model.initial_weights.defined()) {
    weights = as_vector_col_(model.initial_weights, op, "initial_weights").reshape({n_particles}).to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(weights.numel() == n_particles,
                "subsp::math::finance::filtering::particle_filter: initial_weights length mismatch");
  } else {
    weights = pluss::series::full({n_particles}, 1.0 / static_cast<double>(n_particles), out_cfg);
  }
  weights = weights.clamp_min(0.0);
  weights = weights.div(weights.sum().clamp_min(kTiny));

  std::vector<Series> mean_rows;
  std::vector<Series> weight_rows;
  std::vector<Series> particle_rows;
  std::vector<Series> ess_rows;
  mean_rows.reserve(static_cast<std::size_t>(t_obs));
  weight_rows.reserve(static_cast<std::size_t>(t_obs));
  if (model.options.store_particles) {
    particle_rows.reserve(static_cast<std::size_t>(t_obs));
  }
  ess_rows.reserve(static_cast<std::size_t>(t_obs));

  const double ess_cut = std::clamp(model.options.ess_threshold, 0.0, 1.0) * static_cast<double>(n_particles);
  const std::int64_t interval = std::max<std::int64_t>(1, model.options.resample_interval);
  const std::uint64_t seed_base =
      (model.options.resample_seed == 0U) ? 0xD00D1234ULL : model.options.resample_seed;
  double ll = 0.0;

  for (std::int64_t t = 0; t < t_obs; ++t) {
    particles = as_matrix_(model.transition(particles, t), op, "transition output").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(particles.sizes()[0] == n_particles && particles.sizes()[1] == n_state,
                "subsp::math::finance::filtering::particle_filter: transition output shape mismatch");

    const Series idx_t = row_index_(t, out_cfg.platform);
    const Series yt = yv.index_select(0, idx_t).reshape({m, 1});
    if (has_any_finite_(yt)) {
      const Series logw_inc = as_vector_col_(model.loglik(particles, yt, t), op, "loglik output")
                                  .reshape({n_particles})
                                  .to(out_cfg.platform, out_cfg.dtype);
      PLUSS_CHECK(logw_inc.numel() == n_particles,
                  "subsp::math::finance::filtering::particle_filter: loglik output length mismatch");

      const Series logw = weights.clamp_min(kTiny).log().add(logw_inc);
      const Series max_logw = logw.max();
      const Series w_unnorm = logw.sub(max_logw).exp();
      const Series sum_w = w_unnorm.sum().clamp_min(kTiny);
      weights = w_unnorm.div(sum_w);

      ll += std::log(sum_w.item<double>()) + max_logw.item<double>();
    }

    const Series mean = particles.transpose(0, 1).matmul(weights.reshape({n_particles, 1})).reshape({1, n_state});
    const Series ess = weights.square().sum().clamp_min(kTiny).reciprocal().reshape({1});

    mean_rows.push_back(mean);
    weight_rows.push_back(weights.reshape({1, n_particles}));
    ess_rows.push_back(ess);
    if (model.options.store_particles) {
      particle_rows.push_back(particles.reshape({1, n_particles, n_state}));
    }

    bool do_resample = false;
    if (model.options.resample) {
      if (model.options.resample_trigger == ParticleResampleTrigger::Interval) {
        do_resample = ((t + 1) % interval) == 0;
      } else {
        do_resample = ess.host<double>().front() < ess_cut;
      }
    }

    if (do_resample) {
      const Series idx = resample_idx_(weights,
                                       n_particles,
                                       out_cfg.platform,
                                       model.options.resample_method,
                                       seed_base + static_cast<std::uint64_t>(t + 1));
      particles = particles.index_select(0, idx);
      weights = pluss::series::full({n_particles}, 1.0 / static_cast<double>(n_particles), out_cfg);
    }
  }

  return ParticleFilterResult{
      .filtered_mean = stack_rows_(mean_rows),
      .particle_weights = stack_rows_(weight_rows),
      .particles = model.options.store_particles ? stack_rows_(particle_rows) : pluss::series::undefined(),
      .ess = stack_rows_(ess_rows).reshape({t_obs}),
      .loglik = ll,
  };
}

ParticleFilterResult sv_particle_filter(const Series& log_returns,
                                        const volatility::SvParams& params,
                                        std::int64_t num_particles,
                                        double dt,
                                        double ess_threshold,
                                        Config cfg,
                                        std::uint64_t seed) {
  const char* op = "sv_particle_filter";
  validate_observations_(log_returns, op);
  PLUSS_CHECK(num_particles > 1,
              "subsp::math::finance::filtering::sv_particle_filter: num_particles must be > 1");
  PLUSS_CHECK(std::isfinite(dt) && dt > 0.0,
              "subsp::math::finance::filtering::sv_particle_filter: dt must be finite and > 0");

  const Series y = force_obs_matrix_(log_returns);
  const auto out_cfg = f64_cfg_(cfg, y.platform());
  const Series yv = y.to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(yv.sizes()[1] == 1,
              "subsp::math::finance::filtering::sv_particle_filter: log_returns must be univariate");

  const double phi = params.phi;
  const double sigma_h = params.vol_of_logvar;
  const double mean_h = params.logvar_mean;
  const double init_std = sigma_h / std::sqrt(std::max(kTiny, 1.0 - phi * phi));

  const std::uint64_t base_seed = (seed == 0U) ? 0xA1C3D5E7ULL : seed;
  Series particles0 = pluss::series::randn({num_particles, 1}, out_cfg, base_seed)
                          .mul(init_std)
                          .add(params.logvar0);

  ParticleFilterModel model;
  model.initial_particles = particles0;
  model.options.ess_threshold = ess_threshold;
  model.options.resample = true;
  model.options.store_particles = true;

  model.transition = [=](const Series& particles, std::int64_t t) {
    const Series eps = pluss::series::randn({num_particles, 1}, out_cfg, base_seed + 4099U * static_cast<std::uint64_t>(t + 1));
    return particles.mul(phi)
        .add(mean_h * (1.0 - phi))
        .add(eps.mul(sigma_h * std::sqrt(dt)));
  };

  model.loglik = [=](const Series& particles, const Series& obs, std::int64_t) {
    const Series h = particles.reshape({num_particles, 1});
    const Series v = h.exp().mul(dt).clamp_min(params.variance_floor);
    const Series mu = pluss::series::full({num_particles, 1}, params.mu * dt, out_cfg).sub(v.mul(0.5));
    const Series r = obs.reshape({1, 1});
    const Series resid = r.sub(mu);
    return v.log().add(kLog2Pi).add(resid.square().div(v)).mul(-0.5).reshape({num_particles});
  };

  return particle_filter(yv, model, out_cfg);
}

} // namespace pluss::subsp::math::finance::filtering
