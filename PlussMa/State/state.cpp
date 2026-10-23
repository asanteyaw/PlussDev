#include "pluss/subsp/math/state.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/covariance.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::math::state {
namespace {

constexpr double kLog2Pi = 1.83787706640934548356;

Config config_for(const Series &value) {
  return Config{}
      .with_dtype(value.scalar_type())
      .with_platform(value.platform());
}

void require_real(const Series &value, std::string_view name) {
  PLUSS_CHECK(value.defined(),
              "math::state: " + std::string(name) + " must be defined");
  PLUSS_CHECK(value.dtype().is_floating(),
              "math::state: " + std::string(name) +
                  " must have a real floating dtype");
}

void require_same_storage(const Series &value, const Series &reference,
                          std::string_view name) {
  require_real(value, name);
  PLUSS_CHECK(value.platform() == reference.platform(),
              "math::state: " + std::string(name) +
                  " must share the model platform");
  PLUSS_CHECK(value.scalar_type() == reference.scalar_type(),
              "math::state: " + std::string(name) +
                  " must share the model dtype");
}

void require_matrix(const Series &value, const Series &reference,
                    std::string_view name) {
  require_same_storage(value, reference, name);
  PLUSS_CHECK(value.dim() >= 2 && value.size(-2) > 0 && value.size(-1) > 0,
              "math::state: " + std::string(name) +
                  " must have non-empty matrix axes");
  PLUSS_CHECK(value.isfinite().all().item<bool>(),
              "math::state: " + std::string(name) +
                  " must contain only finite values");
}

void require_cov(const Series &value, const Series &reference,
                 std::string_view name) {
  require_matrix(value, reference, name);
  PLUSS_CHECK(value.size(-2) == value.size(-1),
              "math::state: " + std::string(name) + " must be square");
  const double tol = value.scalar_type() == ScalarType::Float64 ? 1e-10 : 2e-5;
  PLUSS_CHECK(value.allclose(value.transpose(-2, -1), tol, tol),
              "math::state: " + std::string(name) + " must be symmetric");
  const double min_eigenvalue =
      value.linearalg.eigvalsh().amin().item<double>();
  PLUSS_CHECK(std::isfinite(min_eigenvalue) && min_eigenvalue >= -tol,
              "math::state: " + std::string(name) +
                  " must be positive semidefinite");
}

Series as_observations(const Series &value) {
  require_real(value, "observations");
  PLUSS_CHECK(value.dim() >= 1,
              "math::state: observations must have a time axis");
  return value.dim() == 1 ? value.unsqueeze(-1) : value;
}

Series as_mean_column(const Series &value, std::int64_t states,
                      const Series &reference) {
  require_same_storage(value, reference, "initial_mean");
  PLUSS_CHECK(value.dim() >= 1,
              "math::state: initial_mean must have a state axis");
  if (value.dim() >= 2 && value.size(-2) == states && value.size(-1) == 1)
    return value;
  PLUSS_CHECK(
      value.size(-1) == states,
      "math::state: initial_mean must have shape [..., n] or [..., n, 1]");
  return value.unsqueeze(-1);
}

Series symmetrize(const Series &value) {
  return value.add(value.transpose(-2, -1)).mul(0.5);
}

Series stabilize(const Series &value, double jitter) {
  const Series symmetric = symmetrize(value);
  if (jitter == 0.0)
    return symmetric;
  return symmetric.add(
      pluss::series::eye(symmetric.size(-1), config_for(symmetric))
          .mul(jitter));
}

Series stack(const std::vector<Series> &parts, std::int64_t axis) {
  PLUSS_CHECK(!parts.empty(), "math::state: cannot stack an empty trajectory");
  if (parts.size() == 1)
    return parts.front();
  return parts.front().unite(
      std::vector<Series>(parts.begin() + 1, parts.end()), axis, true);
}

Series solve_spd(const Series &matrix, const Series &rhs) {
  const Series factor = matrix.linearalg.cholesky(false);
  return factor.linearalg.cholesky_solve(rhs, false).to(matrix.scalar_type());
}

Series right_solve_spd(const Series &rhs, const Series &matrix) {
  return solve_spd(matrix, rhs.transpose(-2, -1))
      .transpose(-2, -1)
      .to(rhs.scalar_type());
}

void validate_options(FilterOptions options) {
  PLUSS_CHECK(std::isfinite(options.info_tol) && options.info_tol >= 0.0,
              "math::state: info_tol must be finite and non-negative");
  PLUSS_CHECK(std::isfinite(options.jitter) && options.jitter >= 0.0,
              "math::state: jitter must be finite and non-negative");
}

struct LoadedModel {
  Series a;
  Series c;
  Series q;
  Series r;
  Series x;
  Series p;
  Series input;
  Series b;
  Series d;
  std::int64_t states{};
  std::int64_t observations{};
  std::int64_t steps{};
  bool has_input{};
};

LoadedModel load(const Series &observations, const LinearModel &model,
                 FilterOptions options) {
  validate_options(options);
  const Series y = as_observations(observations);
  require_real(model.transition, "transition");
  require_matrix(model.transition, model.transition, "transition");
  PLUSS_CHECK(model.transition.size(-2) == model.transition.size(-1),
              "math::state: transition must be square");

  const std::int64_t n = model.transition.size(-1);
  const std::int64_t m = y.size(-1);
  PLUSS_CHECK(
      y.size(-2) > 0 && m > 0,
      "math::state: observations must have shape [..., T, m] with T,m > 0");
  require_same_storage(y, model.transition, "observations");
  require_matrix(model.observation, model.transition, "observation");
  require_cov(model.process_cov, model.transition, "process_cov");
  require_cov(model.observation_cov, model.transition, "observation_cov");
  require_cov(model.initial_cov, model.transition, "initial_cov");
  PLUSS_CHECK(model.observation.size(-2) == m &&
                  model.observation.size(-1) == n,
              "math::state: observation must have shape [..., m, n]");
  PLUSS_CHECK(model.process_cov.size(-2) == n,
              "math::state: process_cov must have shape [..., n, n]");
  PLUSS_CHECK(model.observation_cov.size(-2) == m,
              "math::state: observation_cov must have shape [..., m, m]");
  PLUSS_CHECK(model.initial_cov.size(-2) == n,
              "math::state: initial_cov must have shape [..., n, n]");
  const Series x = as_mean_column(model.initial_mean, n, model.transition);
  PLUSS_CHECK(x.isfinite().all().item<bool>(),
              "math::state: initial_mean must contain only finite values");

  const bool any_input = model.input.defined() || model.state_input.defined() ||
                         model.obs_input.defined();
  Series input = pluss::series::undefined();
  Series b = pluss::series::undefined();
  Series d = pluss::series::undefined();
  if (any_input) {
    PLUSS_CHECK(model.input.defined() && model.state_input.defined(),
                "math::state: input and state_input must be supplied together");
    require_same_storage(model.input, model.transition, "input");
    PLUSS_CHECK(model.input.isfinite().all().item<bool>(),
                "math::state: input must contain only finite values");
    require_matrix(model.state_input, model.transition, "state_input");
    PLUSS_CHECK(model.input.dim() >= 2 && model.input.size(-2) == y.size(-2),
                "math::state: input must have shape [..., T, k]");
    const std::int64_t k = model.input.size(-1);
    PLUSS_CHECK(model.state_input.size(-2) == n &&
                    model.state_input.size(-1) == k,
                "math::state: state_input must have shape [..., n, k]");
    input = model.input;
    b = model.state_input;
    if (model.obs_input.defined()) {
      require_matrix(model.obs_input, model.transition, "obs_input");
      PLUSS_CHECK(model.obs_input.size(-2) == m &&
                      model.obs_input.size(-1) == k,
                  "math::state: obs_input must have shape [..., m, k]");
      d = model.obs_input;
    } else {
      d = pluss::series::zeros({m, k}, config_for(model.transition));
    }
  }

  return {model.transition,
          model.observation,
          model.process_cov,
          model.observation_cov,
          x,
          model.initial_cov,
          input,
          b,
          d,
          n,
          m,
          y.size(-2),
          any_input};
}

} // namespace

