#include "distr_object_detail.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <span>
#include <vector>

namespace pluss::subsp::stat::distr {
namespace {

std::array<double, 2> skellam_params(double lambda1, double lambda2) {
  return {lambda1, lambda2};
}

std::array<double, 4> nig_params(double alpha, double beta, double location, double scale) {
  return {alpha, beta, location, scale};
}

std::array<double, 5> gh_params(double lambda, double alpha, double beta, double location, double scale) {
  return {lambda, alpha, beta, location, scale};
}

double bessel_i_series_scalar(double order, double x) {
  double acc = 0.0;
  const double half = x * 0.5;
  for (int m = 0; m < 48; ++m) {
    const double den = std::tgamma(m + 1.0) * std::tgamma(m + order + 1.0);
    const double term = std::pow(half, 2.0 * static_cast<double>(m) + order) / den;
    acc += term;
    if (std::abs(term) < 1e-16) {
      break;
    }
  }
  return acc;
}

double nig_cumulant_scalar(double alpha, double beta, double location, double scale, std::int64_t order) {
  if (order <= 0) {
    throw std::invalid_argument("Nig::cumulant: order must be >= 1");
  }
  if (!(alpha > 0.0) || !(scale > 0.0) || !(std::abs(beta) < alpha)) {
    throw std::domain_error("Nig::cumulant requires alpha > 0, scale > 0, and |beta| < alpha");
  }

  const long double alpha_ld = static_cast<long double>(alpha);
  const long double beta_ld = static_cast<long double>(beta);
  const long double location_ld = static_cast<long double>(location);
  const long double scale_ld = static_cast<long double>(scale);
  const long double gamma = std::sqrt(alpha_ld * alpha_ld - beta_ld * beta_ld);

  std::vector<long double> coeffs(static_cast<std::size_t>(order) + 1, 0.0L);
  coeffs[0] = gamma;
  for (std::int64_t n = 1; n <= order; ++n) {
    long double acc = 0.0L;
    for (std::int64_t k = 1; k < n; ++k) {
      acc += coeffs[static_cast<std::size_t>(k)] * coeffs[static_cast<std::size_t>(n - k)];
    }
    const long double rhs = n == 1 ? -2.0L * beta_ld : (n == 2 ? -1.0L : 0.0L);
    coeffs[static_cast<std::size_t>(n)] = (rhs - acc) / (2.0L * gamma);
  }

  long double factorial = 1.0L;
  for (std::int64_t i = 2; i <= order; ++i) {
    factorial *= static_cast<long double>(i);
  }

  long double cumulant = -scale_ld * factorial * coeffs[static_cast<std::size_t>(order)];
  if (order == 1) {
    cumulant += location_ld;
  }
  return static_cast<double>(cumulant);
}

double bessel_k_value(double order, double x) {
  if (!(x > 0.0)) {
    return std::numeric_limits<double>::infinity();
  }
  if (x > 32.0) {
    const double order2 = order * order;
    const double corr = 1.0 + (4.0 * order2 - 1.0) / (8.0 * x);
    return std::max(std::sqrt(std::numbers::pi_v<double> / (2.0 * x)) * std::exp(-x) * std::max(corr, 1e-15),
                    std::numeric_limits<double>::min());
  }
  const double adjusted = (std::abs(std::sin(std::numbers::pi_v<double> * order)) < 1e-8) ? (order + 1e-6) : order;
  const double s = std::sin(std::numbers::pi_v<double> * adjusted);
  const double raw = (std::numbers::pi_v<double> * 0.5) *
                     (bessel_i_series_scalar(-adjusted, x) - bessel_i_series_scalar(adjusted, x)) / s;
  if (std::isfinite(raw) && raw > 0.0) {
    return raw;
  }
  const double order2 = order * order;
  const double corr = 1.0 + (4.0 * order2 - 1.0) / (8.0 * x);
  return std::max(std::sqrt(std::numbers::pi_v<double> / (2.0 * x)) * std::exp(-x) * std::max(corr, 1e-15),
                  std::numeric_limits<double>::min());
}

double gh_gig_moment(double lambda, double gamma, double scale, std::int64_t power) {
  const double z = scale * gamma;
  return std::pow(scale / gamma, static_cast<double>(power)) *
         bessel_k_value(lambda + static_cast<double>(power), z) /
         bessel_k_value(lambda, z);
}

double gh_raw_moment(double lambda,
                     double alpha,
                     double beta,
                     double location,
                     double scale,
                     std::int64_t order) {
  const double gamma = std::sqrt(alpha * alpha - beta * beta);
  double moment = 0.0;
  for (std::int64_t j = 0; 2 * j <= order; ++j) {
    const std::int64_t even = 2 * j;
    const double z_moment = detail::odd_double_factorial(even - 1);
    const double outer = detail::binomial_coeff(order, even) * z_moment;
    for (std::int64_t k = 0; k <= order - even; ++k) {
      moment += outer *
                detail::binomial_coeff(order - even, k) *
                std::pow(location, static_cast<double>(order - even - k)) *
                std::pow(beta, static_cast<double>(k)) *
                gh_gig_moment(lambda, gamma, scale, j + k);
    }
  }
  return moment;
}

Series normalized_mixture_weights(const Series& weights) {
  PLUSS_CHECK(weights.defined(), "GaussianMixture: weights must be defined");
  const Series w = weights.to(pluss::pCPU, ScalarType::Float64).reshape({weights.numel()});
  PLUSS_CHECK(w.numel() > 0, "GaussianMixture: expected at least one component");
  const double total = w.sum().item<double>();
  PLUSS_CHECK(total > 0.0, "GaussianMixture: weights must sum to > 0");
  return w.div(total);
}

Config mixture_cfg_like(const Series& like) {
  return Config{}
      .with_dtype(like.scalar_type())
      .with_platform(like.platform())
      .with_pinned(like.is_pinned().item<bool>());
}

void require_mixture_shape_compatibility(const Series& weights, const Series& means, const Series& stddevs) {
  PLUSS_CHECK(weights.defined() && means.defined() && stddevs.defined(),
              "GaussianMixture: weights, means, and stddevs must be defined");
  PLUSS_CHECK(weights.numel() == means.numel() && weights.numel() == stddevs.numel(),
              "GaussianMixture: weights, means, and stddevs must have the same numel");
}

} // namespace

Series Skellam::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("skellam", skellam_params(lambda1, lambda2), sizes, cfg, seed);
}

