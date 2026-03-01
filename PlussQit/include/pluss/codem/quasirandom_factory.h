#pragma once

#include "pluss/codem/series_factory.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

namespace pluss::detail::quasirandom {

struct SobolParams {
  std::uint32_t degree;
  std::uint32_t a;
  std::array<std::uint32_t, 6> m;
};

inline constexpr std::array<SobolParams, 16> kSobolParams = {{
    {0, 0, {0, 0, 0, 0, 0, 0}},             // dim 1 handled specially
    {1, 0, {1, 0, 0, 0, 0, 0}},             // dim 2
    {2, 1, {1, 3, 0, 0, 0, 0}},             // dim 3
    {3, 1, {1, 3, 1, 0, 0, 0}},             // dim 4
    {3, 2, {1, 1, 1, 0, 0, 0}},             // dim 5
    {4, 1, {1, 1, 3, 3, 0, 0}},             // dim 6
    {4, 4, {1, 3, 5, 13, 0, 0}},            // dim 7
    {5, 2, {1, 1, 5, 5, 17, 0}},            // dim 8
    {5, 4, {1, 1, 5, 5, 5, 0}},             // dim 9
    {5, 7, {1, 1, 7, 11, 19, 0}},           // dim 10
    {5, 11, {1, 1, 7, 13, 25, 0}},          // dim 11
    {5, 13, {1, 1, 5, 3, 11, 0}},           // dim 12
    {5, 14, {1, 3, 1, 15, 17, 0}},          // dim 13
    {6, 1, {1, 3, 3, 9, 7, 49}},            // dim 14
    {6, 13, {1, 1, 1, 15, 21, 21}},         // dim 15
    {6, 16, {1, 3, 5, 13, 27, 49}},         // dim 16
}};

inline std::int64_t num_points_for_sobol(std::span<const std::int64_t> sizes) {
  if (sizes.empty()) return 1;
  if (sizes.size() == 1) return sizes.front();
  return std::accumulate(
      sizes.begin(),
      sizes.end() - 1,
      std::int64_t{1},
      [](std::int64_t acc, std::int64_t s) {
        PLUSS_CHECK(s >= 0, "sobol: sizes must be >= 0");
        return acc * s;
      });
}

inline std::int64_t sobol_dimension(std::span<const std::int64_t> sizes) {
  if (sizes.empty()) return 1;
  if (sizes.size() == 1) return 1;
  return sizes.back();
}

inline std::array<std::uint64_t, 64> sobol_directions(std::int64_t dim_index) {
  std::array<std::uint64_t, 64> dirs{};
  if (dim_index == 0) {
    for (std::size_t bit = 0; bit < dirs.size(); ++bit) {
      dirs[bit] = std::uint64_t{1} << (63U - static_cast<unsigned>(bit));
    }
    return dirs;
  }

  PLUSS_CHECK(
      dim_index >= 0 && dim_index < static_cast<std::int64_t>(kSobolParams.size()),
      "sobol: supported dimensions are 1..16 in the current implementation");

  const auto& p = kSobolParams[static_cast<std::size_t>(dim_index)];
  for (std::uint32_t i = 0; i < p.degree; ++i) {
    dirs[i] = std::uint64_t{p.m[i]} << (63U - i);
  }

  for (std::size_t i = p.degree; i < dirs.size(); ++i) {
    std::uint64_t value = dirs[i - p.degree] ^ (dirs[i - p.degree] >> p.degree);
    for (std::uint32_t k = 1; k < p.degree; ++k) {
      if (((p.a >> (p.degree - 1U - k)) & 1U) != 0U) {
        value ^= dirs[i - k];
      }
    }
    dirs[i] = value;
  }

  return dirs;
}

inline double sobol_value(std::uint64_t index, const std::array<std::uint64_t, 64>& dirs) {
  const std::uint64_t gray = index ^ (index >> 1U);
  std::uint64_t x = 0;
  std::uint64_t bits = gray;
  std::size_t bit = 0;
  while (bits != 0U && bit < dirs.size()) {
    if ((bits & 1U) != 0U) {
      x ^= dirs[bit];
    }
    bits >>= 1U;
    ++bit;
  }
  return std::ldexp(static_cast<double>(x), -64);
}

inline std::vector<double> sobol_flat(std::span<const std::int64_t> sizes, std::uint64_t offset) {
  const std::int64_t points = num_points_for_sobol(sizes);
  const std::int64_t dims = sobol_dimension(sizes);
  PLUSS_CHECK(points >= 0, "sobol: number of points must be >= 0");
  PLUSS_CHECK(dims > 0, "sobol: dimension must be > 0");

  std::vector<std::array<std::uint64_t, 64>> directions;
  directions.reserve(static_cast<std::size_t>(dims));
  for (std::int64_t d = 0; d < dims; ++d) {
    directions.push_back(sobol_directions(d));
  }

  std::vector<double> data(static_cast<std::size_t>(points * dims), 0.0);
  for (std::int64_t i = 0; i < points; ++i) {
    const std::uint64_t index = offset + static_cast<std::uint64_t>(i);
    for (std::int64_t d = 0; d < dims; ++d) {
      data[static_cast<std::size_t>(i * dims + d)] =
          sobol_value(index, directions[static_cast<std::size_t>(d)]);
    }
  }
  return data;
}

} // namespace pluss::detail::quasirandom