FilterResult filter(const Series &observations, const LinearModel &model,
                    FilterOptions options) {
  const Series y = as_observations(observations);
  LoadedModel loaded = load(y, model, options);
  Series x = loaded.x;
  Series p = loaded.p;
  const Config cfg = config_for(loaded.a);
  const Series state_eye = pluss::series::eye(loaded.states, cfg);
  const Series obs_eye = pluss::series::eye(loaded.observations, cfg);
  const Series transition_t = loaded.a.transpose(-2, -1);

  std::vector<Series> filtered_means;
  std::vector<Series> filtered_covs;
  std::vector<Series> predicted_means;
  std::vector<Series> predicted_covs;
  std::vector<Series> predicted_obs;
  std::vector<Series> innovations;
  std::vector<Series> innovation_covs;
  std::vector<Series> gains;
  std::vector<Series> data_used;
  std::vector<Series> loglik_terms;
  const auto count = static_cast<std::size_t>(loaded.steps);
  filtered_means.reserve(count);
  filtered_covs.reserve(count);
  predicted_means.reserve(count);
  predicted_covs.reserve(count);
  predicted_obs.reserve(count);
  innovations.reserve(count);
  innovation_covs.reserve(count);
  gains.reserve(count);
  data_used.reserve(count);
  loglik_terms.reserve(count);

  for (std::int64_t t = 0; t < loaded.steps; ++t) {
    const Series observed = y.select(-2, t).unsqueeze(-1);
    const Series batch_zero =
        observed.isfinite().to(loaded.a.scalar_type()).sum(-2, true).mul(0.0);
    Series x_pred = loaded.a.matmul(x).add(batch_zero);
    Series input_t = pluss::series::undefined();
    if (loaded.has_input) {
      input_t = loaded.input.select(-2, t).unsqueeze(-1);
      x_pred = x_pred.add(loaded.b.matmul(input_t));
    }
    const Series p_pred = stabilize(
        loaded.a.matmul(p).matmul(transition_t).add(loaded.q).add(batch_zero),
        options.jitter);
    Series y_pred = loaded.c.matmul(x_pred);
    if (loaded.has_input)
      y_pred = y_pred.add(loaded.d.matmul(input_t));

    const Series innovation = observed.sub(y_pred);
    const Series s_full = symmetrize(loaded.c.matmul(p_pred)
                                         .matmul(loaded.c.transpose(-2, -1))
                                         .add(loaded.r));
    const Series forecast_diag =
        s_full.mul(obs_eye).sum(-1, false).unsqueeze(-1);
    const Series mask = observed.isfinite()
                            .logical_and(forecast_diag.isfinite())
                            .logical_and(forecast_diag.gt(options.info_tol));
    const Series mask_value = mask.to(loaded.a.scalar_type());
    const Series mask_outer = mask_value.matmul(mask_value.transpose(-2, -1));
    const Series c_used = loaded.c.mul(mask_value);
    const Series missing_diag =
        obs_eye.mul(mask_value.mul(-1.0).add(1.0).transpose(-2, -1));
    const Series jitter_diag =
        obs_eye.mul(mask_value.mul(options.jitter).transpose(-2, -1));
    const Series r_used = loaded.r.mul(mask_outer).add(missing_diag);
    const Series s_used = symmetrize(c_used.matmul(p_pred)
                                         .matmul(c_used.transpose(-2, -1))
                                         .add(r_used)
                                         .add(jitter_diag));
    const Series safe_innovation =
        mask.where(innovation, pluss::series::zeros(innovation.sizes(), cfg));
    const Series cross = p_pred.matmul(c_used.transpose(-2, -1));
    const Series factor = s_used.linearalg.cholesky(false);
    const Series gain =
        factor.linearalg.cholesky_solve(cross.transpose(-2, -1), false)
            .transpose(-2, -1)
            .to(cross.scalar_type());

    x = x_pred.add(gain.matmul(safe_innovation));
    const Series residual_map = state_eye.sub(gain.matmul(c_used));
    p = stabilize(residual_map.matmul(p_pred)
                      .matmul(residual_map.transpose(-2, -1))
                      .add(gain.matmul(r_used).matmul(gain.transpose(-2, -1))),
                  options.jitter);

    const Series solved_innovation =
        factor.linearalg.cholesky_solve(safe_innovation, false)
            .to(safe_innovation.scalar_type());
    const Series quadratic = safe_innovation.transpose(-2, -1)
                                 .matmul(solved_innovation)
                                 .squeeze(-1)
                                 .squeeze(-1);
    const Series factor_diag = factor.mul(obs_eye).sum(-1, false);
    const Series logdet = factor_diag.log().sum(-1, false).mul(2.0);
    const Series used_count = mask_value.sum(-2, false).squeeze(-1);
    const Series term =
        used_count.mul(kLog2Pi).add(logdet).add(quadratic).mul(-0.5);

    predicted_means.push_back(x_pred.squeeze(-1).unsqueeze(-2));
    predicted_covs.push_back(p_pred.unsqueeze(-3));
    predicted_obs.push_back(y_pred.squeeze(-1).unsqueeze(-2));
    innovations.push_back(innovation.squeeze(-1).unsqueeze(-2));
    innovation_covs.push_back(s_full.unsqueeze(-3));
    gains.push_back(gain.unsqueeze(-3));
    data_used.push_back(mask.squeeze(-1).unsqueeze(-2));
    filtered_means.push_back(x.squeeze(-1).unsqueeze(-2));
    filtered_covs.push_back(p.unsqueeze(-3));
    loglik_terms.push_back(term.unsqueeze(-1));
  }

  FilterResult result;
  result.filtered_mean = stack(filtered_means, -2);
  result.filtered_cov = stack(filtered_covs, -3);
  result.predicted_mean = stack(predicted_means, -2);
  result.predicted_cov = stack(predicted_covs, -3);
  result.predicted_obs = stack(predicted_obs, -2);
  result.innovation = stack(innovations, -2);
  result.innovation_cov = stack(innovation_covs, -3);
  result.gain = stack(gains, -3);
  result.data_used = stack(data_used, -2);
  result.loglik_terms = stack(loglik_terms, -1);
  result.loglik = result.loglik_terms.sum(-1, false);
  return result;
}

