#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"

namespace pluss::subsp::math::state {

struct LinearModel {
  Series transition = pluss::series::undefined();
  Series observation = pluss::series::undefined();
  Series process_cov = pluss::series::undefined();
  Series observation_cov = pluss::series::undefined();
  Series initial_mean = pluss::series::undefined();
  Series initial_cov = pluss::series::undefined();

  // Optional input path: x[t] = A x[t-1] + B u[t],
  // y[t] = C x[t] + D u[t].
  Series input = pluss::series::undefined();
  Series state_input = pluss::series::undefined();
  Series obs_input = pluss::series::undefined();
};

struct FilterOptions {
  double info_tol = 0.0;
  double jitter = 1e-12;
};

struct FilterResult {
  Series filtered_mean = pluss::series::undefined();
  Series filtered_cov = pluss::series::undefined();
  Series predicted_mean = pluss::series::undefined();
  Series predicted_cov = pluss::series::undefined();
  Series predicted_obs = pluss::series::undefined();
  Series innovation = pluss::series::undefined();
  Series innovation_cov = pluss::series::undefined();
  Series gain = pluss::series::undefined();
  Series data_used = pluss::series::undefined();
  Series loglik_terms = pluss::series::undefined();
  Series loglik = pluss::series::undefined();
};

struct SmoothResult {
  FilterResult filter;
  Series smoothed_mean = pluss::series::undefined();
  Series smoothed_cov = pluss::series::undefined();
  Series gain = pluss::series::undefined();
};

CODEM_API FilterResult filter(const Series &observations,
                              const LinearModel &model,
                              FilterOptions options = {});

CODEM_API SmoothResult smooth(const Series &observations,
                              const LinearModel &model,
                              FilterOptions options = {});

CODEM_API SmoothResult smooth(const FilterResult &filtered,
                              const LinearModel &model,
                              FilterOptions options = {});

} // namespace pluss::subsp::math::state
