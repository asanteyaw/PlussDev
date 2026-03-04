#include "distr_object_detail.h"

#include <array>
#include <cmath>
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

std::array<double, 1> poisson_params(double lambda) {
  return {lambda};
}

std::array<double, 1> geometric_params(double p) {
  return {p};
}

std::array<double, 2> negbinom_params(double r, double p) {
  return {r, p};
}

Series bernoulli_log_mgf_kernel(const Series& t, double p) {
  return t.expm1().mul(p).log1p();
}

double stirling_second_kind(std::int64_t n, std::int64_t k) {
  if (n < 0 || k < 0 || k > n) return 0.0;
  std::vector<long double> row(static_cast<std::size_t>(k) + 1, 0.0L);
  row[0] = 1.0L;
  for (std::int64_t i = 1; i <= n; ++i) {
    const std::int64_t upper = std::min(i, k);
    for (std::int64_t j = upper; j >= 1; --j) {
      row[static_cast<std::size_t>(j)] =
          static_cast<long double>(j) * row[static_cast<std::size_t>(j)] +
          row[static_cast<std::size_t>(j - 1)];
    }
    row[0] = 0.0L;
  }
  return static_cast<double>(row[static_cast<std::size_t>(k)]);
}

double factorial_int(std::int64_t n) {
  double out = 1.0;
  for (std::int64_t i = 2; i <= n; ++i) out *= static_cast<double>(i);
  return out;
}

double polylog_negative_integer_sum(double q, std::int64_t power) {
  if (power < 0) {
    throw std::invalid_argument("polylog_negative_integer_sum: power must be >= 0");
  }
  if (q == 0.0) return 0.0;
  if (power == 0) return q / (1.0 - q);
  const double one_minus_q = 1.0 - q;
  double total = 0.0;
  for (std::int64_t k = 1; k <= power; ++k) {
    total += stirling_second_kind(power, k) * factorial_int(k) *
             std::pow(q, static_cast<double>(k)) /
             std::pow(one_minus_q, static_cast<double>(k + 1));
  }
  return total;
}

double negbinom_cumulant_scalar(double r, double p, std::int64_t order) {
  if (order <= 0) {
    throw std::invalid_argument("NegativeBinomial::cumulant: order must be >= 1");
  }
  if (!(r > 0.0) || !(p > 0.0 && p < 1.0)) {
    throw std::domain_error("NegativeBinomial::cumulant requires r > 0 and 0 < p < 1");
  }
  return r * polylog_negative_integer_sum(1.0 - p, order - 1);
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

Series Bernoulli::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Bernoulli::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = p;
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
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

Series Binomial::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Binomial::cumulant");
  return Bernoulli{p}.cumulant(order).mul(static_cast<double>(count));
}

Series Poisson::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("poisson", poisson_params(lambda), sizes, cfg, seed);
}

Series Poisson::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Poisson::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("poisson", poisson_params(lambda), like, seed), like);
}

Series Poisson::pdf(const Series& x) const { return detail::pdf_with_params("poisson", x, poisson_params(lambda)); }

Series Poisson::logpdf(const Series& x) const {
  return detail::logpdf_with_params("poisson", x, poisson_params(lambda));
}

Series Poisson::logcdf(const Series& x) const {
  return detail::logcdf_with_params("poisson", x, poisson_params(lambda));
}

Series Poisson::cdf(const Series& x) const { return detail::cdf_with_params("poisson", x, poisson_params(lambda)); }

Series Poisson::cgf(const Series& t) const {
  return t.exp().sub(1.0).mul(lambda);
}

Series Poisson::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Poisson::cumulant");
  return detail::scalar_f64(lambda);
}

Series Geometric::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("geometric", geometric_params(p), sizes, cfg, seed);
}

Series Geometric::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Geometric::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("geometric", geometric_params(p), like, seed), like);
}

Series Geometric::pdf(const Series& x) const { return detail::pdf_with_params("geometric", x, geometric_params(p)); }

Series Geometric::logpdf(const Series& x) const {
  return detail::logpdf_with_params("geometric", x, geometric_params(p));
}

Series Geometric::logcdf(const Series& x) const {
  return detail::logcdf_with_params("geometric", x, geometric_params(p));
}

Series Geometric::cdf(const Series& x) const { return detail::cdf_with_params("geometric", x, geometric_params(p)); }

Series Geometric::cgf(const Series& t) const {
  detail::require_all_true(t.lt(-std::log1p(-p)), "Geometric::cgf requires every t element to satisfy t < -log(1 - p)");
  return t.add(std::log(p)).sub(t.exp().mul(1.0 - p).mul(-1.0).add(1.0).log());
}

Series Geometric::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Geometric::cumulant");
  if (!(p > 0.0 && p <= 1.0)) {
    throw std::domain_error("Geometric::cumulant requires 0 < p <= 1");
  }
  if (p == 1.0) {
    return detail::scalar_f64(order == 1 ? 1.0 : 0.0);
  }
  if (order == 1) {
    return detail::scalar_f64(1.0 + negbinom_cumulant_scalar(1.0, p, 1));
  }
  return detail::scalar_f64(negbinom_cumulant_scalar(1.0, p, order));
}

Series NegativeBinomial::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("negbinom", negbinom_params(r, p), sizes, cfg, seed);
}

Series NegativeBinomial::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series NegativeBinomial::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("negbinom", negbinom_params(r, p), like, seed), like);
}

Series NegativeBinomial::pdf(const Series& x) const { return detail::pdf_with_params("negbinom", x, negbinom_params(r, p)); }

Series NegativeBinomial::logpdf(const Series& x) const {
  return detail::logpdf_with_params("negbinom", x, negbinom_params(r, p));
}

Series NegativeBinomial::logcdf(const Series& x) const {
  return detail::logcdf_with_params("negbinom", x, negbinom_params(r, p));
}

Series NegativeBinomial::cdf(const Series& x) const { return detail::cdf_with_params("negbinom", x, negbinom_params(r, p)); }

Series NegativeBinomial::cgf(const Series& t) const {
  detail::require_all_true(t.lt(-std::log1p(-p)), "NegativeBinomial::cgf requires every t element to satisfy t < -log(1 - p)");
  return detail::scalar_f64(std::log(p)).mul(r).sub(t.exp().mul(1.0 - p).mul(-1.0).add(1.0).log().mul(r));
}

Series NegativeBinomial::cumulant(std::int64_t order) const {
  return detail::scalar_f64(negbinom_cumulant_scalar(r, p, order));
}

} // namespace pluss::subsp::stat::distr
