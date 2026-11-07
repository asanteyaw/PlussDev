#include "bessel_k.h"
#include "pluss/tils/kit/scalar_cast.h"
#include "distr_fit_common.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numbers>
#include <numeric>
#include <span>
#include <string>
#include <vector>

namespace pluss::subsp::stats::distr {
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

double series_scalar_param_(const Series& param, const char* op, const char* name) {
  PLUSS_CHECK(param.defined(), std::string(op) + ": missing " + name + " Series parameter");
  PLUSS_CHECK(param.numel() == 1, std::string(op) + ": " + name + " Series parameter must be scalar for sampling");
  return pluss::tils::to_scalar<double>(param.to(ScalarType::Float64));
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

double bessel_k_value(double order, double x) { return besselk::k(order, x); }

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
  const Series w = weights.to(ScalarType::Float64).reshape({weights.numel()});
  PLUSS_CHECK(w.numel() > 0, "GaussianMixture: expected at least one component");
  const double total = w.sum().item<double>();
  PLUSS_CHECK(total > 0.0, "GaussianMixture: weights must sum to > 0");
  return w.div(total);
}

Config mixture_cfg_like(const Series& like) {
  return Config{}
      .with_dtype(like.scalar_type())
      .with_platform(like.platform())
      .with_pinned(like.is_pinned());
}

void require_mixture_shape_compatibility(const Series& weights, const Series& means, const Series& stddevs) {
  PLUSS_CHECK(weights.defined() && means.defined() && stddevs.defined(),
              "GaussianMixture: weights, means, and stddevs must be defined");
  PLUSS_CHECK(weights.numel() == means.numel() && weights.numel() == stddevs.numel(),
              "GaussianMixture: weights, means, and stddevs must have the same numel");
}

constexpr double kFitPenalty = -1e300;
constexpr double kMinInfo = 1e-12;

std::vector<double> fit_data_f64_(const Series& x, const char* op) {
  return fit_detail::data_f64(x, op);
}

double wald_z_value_(double level, const char* op) {
  return fit_detail::wald_z(level, op);
}

std::vector<double> info_diag_from_loglik_(const std::vector<double>& params, const std::function<double(const std::vector<double>&)>& loglik_fn) {
  const auto hdiag = fit_detail::fd_hess_diag(params, loglik_fn);
  std::vector<double> info(hdiag.size(), kMinInfo);
  std::transform(hdiag.begin(), hdiag.end(), info.begin(), [](double h) {
    const double v = -h;
    return (std::isfinite(v) && v > kMinInfo) ? v : kMinInfo;
  });
  return info;
}

Series info_diag_matrix_series_(const std::vector<double>& info, const Series& like) {
  return fit_detail::diag_matrix_series(info, like);
}

Series vcov_diag_matrix_series_(const std::vector<double>& info, const Series& like, const char* op) {
  const auto inv = fit_detail::diag_inverse(info, op);
  return fit_detail::diag_matrix_series(inv, like);
}

std::vector<double> stderr_from_info_diag_(const std::vector<double>& info) {
  std::vector<double> se(info.size(), 0.0);
  std::transform(info.begin(), info.end(), se.begin(), [](double v) { return std::sqrt(1.0 / v); });
  return se;
}

Series confint_matrix_series_(const std::vector<double>& params,
                              const std::vector<double>& se,
                              const Series& like,
                              double level,
                              const char* op) {
  const double z = wald_z_value_(level, op);
  PLUSS_CHECK(params.size() == se.size(), std::string(op) + ": params/se size mismatch");
  std::vector<double> out(params.size() * 2, 0.0);
  for (std::size_t i = 0; i < params.size(); ++i) {
    out[i * 2 + 0] = params[i] - z * se[i];
    out[i * 2 + 1] = params[i] + z * se[i];
  }
  return pluss::codem::make_series(std::span<const double>(out.data(), out.size()),
                                   {static_cast<std::int64_t>(params.size()), 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

double normal_pdf_scalar_(double x, double mean, double stddev) {
  const double z = (x - mean) / stddev;
  return std::exp(-0.5 * z * z) / (stddev * std::sqrt(2.0 * std::numbers::pi_v<double>));
}

std::vector<double> softmax_logits_(const std::vector<double>& logits) {
  const double m = *std::max_element(logits.begin(), logits.end());
  std::vector<double> expv(logits.size(), 0.0);
  std::transform(logits.begin(), logits.end(), expv.begin(), [m](double v) { return std::exp(v - m); });
  const double s = std::accumulate(expv.begin(), expv.end(), 0.0);
  std::transform(expv.begin(), expv.end(), expv.begin(), [s](double v) { return v / s; });
  return expv;
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>
em_gaussian_mixture_1d_(const std::vector<double>& x,
                        const std::vector<double>& init_weights,
                        const std::vector<double>& init_means,
                        const std::vector<double>& init_stddevs,
                        int iters = 20000) {
  const std::size_t n = x.size();
  const std::size_t k = init_weights.size();
  std::vector<double> w = init_weights;
  std::vector<double> m = init_means;
  std::vector<double> s = init_stddevs;
  std::vector<double> resp(n * k, 0.0);

  // Iterate until the log-likelihood stops improving (relative change below 1e-13), not for a fixed count: EM converges linearly and slowly
  // when the components overlap.
  double previous = -std::numeric_limits<double>::infinity();
  for (int iter = 0; iter < iters; ++iter) {
    double loglik = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      double denom = 0.0;
      for (std::size_t j = 0; j < k; ++j) {
        const double r = w[j] * normal_pdf_scalar_(x[i], m[j], std::max(1e-6, s[j]));
        resp[i * k + j] = r;
        denom += r;
      }
      loglik += std::log(std::max(denom, 1e-300));
      const double inv = 1.0 / std::max(denom, 1e-300);
      for (std::size_t j = 0; j < k; ++j) {
        resp[i * k + j] *= inv;
      }
    }

    for (std::size_t j = 0; j < k; ++j) {
      double nk = 0.0;
      double num_mean = 0.0;
      for (std::size_t i = 0; i < n; ++i) {
        const double r = resp[i * k + j];
        nk += r;
        num_mean += r * x[i];
      }
      nk = std::max(nk, 1e-8);
      const double mean_j = num_mean / nk;
      double num_var = 0.0;
      for (std::size_t i = 0; i < n; ++i) {
        const double d = x[i] - mean_j;
        num_var += resp[i * k + j] * d * d;
      }
      w[j] = nk / static_cast<double>(n);
      m[j] = mean_j;
      s[j] = std::sqrt(std::max(num_var / nk, 1e-8));
    }
    const double ws = std::accumulate(w.begin(), w.end(), 0.0);
    std::transform(w.begin(), w.end(), w.begin(), [ws](double wi) { return wi / ws; });
    if (std::abs(loglik - previous) < 1e-13 * (1.0 + std::abs(loglik))) break;
    previous = loglik;
  }
  return {w, m, s};
}

} // namespace

Series Skellam::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::Skellam, skellam_params(lambda1, lambda2), sizes, cfg, seed);
}

Series Skellam::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Skellam::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Skellam, skellam_params(lambda1, lambda2), like, seed), like);
}