Series Skellam::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Skellam::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("skellam", skellam_params(lambda1, lambda2), like, seed), like);
}

Series Skellam::pdf(const Series& x) const { return detail::pdf_with_params("skellam", x, skellam_params(lambda1, lambda2)); }

Series Skellam::logpdf(const Series& x) const {
  return detail::logpdf_with_params("skellam", x, skellam_params(lambda1, lambda2));
}

Series Skellam::logcdf(const Series& x) const {
  return detail::logcdf_with_params("skellam", x, skellam_params(lambda1, lambda2));
}

Series Skellam::cdf(const Series& x) const { return detail::cdf_with_params("skellam", x, skellam_params(lambda1, lambda2)); }

Series Skellam::cgf(const Series& t) const {
  return t.exp().sub(1.0).mul(lambda1).add(t.neg().exp().sub(1.0).mul(lambda2));
}

Series Skellam::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Skellam::cumulant");
  return detail::scalar_f64(lambda1 + (order % 2 == 0 ? lambda2 : -lambda2));
}

Series Nig::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("nig", nig_params(alpha, beta, location, scale), sizes, cfg, seed);
}

Series Nig::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Nig::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("nig", nig_params(alpha, beta, location, scale), like, seed), like);
}

Series Nig::pdf(const Series& x) const { return detail::pdf_with_params("nig", x, nig_params(alpha, beta, location, scale)); }

Series Nig::logpdf(const Series& x) const {
  return detail::logpdf_with_params("nig", x, nig_params(alpha, beta, location, scale));
}

Series Nig::logcdf(const Series& x) const {
  return detail::logcdf_with_params("nig", x, nig_params(alpha, beta, location, scale));
}

Series Nig::cdf(const Series& x) const { return detail::cdf_with_params("nig", x, nig_params(alpha, beta, location, scale)); }

Series Nig::cgf(const Series& t) const {
  detail::require_all_true(t.add(beta).abs().lt(alpha), "Nig::cgf requires every t element to satisfy |beta + t| < alpha");
  const double gamma = std::sqrt(alpha * alpha - beta * beta);
  return t.mul(location).add((t.add(beta).square().mul(-1.0).add(alpha * alpha).sqrt().mul(-scale)).add(scale * gamma));
}

Series Nig::cumulant(std::int64_t order) const {
  return detail::scalar_f64(nig_cumulant_scalar(alpha, beta, location, scale, order));
}

Series Gh::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("gh", gh_params(lambda, alpha, beta, location, scale), sizes, cfg, seed);
}

Series Gh::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Gh::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("gh", gh_params(lambda, alpha, beta, location, scale), like, seed), like);
}

