#include "distr_object_detail.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <span>

namespace pluss::subsp::stat::distr {
namespace {

std::array<double, 2> skellam_params(double lambda1, double lambda2) {
  return {lambda1, lambda2};
}

std::array<double, 4> nig_params(double alpha, double beta, double location, double scale) {
  return {alpha, beta, location, scale};
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

} // namespace pluss::subsp::stat::distr
