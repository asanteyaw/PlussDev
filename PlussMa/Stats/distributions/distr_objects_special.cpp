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
                        int iters = 40) {
  const std::size_t n = x.size();
  const std::size_t k = init_weights.size();
  std::vector<double> w = init_weights;
  std::vector<double> m = init_means;
  std::vector<double> s = init_stddevs;
  std::vector<double> resp(n * k, 0.0);

  for (int iter = 0; iter < iters; ++iter) {
    for (std::size_t i = 0; i < n; ++i) {
      double denom = 0.0;
      for (std::size_t j = 0; j < k; ++j) {
        const double r = w[j] * normal_pdf_scalar_(x[i], m[j], std::max(1e-6, s[j]));
        resp[i * k + j] = r;
        denom += r;
      }
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
  return detail::sample_with_params(dst::Nig, nig_params(alpha, beta, location, scale), sizes, cfg, seed);
}

Series Nig::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Nig::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Nig, nig_params(alpha, beta, location, scale), like, seed), like);
}

Series Nig::pdf(const Series& x) const { return detail::pdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale)); }

Series Nig::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale));
}

Series Nig::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale));
}

Series Nig::cdf(const Series& x) const { return detail::cdf_with_params(dst::Nig, x, nig_params(alpha, beta, location, scale)); }

Series Nig::cgf(const Series& t) const {
  detail::require_all_true(t.add(beta).abs().lt(alpha), "Nig::cgf requires every t element to satisfy |beta + t| < alpha");
  const double gamma = std::sqrt(alpha * alpha - beta * beta);
  return t.mul(location).add((t.add(beta).square().mul(-1.0).add(alpha * alpha).sqrt().mul(-scale)).add(scale * gamma));
}

Series Nig::cumulant(std::int64_t order) const {
  return detail::scalar_f64(nig_cumulant_scalar(alpha, beta, location, scale, order));
}

Series Gh::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::Gh, gh_params(lambda, alpha, beta, location, scale), sizes, cfg, seed);
}

Series Gh::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Gh::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Gh, gh_params(lambda, alpha, beta, location, scale), like, seed), like);
}

Series Gh::pdf(const Series& x) const { return detail::pdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale)); }

Series Gh::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale));
}

Series Gh::cdf(const Series& x) const { return detail::cdf_with_params(dst::Gh, x, gh_params(lambda, alpha, beta, location, scale)); }

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

Series NigFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "NigFit::score");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[0] > std::abs(p[1])) || !(p[3] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Nig{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad({alpha, beta, location, scale}, fn), {4}, x);
}

Series NigFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series NigFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "NigFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[0] > std::abs(p[1])) || !(p[3] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Nig{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_({alpha, beta, location, scale}, fn), x);
}

Series NigFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "NigFit::vcov");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[0] > std::abs(p[1])) || !(p[3] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Nig{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return vcov_diag_matrix_series_(info_diag_from_loglik_({alpha, beta, location, scale}, fn), x, "NigFit::vcov");
}

Series NigFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "NigFit::stderr");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[0] > std::abs(p[1])) || !(p[3] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Nig{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_({alpha, beta, location, scale}, fn)), {4}, x);
}

