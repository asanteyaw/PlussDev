#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/covariance.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/math/state.h"

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

Series stabilize_cov_(const Series& cov, double jitter) {
  return pluss::subsp::math::cov_jitter(cov, jitter);
}

Series row_index_(std::int64_t i, Platform platform) {
  return pluss::series::full({1}, i, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
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

pluss::subsp::math::state::LinearModel shared_model_(const LoadedSsmModel& ssm) {
  return {
      .transition = ssm.A,
      .observation = ssm.C,
      .process_cov = ssm.Q,
      .observation_cov = ssm.R,
      .initial_mean = ssm.mean0,
      .initial_cov = ssm.cov0,
  };
}

SsmFilterResult ssm_filter_(
    const pluss::subsp::math::state::FilterResult& result,
    const LoadedSsmModel& ssm) {
  return SsmFilterResult{
      .filtered_states = result.filtered_mean,
      .filtered_states_cov = result.filtered_cov,
      .forecasted_states = result.predicted_mean,
      .forecasted_states_cov = result.predicted_cov,
      .forecasted_obs = result.predicted_obs,
      .forecasted_obs_cov = result.innovation_cov,
      .forecast_obs_error = result.innovation,
      .kalman_gain = result.gain,
      .data_used = result.data_used.to(ssm.out_cfg.dtype),
      .loglik_per_obs = result.loglik_terms,
      .loglik = result.loglik.item<double>(),
  };
}

SsmFilterResult run_filter_(const Series& y_in, const SsmModel& model,
                            const SsmOptions& options, Config cfg) {
  const char* op = "Ssm::filter";
  const LoadedSsmModel ssm = load_model_(model, cfg, op);
  const Series y =
      force_obs_matrix_(y_in, op).to(ssm.out_cfg.platform, ssm.out_cfg.dtype);
  PLUSS_CHECK(y.size(-1) == ssm.n_obs,
              "subsp::stats::ts::Ssm::filter: observation width must match model observation dimension");
  const auto result = pluss::subsp::math::state::filter(
      y, shared_model_(ssm),
      {.info_tol = std::max(options.tolerance, 0.0), .jitter = kTiny});
  return ssm_filter_(result, ssm);
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
    p = stabilize_cov_(ssm.A.matmul(p).matmul(ssm.A.transpose(0, 1)).add(ssm.Q), kTiny);
    const Series y = ssm.C.matmul(x);
    const Series s = stabilize_cov_(ssm.C.matmul(p).matmul(ssm.C.transpose(0, 1)).add(ssm.R), kTiny);

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
  const Series observations =
      force_obs_matrix_(y, op).to(ssm.out_cfg.platform, ssm.out_cfg.dtype);
  PLUSS_CHECK(observations.size(-1) == ssm.n_obs,
              "subsp::stats::ts::Ssm::smooth: observation width must match model observation dimension");
  const auto result = pluss::subsp::math::state::smooth(
      observations, shared_model_(ssm),
      {.info_tol = std::max(options.tolerance, 0.0), .jitter = kTiny});
  return SsmSmoothResult{
      .filter = ssm_filter_(result.filter, ssm),
      .smoothed_states = result.smoothed_mean,
      .smoothed_states_cov = result.smoothed_cov,
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

  const Series cov0 = stabilize_cov_(ssm.cov0, kTiny);
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
