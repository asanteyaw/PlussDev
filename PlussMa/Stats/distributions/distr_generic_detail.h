#pragma once

#include "pluss/subsp/stats/distr.h"

#include <vector>

namespace pluss::subsp::stats::distr::detail {

// Sum of the log-density of `x` at `params` (minus infinity when the family rejects them).
double generic_loglik_(dst::Kind kind, const std::vector<double>& params, const pluss::Series& x);

// Inverse of the observed information at `params`, by central differences; empty when it is not positive definite.
std::vector<double> generic_vcov_(dst::Kind kind, const std::vector<double>& params, const pluss::Series& x);

}  // namespace pluss::subsp::stats::distr::detail