Series NigFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "NigFit::confint");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[0] > std::abs(p[1])) || !(p[3] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Nig{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {alpha, beta, location, scale};
  auto ci = confint_matrix_series_(p, stderr_from_info_diag_(info_diag_from_loglik_(p, fn)), x, level, "NigFit::confint");
  auto flat = ci.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  flat[6] = std::max(0.0, flat[6]);
  return fit_detail::vec_series_cpu(flat, {4, 2}).to(x.platform(), ScalarType::Float64);
}

NigFit Nig::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Nig::MLE");
  const double n = static_cast<double>(values.size());
  const double mean = std::accumulate(values.begin(), values.end(), 0.0) / n;
  const double var = std::transform_reduce(values.begin(),
                                           values.end(),
                                           0.0,
                                           std::plus<>{},
                                           [mean](double v) {
                                             const double d = v - mean;
                                             return d * d;
                                           }) /
                     n;
  const double sd = std::max(std::sqrt(var), 1e-6);
  const double min_x = *std::min_element(values.begin(), values.end());
  const double max_x = *std::max_element(values.begin(), values.end());
  const double loc_lo = min_x - 3.0 * sd;
  const double loc_hi = max_x + 3.0 * sd;
  const double log_scale_lo = std::log(std::max(0.05 * sd, 1e-6));
  const double log_scale_hi = std::log(std::max(20.0 * sd, 1e-5));

  double alpha_hat = std::max(alpha, 1.0 / sd);
  double beta_hat = std::clamp(beta, -0.95 * alpha_hat, 0.95 * alpha_hat);
  double location_hat = mean;
  double scale_hat = sd;

  std::vector<double> q = {location_hat, std::log(scale_hat)};
  const auto fn = [&](const std::vector<double>& qq) -> double {
    const double loc = qq[0];
    const double sc = std::exp(qq[1]);
    if (!(sc > 0.0) || !(alpha_hat > std::abs(beta_hat))) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Nig{alpha_hat, beta_hat, loc, sc}.logpdf(xs).sum().item<double>();
  };
  for (int it = 0; it < 20; ++it) {
    const auto g = fit_detail::fd_grad(q, fn);
    const double base = fn(q);
    double step = 0.05;
    bool improved = false;
    for (int ls = 0; ls < 10; ++ls) {
      std::vector<double> trial = q;
      trial[0] = std::clamp(trial[0] + step * g[0], loc_lo, loc_hi);
      trial[1] = std::clamp(trial[1] + step * g[1], log_scale_lo, log_scale_hi);
      const double val = fn(trial);
      if (std::isfinite(val) && val > base) {
        q = trial;
        improved = true;
        break;
      }
      step *= 0.5;
    }
    if (!improved) break;
  }
  location_hat = q[0];
  scale_hat = std::exp(q[1]);
  return NigFit{alpha_hat, beta_hat, location_hat, scale_hat};
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

Series GhFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "GhFit::score");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 5 || !(p[1] > std::abs(p[2])) || !(p[4] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gh{p[0], p[1], p[2], p[3], p[4]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad({lambda, alpha, beta, location, scale}, fn), {5}, x);
}

Series GhFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series GhFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "GhFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 5 || !(p[1] > std::abs(p[2])) || !(p[4] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gh{p[0], p[1], p[2], p[3], p[4]}.logpdf(xs).sum().item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_({lambda, alpha, beta, location, scale}, fn), x);
}

Series GhFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "GhFit::vcov");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 5 || !(p[1] > std::abs(p[2])) || !(p[4] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gh{p[0], p[1], p[2], p[3], p[4]}.logpdf(xs).sum().item<double>();
  };
  return vcov_diag_matrix_series_(info_diag_from_loglik_({lambda, alpha, beta, location, scale}, fn), x, "GhFit::vcov");
}

Series GhFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "GhFit::stderr");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 5 || !(p[1] > std::abs(p[2])) || !(p[4] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gh{p[0], p[1], p[2], p[3], p[4]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_({lambda, alpha, beta, location, scale}, fn)), {5}, x);
}

