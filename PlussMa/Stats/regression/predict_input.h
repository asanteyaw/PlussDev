#pragma once

#include "CodeM/core/Errors.h"
#include "pluss/codem/codem.h"

#include <cstdint>
#include <string>

namespace pluss::subsp::stats::regression::detail {

// The prediction matrix for a fit. Inputs are read as for fitting (a vector is one column, a [1, c] row with c > 1 is one column of c
// values) with one exception: for a fit with two or more predictors, a [1, c] row whose width is the number of predictors (or the width
// of the full design, including its leading column of ones) is a single observation, since a single new observation is the usual case
// in prediction and could not otherwise be expressed.
inline pluss::Series prediction_matrix(const pluss::Series& x,
                                       std::int64_t num_predictors,
                                       std::int64_t k_params,
                                       const char* op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": expected defined design matrix");
  const auto s = x.sizes();
  PLUSS_CHECK(!s.empty(), std::string(op) + ": expected non-scalar design matrix");
  if (s.size() == 1) {
    return x.reshape({s[0], 1});
  }
  PLUSS_CHECK(s.size() == 2, std::string(op) + ": expected design matrix to be 1D or 2D");
  if (s[0] == 1 && s[1] > 1) {
    const bool single_observation = num_predictors >= 2 && (s[1] == num_predictors || s[1] == k_params);
    if (!single_observation) {
      return x.reshape({s[1], 1});
    }
  }
  return x;
}

} // namespace pluss::subsp::stats::regression::detail