namespace pluss {

inline Series sobol(std::span<const std::int64_t> sizes,
                    ScalarType dt = ScalarType::Float64,
                    std::uint64_t offset = 0) {
  PLUSS_CHECK(
      dt == ScalarType::Float32 || dt == ScalarType::Float64,
      "sobol: dtype must be Float32 or Float64");
  const auto data = detail::quasirandom::sobol_flat(sizes, offset);
  return pluss::make_series(data, sizes, Config{}.with_dtype(ScalarType::Float64)).to(dt);
}

inline Series sobol(std::initializer_list<std::int64_t> sizes,
                    ScalarType dt = ScalarType::Float64,
                    std::uint64_t offset = 0) {
  return sobol(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, offset);
}

inline Series sobol(std::span<const std::int64_t> sizes,
                    Config cfg,
                    std::uint64_t offset = 0) {
  const ScalarType dt = (cfg.dtype == ScalarType::Float32) ? ScalarType::Float32 : ScalarType::Float64;
  return sobol(sizes, dt, offset).to(cfg.platform, dt);
}

inline Series sobol(std::initializer_list<std::int64_t> sizes,
                    Config cfg,
                    std::uint64_t offset = 0) {
  return sobol(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline Series sobol_normal(std::span<const std::int64_t> sizes,
                           ScalarType dt = ScalarType::Float64,
                           std::uint64_t offset = 0) {
  const Series u = sobol(sizes, ScalarType::Float64, offset).clamp_min(1e-12).clamp_max(1.0 - 1e-12);
  return u.mul(2.0).sub(1.0).erfinv().mul(std::sqrt(2.0)).to(dt);
}

inline Series sobol_normal(std::initializer_list<std::int64_t> sizes,
                           ScalarType dt = ScalarType::Float64,
                           std::uint64_t offset = 0) {
  return sobol_normal(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, offset);
}

inline Series sobol_normal(std::span<const std::int64_t> sizes,
                           Config cfg,
                           std::uint64_t offset = 0) {
  const ScalarType dt = (cfg.dtype == ScalarType::Float32) ? ScalarType::Float32 : ScalarType::Float64;
  return sobol_normal(sizes, dt, offset).to(cfg.platform, dt);
}

inline Series sobol_normal(std::initializer_list<std::int64_t> sizes,
                           Config cfg,
                           std::uint64_t offset = 0) {
  return sobol_normal(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

} // namespace pluss
