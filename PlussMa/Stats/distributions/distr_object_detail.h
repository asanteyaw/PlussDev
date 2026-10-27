#pragma once

#include "pluss/subsp/stats/distr.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pluss::subsp::stats::distr::detail {

using DistKind = ::pluss::subsp::stats::distr::dst::Kind;

template <std::size_t N>
std::span<const double> params_span(const std::array<double, N>& params) {
  return std::span<const double>(params.data(), params.size());
}

template <std::size_t N>
Series sample_with_params(DistKind kind,
                          const std::array<double, N>& params,
                          std::span<const std::int64_t> sizes,
                          Config cfg,
                          std::uint64_t seed) {
  return ::pluss::subsp::stats::distr::sample(kind, params_span(params), sizes, cfg, seed);
}

template <std::size_t N>
Series sample_like_with_params(DistKind kind,
                               const std::array<double, N>& params,
                               const Series& like,
                               std::uint64_t seed) {
  return ::pluss::subsp::stats::distr::sample_like(kind, params_span(params), like, seed);
}

template <std::size_t N>
Series pdf_with_params(DistKind kind, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stats::distr::pdf(kind, x, params_span(params));
}

template <std::size_t N>
Series logpdf_with_params(DistKind kind, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stats::distr::logpdf(kind, x, params_span(params));
}

template <std::size_t N>
Series cdf_with_params(DistKind kind, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stats::distr::cdf(kind, x, params_span(params));
}

template <std::size_t N>
Series logcdf_with_params(DistKind kind, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stats::distr::logcdf(kind, x, params_span(params));
}

inline Series preserve_like(Series x, const Series& like) {
  return x.to(like.platform(), like.scalar_type());
}

inline Config config_like(const Series& like) {
  return Config{}
      .with_dtype(like.scalar_type())
      .with_platform(like.platform())
      .with_pinned(like.is_pinned());
}

inline Series zero_series_like(const Series& x) {
  return pluss::codem::make_scalar_like(x, 0.0);
}

inline void require_all_true(const Series& mask, std::string_view message) {
  if (!mask.all().item<bool>()) {
    throw std::domain_error(std::string(message));
  }
}

inline void require_positive_order(std::int64_t order, std::string_view op) {
  if (order <= 0) {
    throw std::invalid_argument(std::string(op) + ": order must be >= 1");
  }
}

inline Series scalar_f64(double value) {
  return pluss::codem::value(value, ScalarType::Float64);
}

template <class Fn>
Series map_f64_like(const Series& x, Fn&& fn) {
  const Series cpu = x.to(ScalarType::Float64);
  const auto in = cpu.host<double>();
  std::vector<double> out(in.size());
  std::transform(in.begin(), in.end(), out.begin(), std::forward<Fn>(fn));
  return pluss::codem::make_series(out, x.sizes(), Config{}.with_dtype(ScalarType::Float64)).to(x.platform(), ScalarType::Float64);
}

inline double factorial(std::int64_t n) {
  if (n < 0) {
    throw std::invalid_argument("detail::factorial: n must be >= 0");
  }
  double out = 1.0;
  for (std::int64_t i = 2; i <= n; ++i) {
    out *= static_cast<double>(i);
  }
  return out;
}

inline double binomial_coeff(std::int64_t n, std::int64_t k) {
  if (k < 0 || k > n) {
    return 0.0;
  }
  k = std::min(k, n - k);
  double out = 1.0;
  for (std::int64_t i = 1; i <= k; ++i) {
    out *= static_cast<double>(n - k + i);
    out /= static_cast<double>(i);
  }
  return out;
}

inline double rising_factorial(double x, std::int64_t n) {
  double out = 1.0;
  for (std::int64_t i = 0; i < n; ++i) {
    out *= x + static_cast<double>(i);
  }
  return out;
}

inline double odd_double_factorial(std::int64_t n) {
  if (n <= 0) {
    return 1.0;
  }
  double out = 1.0;
  for (std::int64_t i = n; i >= 1; i -= 2) {
    out *= static_cast<double>(i);
  }
  return out;
}

inline double normal_raw_moment(double mean, double stddev, std::int64_t order) {
  double out = 0.0;
  for (std::int64_t j = 0; 2 * j <= order; ++j) {
    const std::int64_t even = 2 * j;
    const double coeff = binomial_coeff(order, even) * odd_double_factorial(even - 1);
    out += coeff * std::pow(stddev, static_cast<double>(even)) *
           std::pow(mean, static_cast<double>(order - even));
  }
  return out;
}

inline std::vector<double> cumulants_from_raw_moments(const std::vector<double>& raw) {
  if (raw.empty()) {
    return {};
  }
  std::vector<double> cumulants(raw.size(), 0.0);
  for (std::size_t n = 1; n < raw.size(); ++n) {
    double value = raw[n];
    for (std::size_t m = 1; m < n; ++m) {
      value -= binomial_coeff(static_cast<std::int64_t>(n - 1), static_cast<std::int64_t>(m - 1)) *
               cumulants[m] * raw[n - m];
    }
    cumulants[n] = value;
  }
  return cumulants;
}

inline std::vector<double> raw_moments_from_centered(const std::vector<double>& centered, double location) {
  if (centered.empty()) {
    return {};
  }
  std::vector<double> raw(centered.size(), 0.0);
  raw[0] = 1.0;
  for (std::size_t n = 1; n < centered.size(); ++n) {
    double value = 0.0;
    for (std::size_t k = 0; k <= n; ++k) {
      value += binomial_coeff(static_cast<std::int64_t>(n), static_cast<std::int64_t>(k)) *
               std::pow(location, static_cast<double>(n - k)) * centered[k];
    }
    raw[n] = value;
  }
  return raw;
}

inline Series zero_only_cgf(const Series& t, std::string_view name) {
  require_all_true(t.eq(0.0), std::string(name) + "::cgf is only defined at t = 0 for this family");
  return zero_series_like(t).to(t.platform(), ScalarType::Float64);
}

inline long double bernoulli_number(std::int64_t n) {
  if (n < 0) {
    throw std::invalid_argument("detail::bernoulli_number: n must be >= 0");
  }
  std::vector<long double> a(static_cast<std::size_t>(n) + 1, 0.0L);
  for (std::int64_t m = 0; m <= n; ++m) {
    a[static_cast<std::size_t>(m)] = 1.0L / static_cast<long double>(m + 1);
    for (std::int64_t j = m; j >= 1; --j) {
      a[static_cast<std::size_t>(j - 1)] =
          static_cast<long double>(j) *
          (a[static_cast<std::size_t>(j - 1)] - a[static_cast<std::size_t>(j)]);
    }
  }
  return a.front();
}

} // namespace pluss::subsp::stats::distr::detail