SmoothResult smooth(const FilterResult &filtered, const LinearModel &model,
                    FilterOptions options) {
  validate_options(options);
  PLUSS_CHECK(
      filtered.filtered_mean.defined() && filtered.filtered_cov.defined() &&
          filtered.predicted_mean.defined() && filtered.predicted_cov.defined(),
      "math::state::smooth: filtered trajectory is incomplete");
  const std::int64_t steps = filtered.filtered_mean.size(-2);
  const std::int64_t states = filtered.filtered_mean.size(-1);
  PLUSS_CHECK(steps > 0, "math::state::smooth: trajectory must be non-empty");
  require_matrix(model.transition, filtered.filtered_mean, "transition");
  PLUSS_CHECK(
      model.transition.size(-2) == states &&
          model.transition.size(-1) == states,
      "math::state::smooth: transition shape does not match the trajectory");

  std::vector<Series> means(static_cast<std::size_t>(steps));
  std::vector<Series> covs(static_cast<std::size_t>(steps));
  std::vector<Series> gains;
  if (steps > 1)
    gains.resize(static_cast<std::size_t>(steps - 1));
  const Series transition_t = model.transition.transpose(-2, -1);
  means.back() = filtered.filtered_mean.select(-2, steps - 1).unsqueeze(-1);
  covs.back() = filtered.filtered_cov.select(-3, steps - 1);

  for (std::int64_t t = steps - 2; t >= 0; --t) {
    const Series filtered_mean =
        filtered.filtered_mean.select(-2, t).unsqueeze(-1);
    const Series filtered_cov = filtered.filtered_cov.select(-3, t);
    const Series next_predicted_mean =
        filtered.predicted_mean.select(-2, t + 1).unsqueeze(-1);
    const Series next_predicted_cov = filtered.predicted_cov.select(-3, t + 1);
    const Series cross = filtered_cov.matmul(transition_t);
    const Series gain = right_solve_spd(cross, next_predicted_cov);
    means[static_cast<std::size_t>(t)] = filtered_mean.add(gain.matmul(
        means[static_cast<std::size_t>(t + 1)].sub(next_predicted_mean)));
    covs[static_cast<std::size_t>(t)] = stabilize(
        filtered_cov.add(gain.matmul(covs[static_cast<std::size_t>(t + 1)].sub(
                                         next_predicted_cov))
                             .matmul(gain.transpose(-2, -1))),
        options.jitter);
    gains[static_cast<std::size_t>(t)] = gain.unsqueeze(-3);
  }

  std::vector<Series> mean_rows;
  std::vector<Series> cov_rows;
  mean_rows.reserve(static_cast<std::size_t>(steps));
  cov_rows.reserve(static_cast<std::size_t>(steps));
  for (std::int64_t t = 0; t < steps; ++t) {
    mean_rows.push_back(
        means[static_cast<std::size_t>(t)].squeeze(-1).unsqueeze(-2));
    cov_rows.push_back(covs[static_cast<std::size_t>(t)].unsqueeze(-3));
  }

  SmoothResult result;
  result.filter = filtered;
  result.smoothed_mean = stack(mean_rows, -2);
  result.smoothed_cov = stack(cov_rows, -3);
  result.gain = gains.empty() ? pluss::series::undefined() : stack(gains, -3);
  return result;
}

SmoothResult smooth(const Series &observations, const LinearModel &model,
                    FilterOptions options) {
  return smooth(filter(observations, model, options), model, options);
}

} // namespace pluss::subsp::math::state
