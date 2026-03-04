#include "distr_object_detail.h"

#include <array>
#include <cstdint>
#include <span>

namespace pluss::subsp::stat::distr {
namespace {

std::array<double, 1> bernoulli_params(double p) {
  return {p};
}

std::array<double, 2> binomial_params(std::int64_t count, double p) {
  return {static_cast<double>(count), p};
}

Series bernoulli_log_mgf_kernel(const Series& t, double p) {
  return t.expm1().mul(p).log1p();
}

} // namespace

Series Bernoulli::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("bernoulli", bernoulli_params(p), sizes, cfg, seed);
}

Series Bernoulli::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Bernoulli::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("bernoulli", bernoulli_params(p), like, seed), like);
}

Series Bernoulli::pdf(const Series& x) const { return detail::pdf_with_params("bernoulli", x, bernoulli_params(p)); }

Series Bernoulli::logpdf(const Series& x) const {
  return detail::logpdf_with_params("bernoulli", x, bernoulli_params(p));
}

Series Bernoulli::logcdf(const Series& x) const {
  return detail::logcdf_with_params("bernoulli", x, bernoulli_params(p));
}

Series Bernoulli::cdf(const Series& x) const { return detail::cdf_with_params("bernoulli", x, bernoulli_params(p)); }

Series Bernoulli::cgf(const Series& t) const {
  return bernoulli_log_mgf_kernel(t, p);
}

Series Binomial::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("binomial", binomial_params(count, p), sizes, cfg, seed);
}

Series Binomial::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Binomial::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("binomial", binomial_params(count, p), like, seed), like);
}

Series Binomial::pdf(const Series& x) const { return detail::pdf_with_params("binomial", x, binomial_params(count, p)); }

Series Binomial::logpdf(const Series& x) const {
  return detail::logpdf_with_params("binomial", x, binomial_params(count, p));
}

Series Binomial::logcdf(const Series& x) const {
  return detail::logcdf_with_params("binomial", x, binomial_params(count, p));
}

Series Binomial::cdf(const Series& x) const { return detail::cdf_with_params("binomial", x, binomial_params(count, p)); }

Series Binomial::cgf(const Series& t) const {
  return bernoulli_log_mgf_kernel(t, p).mul(static_cast<double>(count));
}

} // namespace pluss::subsp::stat::distr