Series Skellam::pdf(const Series& x) const { return detail::pdf_with_params(dst::Skellam, x, skellam_params(lambda1, lambda2)); }

Series Skellam::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Skellam, x, skellam_params(lambda1, lambda2));
}

Series Skellam::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Skellam, x, skellam_params(lambda1, lambda2));
}

Series Skellam::cdf(const Series& x) const { return detail::cdf_with_params(dst::Skellam, x, skellam_params(lambda1, lambda2)); }

Series Skellam::cgf(const Series& t) const {
  return t.exp().sub(1.0).mul(lambda1).add(t.neg().exp().sub(1.0).mul(lambda2));
}

Series Skellam::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Skellam::cumulant");
  return detail::scalar_f64(lambda1 + (order % 2 == 0 ? lambda2 : -lambda2));
}

Series Nig::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    const double alpha_value = series_scalar_param_(alpha_series, "Nig::sample", "alpha_series");
    const double beta_value = series_scalar_param_(beta_series, "Nig::sample", "beta_series");
    const double location_value = series_scalar_param_(location_series, "Nig::sample", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Nig::sample", "scale_series");
    return detail::sample_with_params(dst::Nig, nig_params(alpha_value, beta_value, location_value, scale_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Nig, nig_params(alpha, beta, location, scale), sizes, cfg, seed);
}

Series Nig::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Nig::sample_like(const Series& like, std::uint64_t seed) const {
  if (alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    const double alpha_value = series_scalar_param_(alpha_series, "Nig::sample_like", "alpha_series");
    const double beta_value = series_scalar_param_(beta_series, "Nig::sample_like", "beta_series");
    const double location_value = series_scalar_param_(location_series, "Nig::sample_like", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Nig::sample_like", "scale_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Nig, nig_params(alpha_value, beta_value, location_value, scale_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Nig, nig_params(alpha, beta, location, scale), like, seed), like);
}

Series Nig::pdf(const Series& x) const {
  if (alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(
        dst::Nig, x, pluss::sign_list(alpha_series, beta_series, location_series, scale_series));
  }
  return detail::pdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale));
}