Series Gh::pdf(const Series& x) const { return detail::pdf_with_params("gh", x, gh_params(lambda, alpha, beta, location, scale)); }

Series Gh::logpdf(const Series& x) const {
  return detail::logpdf_with_params("gh", x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::logcdf(const Series& x) const {
  return detail::logcdf_with_params("gh", x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::cdf(const Series& x) const { return detail::cdf_with_params("gh", x, gh_params(lambda, alpha, beta, location, scale)); }

Series Gh::cgf(const Series& t) const {
  if (!(alpha > 0.0) || !(scale > 0.0) || !(std::abs(beta) < alpha)) {
    throw std::domain_error("Gh::cgf requires alpha > 0, scale > 0, and |beta| < alpha");
  }
  const double alpha_value = alpha;
  const double beta_value = beta;
  const double lambda_value = lambda;
  const double location_value = location;
  const double scale_value = scale;
  const double gamma = std::sqrt(alpha_value * alpha_value - beta_value * beta_value);
  const double log_norm = std::log(bessel_k_value(lambda_value, scale_value * gamma));
  return detail::map_f64_like(t, [alpha_value, beta_value, gamma, lambda_value, location_value, scale_value, log_norm](double tv) {
    if (!(std::abs(beta_value + tv) < alpha_value)) {
      throw std::domain_error("Gh::cgf requires |beta + t| < alpha");
    }
    const double new_gamma = std::sqrt(alpha_value * alpha_value - (beta_value + tv) * (beta_value + tv));
    return location_value * tv +
           lambda_value * (std::log(gamma) - std::log(new_gamma)) +
           std::log(bessel_k_value(lambda_value, scale_value * new_gamma)) - log_norm;
  });
}

Series Gh::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Gh::cumulant");
  if (!(alpha > 0.0) || !(scale > 0.0) || !(std::abs(beta) < alpha)) {
    throw std::domain_error("Gh::cumulant requires alpha > 0, scale > 0, and |beta| < alpha");
  }
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = gh_raw_moment(lambda, alpha, beta, location, scale, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series GaussianMixture::sample(std::span<const std::int64_t> sizes,
                               Config cfg,
                               std::uint64_t seed,
                               bool use_sobol) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stat::distr::mixture_normal(weights, means, stddevs, sizes, cfg, seed, use_sobol);
}

Series GaussianMixture::sample(std::initializer_list<std::int64_t> sizes,
                               Config cfg,
                               std::uint64_t seed,
                               bool use_sobol) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed, use_sobol);
}

Series GaussianMixture::sample_like(const Series& like, std::uint64_t seed, bool use_sobol) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stat::distr::mixture_normal(weights, means, stddevs, like.sizes(), mixture_cfg_like(like), seed, use_sobol);
}

Series GaussianMixture::pdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stat::distr::mixture_normal_pdf(x, weights, means, stddevs);
}

Series GaussianMixture::logpdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stat::distr::mixture_normal_logpdf(x, weights, means, stddevs);
}

Series GaussianMixture::logcdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stat::distr::mixture_normal_logcdf(x, weights, means, stddevs);
}

Series GaussianMixture::cdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stat::distr::mixture_normal_cdf(x, weights, means, stddevs);
}

Series GaussianMixture::cgf(const Series& t) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  const Series probs = normalized_mixture_weights(weights).to(t.platform(), ScalarType::Float64);
  const Series means_cpu = means.to(t.platform(), ScalarType::Float64).reshape({1, means.numel()});
  const Series stddevs_cpu = stddevs.to(t.platform(), ScalarType::Float64).reshape({1, stddevs.numel()});
  const Series probs_log = probs.log().reshape({1, probs.numel()});
  const Series flat_t = t.to(t.platform(), ScalarType::Float64).reshape({t.numel(), 1});
  return flat_t.mul(means_cpu)
      .add(flat_t.square().mul(0.5).mul(stddevs_cpu.square()))
      .add(probs_log)
      .logsumexp(1, false)
      .reshape(t.sizes());
}

Series GaussianMixture::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "GaussianMixture::cumulant");
  require_mixture_shape_compatibility(weights, means, stddevs);
  const auto probs = normalized_mixture_weights(weights).to_vec<double>();
  const auto mean_vals = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()}).to_vec<double>();
  const auto stddev_vals = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()}).to_vec<double>();
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    double moment = 0.0;
    for (std::size_t i = 0; i < probs.size(); ++i) {
      moment += probs[i] * detail::normal_raw_moment(mean_vals[i], stddev_vals[i], n);
    }
    raw[static_cast<std::size_t>(n)] = moment;
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

} // namespace pluss::subsp::stat::distr
