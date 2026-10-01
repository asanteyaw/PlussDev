#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/finance/filtering.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kLog2Pi = 1.83787706640934533908;
constexpr double kTiny = 1e-12;

struct LoadedSsmModel {
  Series A = pluss::series::undefined();
  Series B = pluss::series::undefined();
  Series C = pluss::series::undefined();
  Series D = pluss::series::undefined();
  Series Q = pluss::series::undefined();
  Series R = pluss::series::undefined();
  Series mean0 = pluss::series::undefined();
  Series cov0 = pluss::series::undefined();
  std::int64_t n_state = 0;
  std::int64_t n_obs = 0;
  std::int64_t n_state_disturb = 0;
  std::int64_t n_obs_innov = 0;
  Config out_cfg{};
};

Config f64_cfg_(Config cfg, Platform fallback_platform) {
  return Config{}.with_dtype(ScalarType::Float64)
      .with_platform(cfg.platform == Platform::CPU && fallback_platform != Platform::CPU ? fallback_platform : cfg.platform)
      .with_pinned(cfg.pinned);
}

Series as_matrix_(const Series& x, const char* op, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + op + ": " + name + " must be defined");
  const auto s = x.sizes();
  PLUSS_CHECK(!s.empty(), std::string("subsp::stats::ts::") + op + ": " + name + " must be non-scalar");
  if (s.size() == 1) {
    return x.reshape({s[0], 1});
  }
  PLUSS_CHECK(s.size() == 2,
              std::string("subsp::stats::ts::") + op + ": " + name + " must be vector or matrix");
  return x;
}

Series as_vector_col_(const Series& x, const char* op, const char* name) {
  const Series m = as_matrix_(x, op, name);
  const auto s = m.sizes();
  PLUSS_CHECK(s[1] == 1 || s[0] == 1,
              std::string("subsp::stats::ts::") + op + ": " + name + " must be a vector");
  return (s[1] == 1) ? m : m.transpose(0, 1);
}

Series force_obs_matrix_(const Series& y, const char* op) {
  PLUSS_CHECK(y.defined(), std::string("subsp::stats::ts::") + op + ": observations must be defined");
  const auto s = y.sizes();
  PLUSS_CHECK(!s.empty(), std::string("subsp::stats::ts::") + op + ": observations must be non-scalar");
  PLUSS_CHECK(s.size() <= 2, std::string("subsp::stats::ts::") + op + ": observations must be [T] or [T,n]");
  if (s.size() == 1) {
    return y.reshape({s[0], 1});
  }
  return y;
}

Series stabilize_cov_(const Series& cov, double jitter, Config cfg) {
  const Series sym = cov.add(cov.transpose(0, 1)).mul(0.5);
  const std::int64_t n = sym.sizes()[0];
  return sym.add(pluss::series::eye(n, cfg).mul(std::max(jitter, 0.0)));
}