Series Nig::logpdf(const Series& x) const {
  if (alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(
        dst::Nig, x, pluss::sign_list(alpha_series, beta_series, location_series, scale_series));
  }
  return detail::logpdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale));
}

Series Nig::logcdf(const Series& x) const {
  if (alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(
        dst::Nig, x, pluss::sign_list(alpha_series, beta_series, location_series, scale_series));
  }
  return detail::logcdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale));
}

Series Nig::cdf(const Series& x) const {
  if (alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(
        dst::Nig, x, pluss::sign_list(alpha_series, beta_series, location_series, scale_series));
  }
  return detail::cdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale));
}

Series Nig::cgf(const Series& t) const {
  PLUSS_CHECK(!(alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()),
              "Nig::cgf: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_all_true(t.add(beta).abs().lt(alpha), "Nig::cgf requires every t element to satisfy |beta + t| < alpha");
  const double gamma = std::sqrt(alpha * alpha - beta * beta);
  return t.mul(location).add((t.add(beta).square().mul(-1.0).add(alpha * alpha).sqrt().mul(-scale)).add(scale * gamma));
}

Series Nig::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()),
              "Nig::cumulant: Series parameters are unsupported here; use scalar-object parameters");
  return detail::scalar_f64(nig_cumulant_scalar(alpha, beta, location, scale, order));
}

