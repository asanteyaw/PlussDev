#pragma once

#include "distr_object_detail.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::stats::distr::fit_detail {

inline std::vector<double> data_f64(const Series& x, std::string_view op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": x must be defined");
  const auto values = x.to(ScalarType::Float64).to_vec<double>();
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

inline double fd_step(double x) {
  return std::max(1e-6, 1e-5 * std::max(1.0, std::abs(x)));
}

template <class Fn>
std::vector<double> fd_grad(const std::vector<double>& params, Fn&& fn) {
  std::vector<double> grad(params.size(), 0.0);
  std::vector<double> plus = params;
  std::vector<double> minus = params;
  for (std::size_t i = 0; i < params.size(); ++i) {
    const double h = fd_step(params[i]);
    plus[i] = params[i] + h;
    minus[i] = params[i] - h;
    const double fp = fn(plus);
    const double fm = fn(minus);
    grad[i] = (fp - fm) / (2.0 * h);
    plus[i] = params[i];
    minus[i] = params[i];
  }
  return grad;
}

template <class Fn>
std::vector<double> fd_hess_diag(const std::vector<double>& params, Fn&& fn) {
  std::vector<double> diag(params.size(), 0.0);
  std::vector<double> plus = params;
  std::vector<double> minus = params;
  const double f0 = fn(params);
  for (std::size_t i = 0; i < params.size(); ++i) {
    const double h = fd_step(params[i]);
    plus[i] = params[i] + h;
    minus[i] = params[i] - h;
    const double fp = fn(plus);
    const double fm = fn(minus);
    diag[i] = (fp - 2.0 * f0 + fm) / (h * h);
    plus[i] = params[i];
    minus[i] = params[i];
  }
  return diag;
}

inline Series vec_series(const std::vector<double>& values, std::span<const std::int64_t> shape, const Series& like) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   shape,
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline Series vec_series(const std::vector<double>& values, std::initializer_list<std::int64_t> shape, const Series& like) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   shape,
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline Series vec_series_cpu(const std::vector<double>& values, std::initializer_list<std::int64_t> shape) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   shape,
                                   Config{}.with_dtype(ScalarType::Float64));
}

inline Series scalar_series(double value, const Series& like) {
  return detail::scalar_f64(value).to(like.platform(), ScalarType::Float64);
}

inline Series diag_matrix_series(const std::vector<double>& diag, const Series& like) {
  const std::size_t n = diag.size();
  std::vector<double> data(n * n, 0.0);
  for (std::size_t i = 0; i < n; ++i) {
    data[i * n + i] = diag[i];
  }
  return pluss::codem::make_series(std::span<const double>(data.data(), data.size()),
                                   {static_cast<std::int64_t>(n), static_cast<std::int64_t>(n)},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline std::vector<double> diag_inverse(const std::vector<double>& diag, std::string_view op) {
  std::vector<double> inv(diag.size(), 0.0);
  std::transform(diag.begin(), diag.end(), inv.begin(), [op](double v) {
    PLUSS_CHECK(v > 0.0 && std::isfinite(v), std::string(op) + ": expected positive finite diagonal info");
    return 1.0 / v;
  });
  return inv;
}

} // namespace pluss::subsp::stats::distr::fit_detail
