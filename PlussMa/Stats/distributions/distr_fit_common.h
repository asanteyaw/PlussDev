#pragma once

#include "distr_object_detail.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::stat::distr::fit_detail {

inline std::vector<double> data_f64(const Series& x, std::string_view op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": x must be defined");
  const auto values = x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  PLUSS_CHECK(!values.empty(), std::string(op) + ": x must contain at least one observation");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return std::isfinite(v); }),
              std::string(op) + ": x must contain finite values only");
  return values;
}

inline double wald_z(double level, std::string_view op) {
  PLUSS_CHECK(level > 0.0 && level < 1.0, std::string(op) + ": level must be in (0, 1)");
  constexpr double kEps = 1e-15;
  const double alpha = 1.0 - level;
  const double p = std::clamp(1.0 - 0.5 * alpha, kEps, 1.0 - kEps);
  return detail::scalar_f64(2.0 * p).erfcinv().mul(-std::sqrt(2.0)).item<double>();
}

} // namespace pluss::subsp::stat::distr::fit_detail