Series GhFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "GhFit::confint");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 5 || !(p[1] > std::abs(p[2])) || !(p[4] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gh{p[0], p[1], p[2], p[3], p[4]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {lambda, alpha, beta, location, scale};
  auto ci = confint_matrix_series_(p, stderr_from_info_diag_(info_diag_from_loglik_(p, fn)), x, level, "GhFit::confint");
  auto flat = ci.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  flat[8] = std::max(0.0, flat[8]);
  return fit_detail::vec_series_cpu(flat, {5, 2}).to(x.platform(), ScalarType::Float64);
}

GhFit Gh::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Gh::MLE");
  const double n = static_cast<double>(values.size());
  const double mean = std::accumulate(values.begin(), values.end(), 0.0) / n;
  const double var = std::transform_reduce(values.begin(),
                                           values.end(),
                                           0.0,
                                           std::plus<>{},
                                           [mean](double v) {
                                             const double d = v - mean;
                                             return d * d;
                                           }) /
                     n;
  const double sd = std::max(std::sqrt(var), 1e-6);
  const double min_x = *std::min_element(values.begin(), values.end());
  const double max_x = *std::max_element(values.begin(), values.end());
  const double loc_lo = min_x - 3.0 * sd;
  const double loc_hi = max_x + 3.0 * sd;
  const double log_scale_lo = std::log(std::max(0.05 * sd, 1e-6));
  const double log_scale_hi = std::log(std::max(20.0 * sd, 1e-5));

  double lambda_hat = lambda;
  double alpha_hat = std::max(alpha, 1.0 / sd);
  double beta_hat = std::clamp(beta, -0.95 * alpha_hat, 0.95 * alpha_hat);
  double location_hat = mean;
  double scale_hat = sd;

  std::vector<double> q = {location_hat, std::log(scale_hat)};
  const auto fn = [&](const std::vector<double>& qq) -> double {
    const double loc = qq[0];
    const double sc = std::exp(qq[1]);
    if (!(sc > 0.0) || !(alpha_hat > std::abs(beta_hat))) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gh{lambda_hat, alpha_hat, beta_hat, loc, sc}.logpdf(xs).sum().item<double>();
  };
  for (int it = 0; it < 20; ++it) {
    const auto g = fit_detail::fd_grad(q, fn);
    const double base = fn(q);
    double step = 0.05;
    bool improved = false;
    for (int ls = 0; ls < 10; ++ls) {
      std::vector<double> trial = q;
      trial[0] = std::clamp(trial[0] + step * g[0], loc_lo, loc_hi);
      trial[1] = std::clamp(trial[1] + step * g[1], log_scale_lo, log_scale_hi);
      const double val = fn(trial);
      if (std::isfinite(val) && val > base) {
        q = trial;
        improved = true;
        break;
      }
      step *= 0.5;
    }
    if (!improved) break;
  }
  location_hat = q[0];
  scale_hat = std::exp(q[1]);
  return GhFit{lambda_hat, alpha_hat, beta_hat, location_hat, scale_hat};
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

Series GaussianMixtureFit::score(const Series& x) const {
  const auto xvals = fit_data_f64_(x, "GaussianMixtureFit::score");
  require_mixture_shape_compatibility(weights, means, stddevs);
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  const auto w = normalized_mixture_weights(weights).to_vec<double>();
  const auto m = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()}).to_vec<double>();
  const auto s = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()}).to_vec<double>();

  std::vector<double> theta;
  theta.reserve((k - 1) + k + k);
  for (std::size_t i = 0; i + 1 < k; ++i) {
    theta.push_back(std::log(std::max(w[i], 1e-12) / std::max(w[k - 1], 1e-12)));
  }
  theta.insert(theta.end(), m.begin(), m.end());
  std::transform(s.begin(), s.end(), std::back_inserter(theta), [](double v) { return std::log(std::max(v, 1e-6)); });

  const auto fn = [&](const std::vector<double>& t) -> double {
    if (t.size() != (k - 1) + k + k) return kFitPenalty;
    std::vector<double> logits(k, 0.0);
    std::copy(t.begin(), t.begin() + static_cast<std::int64_t>(k - 1), logits.begin());
    const auto ww = softmax_logits_(logits);
    std::vector<double> mm(t.begin() + static_cast<std::int64_t>(k - 1), t.begin() + static_cast<std::int64_t>(k - 1 + k));
    std::vector<double> ss(k, 1.0);
    std::transform(t.begin() + static_cast<std::int64_t>(k - 1 + k), t.end(), ss.begin(), [](double ls) { return std::exp(ls); });
    const Series xs = fit_detail::vec_series_cpu(xvals, {static_cast<std::int64_t>(xvals.size())});
    return GaussianMixture{
               fit_detail::vec_series_cpu(ww, {static_cast<std::int64_t>(k)}),
               fit_detail::vec_series_cpu(mm, {static_cast<std::int64_t>(k)}),
               fit_detail::vec_series_cpu(ss, {static_cast<std::int64_t>(k)})}
        .logpdf(xs)
        .sum()
        .item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad(theta, fn), {static_cast<std::int64_t>(theta.size())}, x);
}