Series Gh::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    const double lambda_value = series_scalar_param_(lambda_series, "Gh::sample", "lambda_series");
    const double alpha_value = series_scalar_param_(alpha_series, "Gh::sample", "alpha_series");
    const double beta_value = series_scalar_param_(beta_series, "Gh::sample", "beta_series");
    const double location_value = series_scalar_param_(location_series, "Gh::sample", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Gh::sample", "scale_series");
    return detail::sample_with_params(
        dst::Gh, gh_params(lambda_value, alpha_value, beta_value, location_value, scale_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Gh, gh_params(lambda, alpha, beta, location, scale), sizes, cfg, seed);
}

Series Gh::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Gh::sample_like(const Series& like, std::uint64_t seed) const {
  if (lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    const double lambda_value = series_scalar_param_(lambda_series, "Gh::sample_like", "lambda_series");
    const double alpha_value = series_scalar_param_(alpha_series, "Gh::sample_like", "alpha_series");
    const double beta_value = series_scalar_param_(beta_series, "Gh::sample_like", "beta_series");
    const double location_value = series_scalar_param_(location_series, "Gh::sample_like", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Gh::sample_like", "scale_series");
    return detail::preserve_like(
        detail::sample_like_with_params(
            dst::Gh,
            gh_params(lambda_value, alpha_value, beta_value, location_value, scale_value),
            like,
            seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Gh, gh_params(lambda, alpha, beta, location, scale), like, seed), like);
}

Series Gh::pdf(const Series& x) const {
  if (lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(
        dst::Gh, x, pluss::sign_list(lambda_series, alpha_series, beta_series, location_series, scale_series));
  }
  return detail::pdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::logpdf(const Series& x) const {
  if (lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(
        dst::Gh, x, pluss::sign_list(lambda_series, alpha_series, beta_series, location_series, scale_series));
  }
  return detail::logpdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::logcdf(const Series& x) const {
  if (lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(
        dst::Gh, x, pluss::sign_list(lambda_series, alpha_series, beta_series, location_series, scale_series));
  }
  return detail::logcdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::cdf(const Series& x) const {
  if (lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(
        dst::Gh, x, pluss::sign_list(lambda_series, alpha_series, beta_series, location_series, scale_series));
  }
  return detail::cdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::cgf(const Series& t) const {
  PLUSS_CHECK(!(lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()),
              "Gh::cgf: Series parameters are unsupported here; use scalar-object parameters");
  if (!(alpha > 0.0) || !(scale > 0.0) || !(std::abs(beta) < alpha)) {
    throw std::domain_error("Gh::cgf requires alpha > 0, scale > 0, and |beta| < alpha");
  }
  const double alpha_value = alpha;
  const double beta_value = beta;
  const double lambda_value = lambda;
  const double location_value = location;
  const double scale_value = scale;
  const double gamma = std::sqrt(alpha_value * alpha_value - beta_value * beta_value);
  const double log_norm = besselk::log_k(lambda_value, scale_value * gamma);
  return detail::map_f64_like(t, [alpha_value, beta_value, gamma, lambda_value, location_value, scale_value, log_norm](double tv) {
    if (!(std::abs(beta_value + tv) < alpha_value)) {
      throw std::domain_error("Gh::cgf requires |beta + t| < alpha");
    }
    const double new_gamma = std::sqrt(alpha_value * alpha_value - (beta_value + tv) * (beta_value + tv));
    return location_value * tv +
           lambda_value * (std::log(gamma) - std::log(new_gamma)) +
           besselk::log_k(lambda_value, scale_value * new_gamma) - log_norm;
  });
}

Series Gh::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(lambda_series.defined() || alpha_series.defined() || beta_series.defined() || location_series.defined() || scale_series.defined()),
              "Gh::cumulant: Series parameters are unsupported here; use scalar-object parameters");
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
  return ::pluss::subsp::stats::distr::mixture_normal(weights, means, stddevs, sizes, cfg, seed, use_sobol);
}

Series GaussianMixture::sample(std::initializer_list<std::int64_t> sizes,
                               Config cfg,
                               std::uint64_t seed,
                               bool use_sobol) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed, use_sobol);
}

Series GaussianMixture::sample_like(const Series& like, std::uint64_t seed, bool use_sobol) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stats::distr::mixture_normal(weights, means, stddevs, like.sizes(), mixture_cfg_like(like), seed, use_sobol);
}

Series GaussianMixture::pdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stats::distr::mixture_normal_pdf(x, weights, means, stddevs);
}

Series GaussianMixture::logpdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stats::distr::mixture_normal_logpdf(x, weights, means, stddevs);
}

Series GaussianMixture::logcdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stats::distr::mixture_normal_logcdf(x, weights, means, stddevs);
}

