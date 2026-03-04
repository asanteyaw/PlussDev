#pragma once

#include "pluss/subsp/stat/distr.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>

namespace pluss::subsp::stat::distr::detail {

template <std::size_t N>
std::span<const double> params_span(const std::array<double, N>& params) {
  return std::span<const double>(params.data(), params.size());
}

template <std::size_t N>
Series sample_with_params(std::string_view name,
                          const std::array<double, N>& params,
                          std::span<const std::int64_t> sizes,
                          Config cfg,
                          std::uint64_t seed) {
  return ::pluss::subsp::stat::distr::sample(name, params_span(params), sizes, cfg, seed);
}

template <std::size_t N>
Series sample_like_with_params(std::string_view name,
                               const std::array<double, N>& params,
                               const Series& like,
                               std::uint64_t seed) {
  return ::pluss::subsp::stat::distr::sample_like(name, params_span(params), like, seed);
}

template <std::size_t N>
Series pdf_with_params(std::string_view name, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stat::distr::pdf(name, x, params_span(params));
}

template <std::size_t N>
Series logpdf_with_params(std::string_view name, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stat::distr::logpdf(name, x, params_span(params));
}

template <std::size_t N>
Series cdf_with_params(std::string_view name, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stat::distr::cdf(name, x, params_span(params));
}

template <std::size_t N>
Series logcdf_with_params(std::string_view name, const Series& x, const std::array<double, N>& params) {
  return ::pluss::subsp::stat::distr::logcdf(name, x, params_span(params));
}

inline Series preserve_like(Series x, const Series& like) {
  return x.to(like.platform(), like.scalar_type());
}

inline Config config_like(const Series& like) {
  return Config{}
      .with_dtype(like.scalar_type())
      .with_platform(like.platform())
      .with_pinned(like.is_pinned().item<bool>());
}

inline Series zero_series_like(const Series& x) {
  return pluss::codem::make_scalar_like(x, 0.0);
}

inline void require_all_true(const Series& mask, std::string_view message) {
  if (!mask.all().item<bool>()) {
    throw std::domain_error(std::string(message));
  }
}

} // namespace pluss::subsp::stat::distr::detail