Series GaussianMixtureFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series GaussianMixtureFit::observed_inf(const Series& x) const {
  const auto xvals = fit_data_f64_(x, "GaussianMixtureFit::observed_inf");
  require_mixture_shape_compatibility(weights, means, stddevs);
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  const auto w = normalized_mixture_weights(weights).to_vec<double>();
  const auto m = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()}).to_vec<double>();
  const auto s = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()}).to_vec<double>();
  std::vector<double> theta;
  theta.reserve((k - 1) + k + k);
  for (std::size_t i = 0; i + 1 < k; ++i) theta.push_back(std::log(std::max(w[i], 1e-12) / std::max(w[k - 1], 1e-12)));
  theta.insert(theta.end(), m.begin(), m.end());
  std::transform(s.begin(), s.end(), std::back_inserter(theta), [](double v) { return std::log(std::max(v, 1e-6)); });

  const auto fn = [&](const std::vector<double>& t) -> double {
    if (t.size() != (k - 1) + k + k) return kFitPenalty;
    std::vector<double> logits(k, 0.0);
    std::copy(t.begin(), t.begin() + static_cast<std::int64_t>(k - 1), logits.begin());
    const auto ww = softmax_logits_(logits);
    std::vector<double> mm(t.begin() + static_cast<std::int64_t>(k - 1), t.begin() + static_cast<std::int64_t>(k - 1 + k));
    std::vector<double> ss(k, 1.0);
    std::transform(t.begin() + static_cast<std::int64_t>(k - 1 + k), t.end(), ss.begin(), [](double ls) { return std::exp(ls); });
    const Series xs = fit_detail::vec_series_cpu(xvals, {static_cast<std::int64_t>(xvals.size())});
    return GaussianMixture{
               fit_detail::vec_series_cpu(ww, {static_cast<std::int64_t>(k)}),
               fit_detail::vec_series_cpu(mm, {static_cast<std::int64_t>(k)}),
               fit_detail::vec_series_cpu(ss, {static_cast<std::int64_t>(k)})}
        .logpdf(xs)
        .sum()
        .item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_(theta, fn), x);
}

Series GaussianMixtureFit::vcov(const Series& x) const {
  const auto info = observed_inf(x).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const std::int64_t n = static_cast<std::int64_t>(std::sqrt(static_cast<double>(info.size())));
  std::vector<double> diag(static_cast<std::size_t>(n), kMinInfo);
  for (std::int64_t i = 0; i < n; ++i) diag[static_cast<std::size_t>(i)] = std::max(info[static_cast<std::size_t>(i * n + i)], kMinInfo);
  return vcov_diag_matrix_series_(diag, x, "GaussianMixtureFit::vcov");
}

Series GaussianMixtureFit::stderr(const Series& x) const {
  const auto info = observed_inf(x).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const std::int64_t n = static_cast<std::int64_t>(std::sqrt(static_cast<double>(info.size())));
  std::vector<double> diag(static_cast<std::size_t>(n), kMinInfo);
  for (std::int64_t i = 0; i < n; ++i) diag[static_cast<std::size_t>(i)] = std::max(info[static_cast<std::size_t>(i * n + i)], kMinInfo);
  return fit_detail::vec_series(stderr_from_info_diag_(diag), {n}, x);
}

Series GaussianMixtureFit::confint(const Series& x, double level) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  const auto w = normalized_mixture_weights(weights).to_vec<double>();
  const auto m = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()}).to_vec<double>();
  const auto s = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()}).to_vec<double>();
  std::vector<double> params;
  params.reserve((k - 1) + k + k);
  for (std::size_t i = 0; i + 1 < k; ++i) {
    params.push_back(std::log(std::max(w[i], 1e-12) / std::max(w[k - 1], 1e-12)));
  }
  params.insert(params.end(), m.begin(), m.end());
  std::transform(s.begin(), s.end(), std::back_inserter(params), [](double v) { return std::log(std::max(v, 1e-6)); });
  const auto se = stderr(x).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  return confint_matrix_series_(params, se, x, level, "GaussianMixtureFit::confint");
}

GaussianMixtureFit GaussianMixture::MLE(const Series& x) const {
  require_mixture_shape_compatibility(weights, means, stddevs);
  const auto xvals = fit_data_f64_(x, "GaussianMixture::MLE");
  const std::size_t k = static_cast<std::size_t>(weights.numel());
  PLUSS_CHECK(k > 0, "GaussianMixture::MLE: need at least one component");
  auto w = normalized_mixture_weights(weights).to_vec<double>();
  auto m = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()}).to_vec<double>();
  auto s = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()}).to_vec<double>();

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

  auto [w_fit, m_fit, s_fit] = em_gaussian_mixture_1d_(xvals, w, m, s, 40);
  return GaussianMixtureFit{
      fit_detail::vec_series_cpu(w_fit, {static_cast<std::int64_t>(k)}).to(x.platform(), ScalarType::Float64),
      fit_detail::vec_series_cpu(m_fit, {static_cast<std::int64_t>(k)}).to(x.platform(), ScalarType::Float64),
      fit_detail::vec_series_cpu(s_fit, {static_cast<std::int64_t>(k)}).to(x.platform(), ScalarType::Float64)};
}

} // namespace pluss::subsp::stat::distr