Series GaussianMixture::cdf(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  return ::pluss::subsp::stats::distr::mixture_normal_cdf(x, weights, means, stddevs);
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
  const auto probs = normalized_mixture_weights(weights).host<double>();
  const auto mean_vals = means.to(ScalarType::Float64).reshape({means.numel()}).host<double>();
  const auto stddev_vals = stddevs.to(ScalarType::Float64).reshape({stddevs.numel()}).host<double>();
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

Nig NigFit::model() const {
  return Nig{alpha, beta, location, scale};
}

Series NigFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series NigFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series NigFit::nll(const Series& x) const {
  return loglik(x).neg();
}

PLUSS_DISTR_FIT_STATS(NigFit,
                      (std::vector<double>{alpha, beta, location, scale}),
                      (std::vector<bool>{true, true, true, true}),
                      fit_detail::nig_loglik)


NigFit Nig::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Nig::MLE");
  const double n = static_cast<double>(values.size());
  const double mean = std::accumulate(values.begin(), values.end(), 0.0) / n;
  double var = 0.0;
  for (double v : values) var += (v - mean) * (v - mean);
  const double sd = std::max(std::sqrt(var / n), 1e-8);
  const auto ll = fit_detail::nig_loglik(values);
  // Unconstrained coordinates: (log alpha, atanh(beta / alpha), location, log scale); several starting shapes, best result kept.
  const auto decode = [](const std::vector<double>& q) { const double a = std::exp(q[0]); return std::vector<double>{a, a * std::tanh(q[1]), q[2], std::exp(q[3])}; };
  const auto objective = [&](const std::vector<double>& q) {
    if (!std::isfinite(q[0]) || !std::isfinite(q[1]) || std::abs(q[1]) > 20.0) return fit_detail::kPenalty;
    const double v = ll(decode(q));
    return std::isfinite(v) ? v : fit_detail::kPenalty;
  };
  std::vector<double> best;
  double best_val = fit_detail::kPenalty;
  for (double shape : {0.5, 1.0, 3.0}) {
    const auto q = fit_detail::maximize_simplex(objective, {std::log(shape / sd), 0.0, mean, std::log(sd * shape)}, 0.1, 2000, 3);
    const double v = objective(q);
    if (v > best_val) {
      best_val = v;
      best = q;
    }
  }
  const auto p = decode(best);
  return NigFit{p[0], p[1], p[2], p[3]};
}

Gh GhFit::model() const {
  return Gh{lambda, alpha, beta, location, scale};
}

Series GhFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series GhFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series GhFit::nll(const Series& x) const {
  return loglik(x).neg();
}

PLUSS_DISTR_FIT_STATS(GhFit,
                      (std::vector<double>{lambda, alpha, beta, location, scale}),
                      (std::vector<bool>{true, true, true, true, true}),
                      fit_detail::gh_loglik)


GhFit Gh::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Gh::MLE");
  const double n = static_cast<double>(values.size());
  const double mean = std::accumulate(values.begin(), values.end(), 0.0) / n;
  double var = 0.0;
  for (double v : values) var += (v - mean) * (v - mean);
  const double sd = std::max(std::sqrt(var / n), 1e-8);
  const auto ll = fit_detail::gh_loglik(values);
  // Coordinates: (lambda, log alpha, atanh(beta / alpha), location, log scale).
  const auto decode = [](const std::vector<double>& q) { const double a = std::exp(q[1]); return std::vector<double>{q[0], a, a * std::tanh(q[2]), q[3], std::exp(q[4])}; };
  const auto objective = [&](const std::vector<double>& q) {
    // the scale is kept away from zero: as it shrinks the family approaches a limit (variance-gamma) that is not a member
    if (!std::isfinite(q[0]) || std::abs(q[0]) > 30.0 || std::abs(q[2]) > 20.0 || q[4] < std::log(1e-3 * sd)) return fit_detail::kPenalty;
    const double v = ll(decode(q));
    return std::isfinite(v) ? v : fit_detail::kPenalty;
  };
  std::vector<double> best;
  double best_val = fit_detail::kPenalty;
  for (double lam : {lambda, -0.5, 1.0}) {
    for (double shape : {0.7, 2.0}) {
      const auto q = fit_detail::maximize_simplex(objective, {lam, std::log(shape / sd), 0.0, mean, std::log(sd * shape)}, 0.15, 2000, 3);
      const double v = objective(q);
      if (v > best_val) {
        best_val = v;
        best = q;
      }
    }
  }
  const auto p = decode(best);
  return GhFit{p[0], p[1], p[2], p[3], p[4]};
}

GaussianMixture GaussianMixtureFit::model() const {
  return GaussianMixture{weights, means, stddevs};
}

Series GaussianMixtureFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series GaussianMixtureFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series GaussianMixtureFit::nll(const Series& x) const {
  return loglik(x).neg();
}