Series row_index_(std::int64_t i, Platform platform) {
  return pluss::series::full({1}, i, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

Series index_from_vector_(const std::vector<std::int64_t>& idx, Platform platform) {
  return pluss::make_series(idx, {static_cast<std::int64_t>(idx.size())},
                            pluss::cfg::dtype(ScalarType::Int64),
                            pluss::cfg::platform(platform));
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

std::vector<std::int64_t> informative_obs_idx_(const Series& yt, const Series& forecast_cov, double tol) {
  const std::int64_t n = yt.numel();
  const std::vector<double> yv = yt.to(pluss::pCPU, ScalarType::Float64).reshape({n}).to_vec<double>();
  const std::vector<double> s = forecast_cov.to(pluss::pCPU, ScalarType::Float64).reshape({n * n}).to_vec<double>();
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const double sii = s[static_cast<std::size_t>(i * n + i)];
    if (std::isfinite(yv[static_cast<std::size_t>(i)]) && std::isfinite(sii) && sii > tol) {
      idx.push_back(i);
    }
  }
  return idx;
}

Series data_used_row_(std::int64_t n, const std::vector<std::int64_t>& idx, Config cfg) {
  std::vector<double> used(static_cast<std::size_t>(n), 0.0);
  for (const auto i : idx) {
    if (i >= 0 && i < n) {
      used[static_cast<std::size_t>(i)] = 1.0;
    }
  }
  return pluss::make_series(used, {1, n},
                            pluss::cfg::dtype(cfg.dtype),
                            pluss::cfg::platform(cfg.platform));
}

double innovation_loglik_(const Series& innovation, const Series& forecast_cov, double tol) {
  const auto m = innovation.numel();
  if (m <= 0) {
    return 0.0;
  }
  const Series s_inv = pluss::subsp::math::linalg::pinverse(forecast_cov, 1e-12);
  const double quad = innovation.transpose(0, 1).matmul(s_inv).matmul(innovation).reshape({1}).item<double>();
  const Series slog = pluss::subsp::math::linalg::slogdet(forecast_cov).reshape({2});
  const double sign = slog.select(0, 0).item<double>();
  const double logdet = slog.select(0, 1).item<double>();
  if (sign > 0.0 && std::isfinite(logdet)) {
    return -0.5 * (static_cast<double>(m) * kLog2Pi + logdet + quad);
  }

  const Series singular_values = pluss::subsp::math::linalg::svd(forecast_cov, false).S;
  const std::vector<double> sv =
      singular_values.to(pluss::pCPU, ScalarType::Float64).reshape({singular_values.numel()}).to_vec<double>();
  const double eps = std::max(tol, kTiny);
  std::int64_t rank = 0;
  double pseudo_logdet = 0.0;
  for (double v : sv) {
    if (v > eps && std::isfinite(v)) {
      pseudo_logdet += std::log(v);
      ++rank;
    }
  }
  if (rank == 0) {
    return 0.0;
  }
  return -0.5 * (static_cast<double>(rank) * kLog2Pi + pseudo_logdet + quad);
}

Series slice_row_(const Series& matrix, std::int64_t row, std::int64_t cols, Platform platform) {
  return matrix.index_select(0, row_index_(row, platform)).reshape({cols, 1});
}

Series slice_matrix_from_cube_(const Series& cube,
                               std::int64_t row,
                               std::int64_t r,
                               std::int64_t c,
                               Platform platform) {
  return cube.index_select(0, row_index_(row, platform)).reshape({r, c});
}

LoadedSsmModel load_model_(const SsmModel& model, Config cfg, const char* op) {
  const Platform fallback = model.transition.defined() ? model.transition.platform() : pluss::pCPU;
  const auto out_cfg = f64_cfg_(cfg, fallback);

  const Series A = as_matrix_(model.transition, op, "transition").to(out_cfg.platform, out_cfg.dtype);
  const Series B = as_matrix_(model.state_loading, op, "state_loading").to(out_cfg.platform, out_cfg.dtype);
  const Series C = as_matrix_(model.observation, op, "observation").to(out_cfg.platform, out_cfg.dtype);
  const Series D = as_matrix_(model.innovation_loading, op, "innovation_loading").to(out_cfg.platform, out_cfg.dtype);
  const Series mean0 = as_vector_col_(model.mean0, op, "mean0").to(out_cfg.platform, out_cfg.dtype);
  const Series cov0 = as_matrix_(model.cov0, op, "cov0").to(out_cfg.platform, out_cfg.dtype);

  const auto As = A.sizes();
  const auto Bs = B.sizes();
  const auto Cs = C.sizes();
  const auto Ds = D.sizes();
  PLUSS_CHECK(As.size() == 2 && As[0] == As[1],
              std::string("subsp::stats::ts::") + op + ": transition must be square");
  const std::int64_t m = As[0];
  PLUSS_CHECK(Bs.size() == 2 && Bs[0] == m,
              std::string("subsp::stats::ts::") + op + ": state_loading row count must match transition");
  const std::int64_t k = Bs[1];
  PLUSS_CHECK(Cs.size() == 2 && Cs[1] == m,
              std::string("subsp::stats::ts::") + op + ": observation column count must match transition");
  const std::int64_t n = Cs[0];
  PLUSS_CHECK(Ds.size() == 2 && Ds[0] == n,
              std::string("subsp::stats::ts::") + op + ": innovation_loading row count must match observation");
  const std::int64_t h = Ds[1];
  PLUSS_CHECK(mean0.numel() == m,
              std::string("subsp::stats::ts::") + op + ": mean0 dimension mismatch");
  PLUSS_CHECK(cov0.sizes()[0] == m && cov0.sizes()[1] == m,
              std::string("subsp::stats::ts::") + op + ": cov0 dimension mismatch");

  const Series Q = B.matmul(B.transpose(0, 1));
  const Series R = D.matmul(D.transpose(0, 1));

  return LoadedSsmModel{
      .A = A,
      .B = B,
      .C = C,
      .D = D,
      .Q = Q,
      .R = R,
      .mean0 = mean0,
      .cov0 = cov0,
      .n_state = m,
      .n_obs = n,
      .n_state_disturb = k,
      .n_obs_innov = h,
      .out_cfg = out_cfg,
  };
}

SsmFilterResult run_filter_(const Series& y_in, const SsmModel& model, const SsmOptions& options, Config cfg) {
  const char* op = "Ssm::filter";
  const LoadedSsmModel ssm = load_model_(model, cfg, op);
  const Series y = force_obs_matrix_(y_in, op).to(ssm.out_cfg.platform, ssm.out_cfg.dtype);
  const auto ys = y.sizes();
  PLUSS_CHECK(ys[1] == ssm.n_obs,
              "subsp::stats::ts::Ssm::filter: observation width must match model observation dimension");

  pluss::subsp::math::finance::filtering::KalmanModel kf_model;
  kf_model.transition = ssm.A;
  kf_model.observation = ssm.C;
  kf_model.process_cov = ssm.Q;
  kf_model.observation_cov = ssm.R;
  kf_model.initial_mean = ssm.mean0;
  kf_model.initial_cov = ssm.cov0;
  kf_model.info_tol = std::max(options.tolerance, 0.0);

  const auto kf = pluss::subsp::math::finance::filtering::kalman_filter(y, kf_model, ssm.out_cfg);
  const std::int64_t t_obs = ys[0];

  const Series forecasted_obs = kf.predicted_mean.matmul(ssm.C.transpose(0, 1));
  const Series forecast_obs_error = kf.innovation;

  std::vector<Series> gain_rows;
  std::vector<double> loglik_terms(static_cast<std::size_t>(t_obs), 0.0);
  gain_rows.reserve(static_cast<std::size_t>(t_obs));

  for (std::int64_t t = 0; t < t_obs; ++t) {
    const Series p_pred_t =
        slice_matrix_from_cube_(kf.predicted_cov, t, ssm.n_state, ssm.n_state, ssm.out_cfg.platform);
    const Series s_full_t =
        slice_matrix_from_cube_(kf.innovation_cov, t, ssm.n_obs, ssm.n_obs, ssm.out_cfg.platform);
    const Series err_t = slice_row_(forecast_obs_error, t, ssm.n_obs, ssm.out_cfg.platform);
    const Series used_t = kf.data_used.index_select(0, row_index_(t, ssm.out_cfg.platform)).reshape({ssm.n_obs});
    const std::vector<double> used_v = used_t.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

    std::vector<std::int64_t> obs_idx;
    obs_idx.reserve(static_cast<std::size_t>(ssm.n_obs));
    for (std::int64_t i = 0; i < ssm.n_obs; ++i) {
      if (used_v[static_cast<std::size_t>(i)] > 0.5) {
        obs_idx.push_back(i);
      }
    }

    std::vector<double> gain_full(static_cast<std::size_t>(ssm.n_state * ssm.n_obs), 0.0);
    if (!obs_idx.empty()) {
      const Series idx_obs = index_from_vector_(obs_idx, ssm.out_cfg.platform);
      const std::int64_t n_used = static_cast<std::int64_t>(obs_idx.size());
      const Series c_used = ssm.C.index_select(0, idx_obs).reshape({n_used, ssm.n_state});
      const Series s_used = s_full_t.index_select(0, idx_obs).index_select(1, idx_obs).reshape({n_used, n_used});
      const Series k_used =
          p_pred_t.matmul(c_used.transpose(0, 1)).matmul(pluss::subsp::math::linalg::pinverse(s_used, 1e-12));
      const Series err_used = err_t.index_select(0, idx_obs).reshape({n_used, 1});
      loglik_terms[static_cast<std::size_t>(t)] =
          innovation_loglik_(err_used, s_used, std::max(options.tolerance, 0.0));

      const std::vector<double> kvals =
          k_used.to(pluss::pCPU, ScalarType::Float64).reshape({ssm.n_state * n_used}).to_vec<double>();
      for (std::int64_t r = 0; r < ssm.n_state; ++r) {
        for (std::int64_t c = 0; c < n_used; ++c) {
          const std::int64_t out_c = obs_idx[static_cast<std::size_t>(c)];
          gain_full[static_cast<std::size_t>(r * ssm.n_obs + out_c)] =
              kvals[static_cast<std::size_t>(r * n_used + c)];
        }
      }
    }
    gain_rows.push_back(
        pluss::make_series(gain_full, {1, ssm.n_state, ssm.n_obs},
                           pluss::cfg::dtype(ScalarType::Float64),
                           pluss::cfg::platform(pluss::pCPU))
            .to(ssm.out_cfg.platform, ssm.out_cfg.dtype));
  }

  return SsmFilterResult{
      .filtered_states = kf.filtered_mean,
      .filtered_states_cov = kf.filtered_cov,
      .forecasted_states = kf.predicted_mean,
      .forecasted_states_cov = kf.predicted_cov,
      .forecasted_obs = forecasted_obs,
      .forecasted_obs_cov = kf.innovation_cov,
      .forecast_obs_error = forecast_obs_error,
      .kalman_gain = stack_rows_(gain_rows),
      .data_used = kf.data_used,
      .loglik_per_obs = pluss::make_series(
          loglik_terms, {t_obs},
          pluss::cfg::dtype(ScalarType::Float64),
          pluss::cfg::platform(pluss::pCPU))
                            .to(ssm.out_cfg.platform, ssm.out_cfg.dtype),
      .loglik = kf.loglik,
  };
}

SsmForecastResult run_forecast_with_state_(std::int64_t horizon,
                                           const LoadedSsmModel& ssm,
                                           Series x,
                                           Series p) {
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::Ssm::forecast: horizon must be > 0");

  std::vector<Series> state_rows;
  std::vector<Series> state_cov_rows;
  std::vector<Series> obs_rows;
  std::vector<Series> obs_cov_rows;
  state_rows.reserve(static_cast<std::size_t>(horizon));
  state_cov_rows.reserve(static_cast<std::size_t>(horizon));
  obs_rows.reserve(static_cast<std::size_t>(horizon));
  obs_cov_rows.reserve(static_cast<std::size_t>(horizon));

  for (std::int64_t h = 0; h < horizon; ++h) {
    x = ssm.A.matmul(x);
    p = stabilize_cov_(ssm.A.matmul(p).matmul(ssm.A.transpose(0, 1)).add(ssm.Q), kTiny, ssm.out_cfg);
    const Series y = ssm.C.matmul(x);
    const Series s = stabilize_cov_(ssm.C.matmul(p).matmul(ssm.C.transpose(0, 1)).add(ssm.R), kTiny, ssm.out_cfg);

    state_rows.push_back(x.reshape({1, ssm.n_state}));
    state_cov_rows.push_back(p.reshape({1, ssm.n_state, ssm.n_state}));
    obs_rows.push_back(y.reshape({1, ssm.n_obs}));
    obs_cov_rows.push_back(s.reshape({1, ssm.n_obs, ssm.n_obs}));
  }

  return SsmForecastResult{
      .observations = stack_rows_(obs_rows),
      .observations_cov = stack_rows_(obs_cov_rows),
      .states = stack_rows_(state_rows),
      .states_cov = stack_rows_(state_cov_rows),
  };
}

} // namespace

SsmFilterResult Ssm::filter(const Series& y,
                            const SsmModel& model,
                            const SsmOptions& options,
                            Config cfg) {
  return run_filter_(y, model, options, cfg);
}

SsmSmoothResult Ssm::smooth(const Series& y,
                            const SsmModel& model,
                            const SsmOptions& options,
                            Config cfg) {
  const char* op = "Ssm::smooth";
  const LoadedSsmModel ssm = load_model_(model, cfg, op);
  const SsmFilterResult filt = run_filter_(y, model, options, cfg);

  const std::int64_t t_obs = filt.filtered_states.sizes()[0];
  PLUSS_CHECK(t_obs > 0, "subsp::stats::ts::Ssm::smooth: observations must be non-empty");

  std::vector<Series> xs(
      static_cast<std::size_t>(t_obs), pluss::series::undefined());
  std::vector<Series> ps(
      static_cast<std::size_t>(t_obs), pluss::series::undefined());

  xs[static_cast<std::size_t>(t_obs - 1)] =
      slice_row_(filt.filtered_states, t_obs - 1, ssm.n_state, ssm.out_cfg.platform);
  ps[static_cast<std::size_t>(t_obs - 1)] =
      slice_matrix_from_cube_(filt.filtered_states_cov, t_obs - 1, ssm.n_state, ssm.n_state, ssm.out_cfg.platform);

  for (std::int64_t t = t_obs - 2; t >= 0; --t) {
    const Series xf_t = slice_row_(filt.filtered_states, t, ssm.n_state, ssm.out_cfg.platform);
    const Series pf_t =
        slice_matrix_from_cube_(filt.filtered_states_cov, t, ssm.n_state, ssm.n_state, ssm.out_cfg.platform);
    const Series xp_next = slice_row_(filt.forecasted_states, t + 1, ssm.n_state, ssm.out_cfg.platform);
    const Series pp_next =
        slice_matrix_from_cube_(filt.forecasted_states_cov, t + 1, ssm.n_state, ssm.n_state, ssm.out_cfg.platform);

    const Series j =
        pf_t.matmul(ssm.A.transpose(0, 1)).matmul(pluss::subsp::math::linalg::pinverse(pp_next, 1e-12));
    const Series xs_next = xs[static_cast<std::size_t>(t + 1)];
    const Series ps_next = ps[static_cast<std::size_t>(t + 1)];
    const Series xs_t = xf_t.add(j.matmul(xs_next.sub(xp_next)));
    const Series ps_t =
        stabilize_cov_(pf_t.add(j.matmul(ps_next.sub(pp_next)).matmul(j.transpose(0, 1))), kTiny, ssm.out_cfg);

    xs[static_cast<std::size_t>(t)] = xs_t;
    ps[static_cast<std::size_t>(t)] = ps_t;
  }

  std::vector<Series> x_rows;
  std::vector<Series> p_rows;
  x_rows.reserve(static_cast<std::size_t>(t_obs));
  p_rows.reserve(static_cast<std::size_t>(t_obs));
  for (std::int64_t t = 0; t < t_obs; ++t) {
    x_rows.push_back(xs[static_cast<std::size_t>(t)].reshape({1, ssm.n_state}));
    p_rows.push_back(ps[static_cast<std::size_t>(t)].reshape({1, ssm.n_state, ssm.n_state}));
  }

  return SsmSmoothResult{
      .filter = filt,
      .smoothed_states = stack_rows_(x_rows),
      .smoothed_states_cov = stack_rows_(p_rows),
  };
}

SsmForecastResult Ssm::forecast(std::int64_t horizon, const SsmModel& model, Config cfg) {
  const char* op = "Ssm::forecast";
  const LoadedSsmModel ssm = load_model_(model, cfg, op);
  Series x = ssm.mean0;
  Series p = ssm.cov0;
  return run_forecast_with_state_(horizon, ssm, x, p);
}

SsmForecastResult Ssm::forecast(std::int64_t horizon,
                                const Series& y_history,
                                const SsmModel& model,
                                const SsmOptions& options,
                                Config cfg) {
  const char* op = "Ssm::forecast";
  const LoadedSsmModel ssm = load_model_(model, cfg, op);
  if (!y_history.defined() || y_history.numel() == 0) {
    return run_forecast_with_state_(horizon, ssm, ssm.mean0, ssm.cov0);
  }

  const SsmFilterResult filt = run_filter_(y_history, model, options, cfg);
  const std::int64_t t_obs = filt.filtered_states.sizes()[0];
  Series x = slice_row_(filt.filtered_states, t_obs - 1, ssm.n_state, ssm.out_cfg.platform);
  Series p = slice_matrix_from_cube_(filt.filtered_states_cov, t_obs - 1, ssm.n_state, ssm.n_state, ssm.out_cfg.platform);
  return run_forecast_with_state_(horizon, ssm, x, p);
}

SsmSimResult Ssm::simulate(std::int64_t num_observations,
                           const SsmModel& model,
                           std::int64_t num_paths,
                           Config cfg,
                           std::uint64_t seed) {
  const char* op = "Ssm::simulate";
  PLUSS_CHECK(num_observations > 0, "subsp::stats::ts::Ssm::simulate: num_observations must be > 0");
  PLUSS_CHECK(num_paths > 0, "subsp::stats::ts::Ssm::simulate: num_paths must be > 0");
  const LoadedSsmModel ssm = load_model_(model, cfg, op);

  const Series cov0 = stabilize_cov_(ssm.cov0, kTiny, ssm.out_cfg);
  const Series chol0 = pluss::subsp::math::linalg::cholesky(cov0).to(ssm.out_cfg.platform, ssm.out_cfg.dtype);
  std::uint64_t draw_seed = (seed == 0U) ? 0xA5EED1234ULL : seed;

  std::vector<Series> y_paths;
  std::vector<Series> x_paths;
  std::vector<Series> u_paths;
  std::vector<Series> e_paths;
  y_paths.reserve(static_cast<std::size_t>(num_paths));
  x_paths.reserve(static_cast<std::size_t>(num_paths));
  u_paths.reserve(static_cast<std::size_t>(num_paths));
  e_paths.reserve(static_cast<std::size_t>(num_paths));

  for (std::int64_t p = 0; p < num_paths; ++p) {
    const Series z0 = pluss::series::randn({ssm.n_state, 1}, ssm.out_cfg, draw_seed++);
    Series x = ssm.mean0.add(chol0.matmul(z0));

    std::vector<Series> y_rows;
    std::vector<Series> x_rows;
    std::vector<Series> u_rows;
    std::vector<Series> e_rows;
    y_rows.reserve(static_cast<std::size_t>(num_observations));
    x_rows.reserve(static_cast<std::size_t>(num_observations));
    u_rows.reserve(static_cast<std::size_t>(num_observations));
    e_rows.reserve(static_cast<std::size_t>(num_observations));

    for (std::int64_t t = 0; t < num_observations; ++t) {
      const Series u = pluss::series::randn({ssm.n_state_disturb, 1}, ssm.out_cfg, draw_seed++);
      x = ssm.A.matmul(x).add(ssm.B.matmul(u));

      const Series e = pluss::series::randn({ssm.n_obs_innov, 1}, ssm.out_cfg, draw_seed++);
      const Series y = ssm.C.matmul(x).add(ssm.D.matmul(e));

      x_rows.push_back(x.reshape({1, ssm.n_state}));
      y_rows.push_back(y.reshape({1, ssm.n_obs}));
      u_rows.push_back(u.reshape({1, ssm.n_state_disturb}));
      e_rows.push_back(e.reshape({1, ssm.n_obs_innov}));
    }

    const Series y_path = stack_rows_(y_rows);
    const Series x_path = stack_rows_(x_rows);
    const Series u_path = stack_rows_(u_rows);
    const Series e_path = stack_rows_(e_rows);
    if (num_paths == 1) {
      y_paths.push_back(y_path);
      x_paths.push_back(x_path);
      u_paths.push_back(u_path);
      e_paths.push_back(e_path);
    } else {
      y_paths.push_back(y_path.reshape({1, num_observations, ssm.n_obs}));
      x_paths.push_back(x_path.reshape({1, num_observations, ssm.n_state}));
      u_paths.push_back(u_path.reshape({1, num_observations, ssm.n_state_disturb}));
      e_paths.push_back(e_path.reshape({1, num_observations, ssm.n_obs_innov}));
    }
  }

  return SsmSimResult{
      .observations = stack_rows_(y_paths),
      .states = stack_rows_(x_paths),
      .state_disturbance = stack_rows_(u_paths),
      .obs_innovation = stack_rows_(e_paths),
  };
}

} // namespace pluss::subsp::stats::ts
