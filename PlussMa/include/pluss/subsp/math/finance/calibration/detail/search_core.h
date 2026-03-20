#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <utility>

namespace pluss::subsp::math::finance::calibration::detail {

template <std::size_t D>
std::array<double, D> project(std::array<double, D> x,
                              const std::array<double, D>& lo,
                              const std::array<double, D>& hi) {
  for (std::size_t i = 0; i < D; ++i) {
    x[i] = std::clamp(x[i], lo[i], hi[i]);
  }
  return x;
}

template <std::size_t D>
bool same_vec(const std::array<double, D>& a, const std::array<double, D>& b, double tol = 0.0) {
  for (std::size_t i = 0; i < D; ++i) {
    if (std::abs(a[i] - b[i]) > tol) {
      return false;
    }
  }
  return true;
}

template <std::size_t D>
double max_step(const std::array<double, D>& step) {
  double m = 0.0;
  for (double v : step) {
    m = std::max(m, std::abs(v));
  }
  return m;
}

inline double weighted_rmse(double sse, double wsum, std::size_t n) {
  if (n == 0U) {
    return std::numeric_limits<double>::infinity();
  }
  const double denom = (wsum > 0.0) ? wsum : static_cast<double>(n);
  return std::sqrt(std::max(0.0, sse / denom));
}

template <std::size_t D>
std::array<double, D> random_point(std::mt19937_64& rng,
                                   const std::array<double, D>& lo,
                                   const std::array<double, D>& hi) {
  std::array<double, D> x{};
  for (std::size_t i = 0; i < D; ++i) {
    std::uniform_real_distribution<double> dist(lo[i], hi[i]);
    x[i] = dist(rng);
  }
  return x;
}

template <std::size_t D, typename ObjectiveFn>
std::pair<std::array<double, D>, double> pattern_search(const std::array<double, D>& start,
                                                         const std::array<double, D>& lo,
                                                         const std::array<double, D>& hi,
                                                         std::int64_t max_iterations,
                                                         double step_fraction,
                                                         double step_decay,
                                                         double tolerance,
                                                         ObjectiveFn&& objective,
                                                         std::int64_t& evaluations,
                                                         bool& converged) {
  std::array<double, D> x = project(start, lo, hi);
  std::array<double, D> step{};
  for (std::size_t i = 0; i < D; ++i) {
    step[i] = std::max(tolerance * 10.0, (hi[i] - lo[i]) * step_fraction);
  }

  double best = objective(x);
  ++evaluations;

  for (std::int64_t it = 0; it < max_iterations; ++it) {
    bool improved = false;
    for (std::size_t d = 0; d < D; ++d) {
      std::array<double, D> plus = x;
      plus[d] = std::min(hi[d], x[d] + step[d]);
      if (!same_vec(plus, x)) {
        const double val = objective(plus);
        ++evaluations;
        if (val < best) {
          best = val;
          x = plus;
          improved = true;
          continue;
        }
      }

      std::array<double, D> minus = x;
      minus[d] = std::max(lo[d], x[d] - step[d]);
      if (!same_vec(minus, x)) {
        const double val = objective(minus);
        ++evaluations;
        if (val < best) {
          best = val;
          x = minus;
          improved = true;
        }
      }
    }

    if (!improved) {
      for (double& s : step) {
        s *= step_decay;
      }
      if (max_step(step) <= tolerance) {
        converged = true;
        break;
      }
    }
  }

  if (max_step(step) <= tolerance) {
    converged = true;
  }
  return {x, best};
}

} // namespace pluss::subsp::math::finance::calibration::detail