namespace {

// Unconstrained coordinates of a k-component mixture: (logit of w_1 ... w_{k-1} relative to w_k, means, log standard deviations).
std::vector<double> gm_theta_(const GaussianMixtureFit& fit) {
  require_mixture_shape_compatibility(fit.weights, fit.means, fit.stddevs);
  const std::size_t k = static_cast<std::size_t>(fit.weights.numel());
  const auto w = normalized_mixture_weights(fit.weights).host<double>();
  const auto m = fit.means.to(ScalarType::Float64).reshape({fit.means.numel()}).host<double>();
  const auto s = fit.stddevs.to(ScalarType::Float64).reshape({fit.stddevs.numel()}).host<double>();
  std::vector<double> theta;
  theta.reserve(3 * k - 1);
  for (std::size_t i = 0; i + 1 < k; ++i) theta.push_back(std::log(std::max(w[i], 1e-12) / std::max(w[k - 1], 1e-12)));
  theta.insert(theta.end(), m.begin(), m.end());
  for (double v : s) theta.push_back(std::log(std::max(v, 1e-6)));
  return theta;
}

fit_detail::LogLikFn gm_loglik_(const std::vector<double>& x, std::size_t k) {
  return [&x, k](const std::vector<double>& t) -> double {
    if (t.size() != 3 * k - 1) return fit_detail::kPenalty;
    std::vector<double> logits(k, 0.0);
    std::copy(t.begin(), t.begin() + static_cast<std::int64_t>(k - 1), logits.begin());
    const auto w = softmax_logits_(logits);
    double acc = 0.0;
    for (double xi : x) {
      double dens = 0.0;
      for (std::size_t j = 0; j < k; ++j) dens += w[j] * normal_pdf_scalar_(xi, t[k - 1 + j], std::exp(t[2 * k - 1 + j]));
      acc += std::log(std::max(dens, 1e-300));
    }
    return acc;
  };
}

// Covariance of the natural parameters (w_1..w_k, means, standard deviations) by the delta method from the covariance of the coordinates.
struct GmStats {
  std::vector<double> natural;  // 3k
  std::vector<double> vcov;     // 3k x 3k
  std::vector<double> se;       // 3k
};

GmStats gm_stats_(const GaussianMixtureFit& fit, const std::vector<double>& x) {
  const auto theta = gm_theta_(fit);
  const std::size_t k = static_cast<std::size_t>(fit.weights.numel());
  const std::size_t p = 3 * k - 1, q = 3 * k;
  const auto st = fit_detail::fit_stats(theta, std::vector<bool>(p, true), gm_loglik_(x, k));
  std::vector<double> logits(k, 0.0);
  std::copy(theta.begin(), theta.begin() + static_cast<std::int64_t>(k - 1), logits.begin());
  const auto w = softmax_logits_(logits);
  GmStats out;
  out.natural.assign(q, 0.0);
  for (std::size_t j = 0; j < k; ++j) {
    out.natural[j] = w[j];
    out.natural[k + j] = theta[k - 1 + j];
    out.natural[2 * k + j] = std::exp(theta[2 * k - 1 + j]);
  }
  // Jacobian J (q x p): weights from logits, means and standard deviations elementwise.
  std::vector<double> J(q * p, 0.0);
  for (std::size_t i = 0; i < k; ++i) {
    for (std::size_t j = 0; j + 1 < k; ++j) J[i * p + j] = w[i] * ((i == j ? 1.0 : 0.0) - w[j]);
    J[(k + i) * p + (k - 1 + i)] = 1.0;
    J[(2 * k + i) * p + (2 * k - 1 + i)] = out.natural[2 * k + i];
  }
  out.vcov.assign(q * q, 0.0);
  for (std::size_t a = 0; a < q; ++a) {
    for (std::size_t b = 0; b < q; ++b) {
      double acc = 0.0;
      for (std::size_t i = 0; i < p; ++i) {
        for (std::size_t j = 0; j < p; ++j) acc += J[a * p + i] * st.vcov[i * p + j] * J[b * p + j];
      }
      out.vcov[a * q + b] = acc;
    }
  }
  out.se.resize(q);
  for (std::size_t a = 0; a < q; ++a) out.se[a] = std::sqrt(std::max(out.vcov[a * q + a], 0.0));
  return out;
}

}  // namespace

