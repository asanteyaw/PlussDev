#pragma once

#include "pluss/codem/series_factory.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pluss::subsp::math {

// Canonical covariance contracts shared by statistics and scientific domains.
// Matrix work remains batched and backend-local; only contract predicates
// reduce to a host boolean.
inline void cov_check(const Series &covariance,
                      std::string_view where = "cov_check") {
  if (!covariance.defined())
    throw std::invalid_argument(std::string(where) +
                                ": expected a defined covariance");
  if (!covariance.dtype().is_floating())
    throw std::invalid_argument(std::string(where) +
                                ": covariance must have a real floating dtype");
  if (covariance.dim() < 2 || covariance.size(-2) != covariance.size(-1))
    throw std::invalid_argument(std::string(where) +
                                ": covariance must have shape [..., n, n]");
  if (!covariance.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(where) +
                                ": covariance values must be finite");
}

[[nodiscard]] inline bool cov_is_sym(const Series &covariance,
                                     double rtol = 1e-6, double atol = 1e-8) {
  cov_check(covariance, "cov_is_sym");
  if (!(rtol >= 0.0) || !std::isfinite(rtol) || !(atol >= 0.0) ||
      !std::isfinite(atol))
    throw std::invalid_argument(
        "cov_is_sym: tolerances must be finite and non-negative");
  return covariance.allclose(covariance.transpose(-2, -1), rtol, atol);
}

[[nodiscard]] inline Series cov_sym(const Series &covariance) {
  cov_check(covariance, "cov_sym");
  return covariance.add(covariance.transpose(-2, -1)).mul(0.5);
}

[[nodiscard]] inline Series cov_diag(const Series &covariance) {
  cov_check(covariance, "cov_diag");
  const auto width = covariance.size(-1);
  const Config cfg = Config{}
                         .with_dtype(covariance.scalar_type())
                         .with_platform(covariance.platform());
  return covariance.mul(pluss::series::eye(width, cfg)).sum(-1, false);
}

[[nodiscard]] inline Series cov_jitter(const Series &covariance,
                                       double jitter) {
  if (!(jitter >= 0.0) || !std::isfinite(jitter))
    throw std::invalid_argument(
        "cov_jitter: jitter must be finite and non-negative");
  const Series symmetric = cov_sym(covariance);
  if (jitter == 0.0)
    return symmetric;
  const auto width = symmetric.size(-1);
  const Config cfg = Config{}
                         .with_dtype(symmetric.scalar_type())
                         .with_platform(symmetric.platform());
  return symmetric.add(pluss::series::eye(width, cfg).mul(jitter));
}

} // namespace pluss::subsp::math
