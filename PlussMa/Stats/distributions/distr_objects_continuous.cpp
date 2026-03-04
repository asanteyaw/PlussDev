#include "distr_object_detail.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <span>

namespace pluss::subsp::stat::distr {
namespace {

std::array<double, 2> normal_params(double mean, double stddev) {
  return {mean, stddev};
}

std::array<double, 3> gamma_params(double shape, double scale, double location) {
  return {location, scale, shape};
}

std::array<double, 2> uniform_params(double low, double high) {
  return {low, high};
}

std::array<double, 1> exponential_params(double rate) {
  return {rate};
}

Series stable_log_expm1_over_x(const Series& x) {
  const Series small = x.abs().lt(1e-5);
  const Series general = x.expm1().div(x).log();
  const Series series = x.mul(0.5).add(x.mul(x).div(24.0));
  return general.where(small.logical_not(), series);
}

} // namespace

Series Normal::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("normal", normal_params(mean, stddev), sizes, cfg, seed);
}

Series Normal::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Normal::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("normal", normal_params(mean, stddev), like, seed), like);
}

Series Normal::pdf(const Series& x) const { return detail::pdf_with_params("normal", x, normal_params(mean, stddev)); }

Series Normal::logpdf(const Series& x) const {
  return detail::logpdf_with_params("normal", x, normal_params(mean, stddev));
}

Series Normal::logcdf(const Series& x) const {
  return detail::logcdf_with_params("normal", x, normal_params(mean, stddev));
}

Series Normal::cdf(const Series& x) const { return detail::cdf_with_params("normal", x, normal_params(mean, stddev)); }

Series Normal::cgf(const Series& t) const {
  return t.mul(mean).add(t.mul(t).mul(0.5 * stddev * stddev));
}

Series Gamma::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("gamma", gamma_params(shape, scale, location), sizes, cfg, seed);
}

Series Gamma::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Gamma::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("gamma", gamma_params(shape, scale, location), like, seed), like);
}

Series Gamma::pdf(const Series& x) const { return detail::pdf_with_params("gamma", x, gamma_params(shape, scale, location)); }

Series Gamma::logpdf(const Series& x) const {
  return detail::logpdf_with_params("gamma", x, gamma_params(shape, scale, location));
}

Series Gamma::logcdf(const Series& x) const {
  return detail::logcdf_with_params("gamma", x, gamma_params(shape, scale, location));
}

Series Gamma::cdf(const Series& x) const { return detail::cdf_with_params("gamma", x, gamma_params(shape, scale, location)); }

Series Gamma::cgf(const Series& t) const {
  detail::require_all_true(t.lt(1.0 / scale), "Gamma::cgf requires every t element to satisfy t < 1 / scale");
  return t.mul(location).sub(t.mul(scale).neg().log1p().mul(shape));
}

Series Uniform::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("uniform", uniform_params(low, high), sizes, cfg, seed);
}

Series Uniform::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Uniform::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("uniform", uniform_params(low, high), like, seed), like);
}

Series Uniform::pdf(const Series& x) const { return detail::pdf_with_params("uniform", x, uniform_params(low, high)); }

Series Uniform::logpdf(const Series& x) const {
  return detail::logpdf_with_params("uniform", x, uniform_params(low, high));
}

Series Uniform::logcdf(const Series& x) const {
  return detail::logcdf_with_params("uniform", x, uniform_params(low, high));
}

Series Uniform::cdf(const Series& x) const { return detail::cdf_with_params("uniform", x, uniform_params(low, high)); }

Series Uniform::cgf(const Series& t) const {
  const double width = high - low;
  const Series scaled = t.mul(width);
  return t.mul(low).add(stable_log_expm1_over_x(scaled));
}

Series Exponential::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("exponential", exponential_params(rate), sizes, cfg, seed);
}

Series Exponential::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Exponential::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("exponential", exponential_params(rate), like, seed), like);
}

Series Exponential::pdf(const Series& x) const {
  return detail::pdf_with_params("exponential", x, exponential_params(rate));
}

Series Exponential::logpdf(const Series& x) const {
  return detail::logpdf_with_params("exponential", x, exponential_params(rate));
}

Series Exponential::logcdf(const Series& x) const {
  return detail::logcdf_with_params("exponential", x, exponential_params(rate));
}

Series Exponential::cdf(const Series& x) const {
  return detail::cdf_with_params("exponential", x, exponential_params(rate));
}

Series Exponential::cgf(const Series& t) const {
  detail::require_all_true(t.lt(rate), "Exponential::cgf requires every t element to satisfy t < rate");
  return t.div(-rate).log1p().neg();
}

} // namespace pluss::subsp::stat::distr