// score and observed information are in the unconstrained coordinates of gm_theta_ (3k - 1 of them); vcov, stderr and confint are for the
// natural parameters (w_1..w_k, means, standard deviations), 3k of them, by the delta method.
Series GaussianMixtureFit::score(const Series& x) const {
  const auto xvals = fit_data_f64_(x, "GaussianMixtureFit::score");
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  const auto theta = gm_theta_(*this);
  return fit_detail::vec_series(fit_detail::fit_score(theta, std::vector<bool>(theta.size(), true), gm_loglik_(xvals, k)), {static_cast<std::int64_t>(theta.size())}, x);
}

Series GaussianMixtureFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series GaussianMixtureFit::observed_inf(const Series& x) const {
  const auto xvals = fit_data_f64_(x, "GaussianMixtureFit::observed_inf");
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  const auto theta = gm_theta_(*this);
  const auto st = fit_detail::fit_stats(theta, std::vector<bool>(theta.size(), true), gm_loglik_(xvals, k));
  return fit_detail::matrix_series(st.info, st.k, x);
}

Series GaussianMixtureFit::vcov(const Series& x) const {
  const auto st = gm_stats_(*this, fit_data_f64_(x, "GaussianMixtureFit::vcov"));
  return fit_detail::matrix_series(st.vcov, st.natural.size(), x);
}

Series GaussianMixtureFit::stderr(const Series& x) const {
  const auto st = gm_stats_(*this, fit_data_f64_(x, "GaussianMixtureFit::stderr"));
  return fit_detail::vec_series(st.se, {static_cast<std::int64_t>(st.se.size())}, x);
}

Series GaussianMixtureFit::confint(const Series& x, double level) const {
  const auto st = gm_stats_(*this, fit_data_f64_(x, "GaussianMixtureFit::confint"));
  return fit_detail::interval_series(st.natural, st.se, wald_z_value_(level, "GaussianMixtureFit::confint"), x);
}

GaussianMixtureFit GaussianMixture::MLE(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  const auto xvals = fit_data_f64_(x, "GaussianMixture::MLE");
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  PLUSS_CHECK(k > 0, "GaussianMixture::MLE: need at least one component");
  auto w = normalized_mixture_weights(weights).pull<double>();
  auto m = means.to(ScalarType::Float64)
               .reshape({means.numel()})
               .pull<double>();
  auto s = stddevs.to(ScalarType::Float64)
               .reshape({stddevs.numel()})
               .pull<double>();

  if (m.size() != k) m.assign(k, 0.0);
  if (s.size() != k) s.assign(k, 1.0);

  const double n = static_cast<double>(xvals.size());
  const double mean_x = std::accumulate(xvals.begin(), xvals.end(), 0.0) / n;
  const double var_x = std::transform_reduce(xvals.begin(),
                                             xvals.end(),
                                             0.0,
                                             std::plus<>{},
                                             [mean_x](double v) {
                                               const double d = v - mean_x;
                                               return d * d;
                                             }) /
                       n;
  const double sd_x = std::max(std::sqrt(var_x), 1e-3);

  for (std::size_t i = 0; i < k; ++i) {
    if (!std::isfinite(m[i])) {
      m[i] = mean_x + (static_cast<double>(i) - 0.5 * static_cast<double>(k - 1)) * sd_x * 0.5;
    }
    if (!(s[i] > 0.0) || !std::isfinite(s[i])) {
      s[i] = sd_x;
    }
  }

  auto [w_fit, m_fit, s_fit] = em_gaussian_mixture_1d_(xvals, w, m, s);
  return GaussianMixtureFit{
      fit_detail::vec_series_cpu(w_fit, {static_cast<std::int64_t>(k)}).to(x.platform(), ScalarType::Float64),
      fit_detail::vec_series_cpu(m_fit, {static_cast<std::int64_t>(k)}).to(x.platform(), ScalarType::Float64),
      fit_detail::vec_series_cpu(s_fit, {static_cast<std::int64_t>(k)}).to(x.platform(), ScalarType::Float64)};
}

} // namespace pluss::subsp::stats::distr
