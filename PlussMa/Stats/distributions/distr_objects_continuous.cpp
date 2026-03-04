#include "distr_object_detail.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numbers>
#include <span>
#include <vector>

namespace pluss::subsp::stat::distr {
namespace {

std::array<double, 2> normal_params(double mean, double stddev) {
  return {mean, stddev};
}

std::array<double, 3> gamma_params(double shape, double scale, double location) {
  return {location, scale, shape};
}

std::array<double, 1> chisq_params(std::int64_t degrees_of_freedom) {
  return {static_cast<double>(degrees_of_freedom)};
}

std::array<double, 2> inverse_gaussian_params(double theta, double chi) {
  return {theta, chi};
}

std::array<double, 2> lognormal_params(double mean_log, double stddev_log) {
  return {mean_log, stddev_log};
}

std::array<double, 2> uniform_params(double low, double high) {
  return {low, high};
}

std::array<double, 1> exponential_params(double rate) {
  return {rate};
}

std::array<double, 2> laplace_params(double location, double scale) {
  return {location, scale};
}

std::array<double, 2> logistic_params(double location, double scale) {
  return {location, scale};
}

std::array<double, 1> rayleigh_params(double sigma) {
  return {sigma};
}

std::array<double, 4> normaltrunc_params(double lower, double upper, double mean, double stddev) {
  return {lower, upper, mean, stddev};
}

std::array<double, 3> student_t_params(double degrees_of_freedom, double location, double scale) {
  return {degrees_of_freedom, location, scale};
}

std::array<double, 2> f_params(double numerator_degrees_of_freedom, double denominator_degrees_of_freedom) {
  return {numerator_degrees_of_freedom, denominator_degrees_of_freedom};
}

std::array<double, 2> weibull_params(double shape, double scale) {
  return {shape, scale};
}

std::array<double, 4> beta_params(double low, double high, double alpha, double beta) {
  return {low, high, alpha, beta};
}

Series stable_log_expm1_over_x(const Series& x) {
  const Series small = x.abs().lt(1e-5);
  const Series general = x.expm1().div(x).log();
  const Series series = x.mul(0.5).add(x.mul(x).div(24.0));
  return general.where(small.logical_not(), series);
}

Series stable_log_x_over_sin_x(const Series& x) {
  const Series small = x.abs().lt(1e-4);
  const Series x2 = x.square();
  const Series series = x2.div(6.0).add(x2.square().div(180.0)).add(x2.square().mul(x2).div(2835.0));
  const Series general = x.div(x.sin()).log();
  return general.where(small.logical_not(), series);
}

double rayleigh_raw_moment(double sigma, std::int64_t order) {
  return std::pow(sigma, static_cast<double>(order)) *
         std::pow(2.0, 0.5 * static_cast<double>(order)) *
         std::tgamma(1.0 + 0.5 * static_cast<double>(order));
}

double std_normal_pdf_scalar(double x) {
  return std::exp(-0.5 * x * x) / std::sqrt(2.0 * std::numbers::pi_v<double>);
}

double std_normal_logcdf_scalar(double x) {
  return ::pluss::subsp::stat::distr::logcdf("normal", detail::scalar_f64(x), std::initializer_list<double>{}).item<double>();
}

double logsubexp_scalar(double log_hi, double log_lo) {
  if (log_lo == -std::numeric_limits<double>::infinity()) return log_hi;
  return log_hi + std::log1p(-std::exp(log_lo - log_hi));
}

std::vector<double> normaltrunc_standard_raw_moments(double alpha, double beta, std::int64_t order) {
  if (!(beta >= alpha)) {
    throw std::domain_error("NormalTrunc::cumulant requires upper >= lower");
  }
  const double log_z = logsubexp_scalar(std_normal_logcdf_scalar(beta), std_normal_logcdf_scalar(alpha));
  const double z = std::exp(log_z);
  const double phi_alpha = std_normal_pdf_scalar(alpha);
  const double phi_beta = std_normal_pdf_scalar(beta);
  std::vector<double> moments(static_cast<std::size_t>(order) + 1, 0.0);
  moments[0] = 1.0;
  if (order >= 1) {
    moments[1] = (phi_alpha - phi_beta) / z;
  }
  for (std::int64_t n = 2; n <= order; ++n) {
    moments[static_cast<std::size_t>(n)] =
        (static_cast<double>(n - 1) * moments[static_cast<std::size_t>(n - 2)] +
         (std::pow(alpha, static_cast<double>(n - 1)) * phi_alpha -
          std::pow(beta, static_cast<double>(n - 1)) * phi_beta) / z);
  }
  return moments;
}

double integrate_simpson(const std::function<double(double)>& fn, double a, double b, int n = 2048) {
  if (b <= a) {
    return 0.0;
  }
  if (n % 2 != 0) {
    ++n;
  }
  const double h = (b - a) / static_cast<double>(n);
  double acc = fn(a) + fn(b);
  for (int i = 1; i < n; ++i) {
    const double x = a + h * static_cast<double>(i);
    acc += fn(x) * ((i % 2 == 0) ? 2.0 : 4.0);
  }
  return acc * h / 3.0;
}

double log_mgf_from_moments_series(const std::function<long double(std::int64_t)>& raw_moment_fn,
                                   double t,
                                   std::string_view name,
                                   int max_terms = 256) {
  long double sum = 1.0L;
  long double term = 1.0L;
  for (int n = 1; n <= max_terms; ++n) {
    term *= static_cast<long double>(t) / static_cast<long double>(n);
    const long double add = term * raw_moment_fn(n);
    sum += add;
    if (std::abs(add) <= 1e-18L * std::max(1.0L, std::abs(sum))) {
      break;
    }
    if (n == max_terms) {
      throw std::runtime_error(std::string(name) + "::cgf series did not converge");
    }
  }
  if (!(sum > 0.0L) || !std::isfinite(static_cast<double>(sum))) {
    throw std::runtime_error(std::string(name) + "::cgf encountered a non-positive or non-finite mgf");
  }
  return std::log(static_cast<double>(sum));
}

double inverse_gaussian_cumulant_scalar(double theta, double chi, std::int64_t order) {
  if (order <= 0) {
    throw std::invalid_argument("InverseGaussian::cumulant: order must be >= 1");
  }
  if (!(theta > 0.0) || !(chi > 0.0)) {
    throw std::domain_error("InverseGaussian::cumulant requires theta > 0 and chi > 0");
  }
  return detail::odd_double_factorial(2 * order - 3) *
         std::pow(theta, static_cast<double>(2 * order - 1)) /
         std::pow(chi, static_cast<double>(order - 1));
}

double lognormal_raw_moment(double mean_log, double stddev_log, std::int64_t order) {
  const double n = static_cast<double>(order);
  return std::exp(n * mean_log + 0.5 * n * n * stddev_log * stddev_log);
}

double student_t_standard_raw_moment(double degrees_of_freedom, std::int64_t order) {
  if (order % 2 == 1) {
    return 0.0;
  }
  if (!(degrees_of_freedom > static_cast<double>(order))) {
    throw std::domain_error("StudentT::cumulant requires degrees_of_freedom > order");
  }
  const double m = 0.5 * static_cast<double>(order);
  return std::pow(degrees_of_freedom, m) *
         std::tgamma(m + 0.5) *
         std::tgamma(0.5 * (degrees_of_freedom - static_cast<double>(order))) /
         (std::sqrt(std::numbers::pi_v<double>) * std::tgamma(0.5 * degrees_of_freedom));
}

double f_raw_moment(double numerator_degrees_of_freedom,
                    double denominator_degrees_of_freedom,
                    std::int64_t order) {
  if (!(denominator_degrees_of_freedom > 2.0 * static_cast<double>(order))) {
    throw std::domain_error("FDistribution::cumulant requires denominator_degrees_of_freedom > 2 * order");
  }
  const double n = static_cast<double>(order);
  return std::pow(denominator_degrees_of_freedom / numerator_degrees_of_freedom, n) *
         std::tgamma(0.5 * numerator_degrees_of_freedom + n) *
         std::tgamma(0.5 * denominator_degrees_of_freedom - n) /
         (std::tgamma(0.5 * numerator_degrees_of_freedom) * std::tgamma(0.5 * denominator_degrees_of_freedom));
}

double beta_raw_moment(double low, double high, double alpha, double beta, std::int64_t order) {
  const double width = high - low;
  double moment = 0.0;
  for (std::int64_t k = 0; k <= order; ++k) {
    moment += detail::binomial_coeff(order, k) *
              std::pow(low, static_cast<double>(order - k)) *
              std::pow(width, static_cast<double>(k)) *
              detail::rising_factorial(alpha, k) /
              detail::rising_factorial(alpha + beta, k);
  }
  return moment;
}

double weibull_raw_moment(double shape, double scale, std::int64_t order) {
  return std::pow(scale, static_cast<double>(order)) *
         std::tgamma(1.0 + static_cast<double>(order) / shape);
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

Series Normal::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Normal::cumulant");
  if (order == 1) return detail::scalar_f64(mean);
  if (order == 2) return detail::scalar_f64(stddev * stddev);
  return detail::scalar_f64(0.0);
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

Series Gamma::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Gamma::cumulant");
  if (order == 1) {
    return detail::scalar_f64(location + shape * scale);
  }
  return detail::scalar_f64(shape * detail::factorial(order - 1) * std::pow(scale, static_cast<double>(order)));
}

Series ChiSquare::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("chisq", chisq_params(degrees_of_freedom), sizes, cfg, seed);
}

Series ChiSquare::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series ChiSquare::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("chisq", chisq_params(degrees_of_freedom), like, seed), like);
}

Series ChiSquare::pdf(const Series& x) const { return detail::pdf_with_params("chisq", x, chisq_params(degrees_of_freedom)); }

Series ChiSquare::logpdf(const Series& x) const {
  return detail::logpdf_with_params("chisq", x, chisq_params(degrees_of_freedom));
}

Series ChiSquare::logcdf(const Series& x) const {
  return detail::logcdf_with_params("chisq", x, chisq_params(degrees_of_freedom));
}

Series ChiSquare::cdf(const Series& x) const { return detail::cdf_with_params("chisq", x, chisq_params(degrees_of_freedom)); }

Series ChiSquare::cgf(const Series& t) const {
  const double half_df = 0.5 * static_cast<double>(degrees_of_freedom);
  detail::require_all_true(t.lt(0.5), "ChiSquare::cgf requires every t element to satisfy t < 1/2");
  return t.mul(-2.0).log1p().mul(-half_df);
}

Series ChiSquare::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "ChiSquare::cumulant");
  return detail::scalar_f64(static_cast<double>(degrees_of_freedom) *
                            detail::factorial(order - 1) * std::pow(2.0, static_cast<double>(order - 1)));
}

Series InverseGaussian::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("ig", inverse_gaussian_params(theta, chi), sizes, cfg, seed);
}

Series InverseGaussian::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series InverseGaussian::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("ig", inverse_gaussian_params(theta, chi), like, seed), like);
}

Series InverseGaussian::pdf(const Series& x) const {
  return detail::pdf_with_params("ig", x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::logpdf(const Series& x) const {
  return detail::logpdf_with_params("ig", x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::logcdf(const Series& x) const {
  return detail::logcdf_with_params("ig", x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::cdf(const Series& x) const {
  return detail::cdf_with_params("ig", x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::cgf(const Series& t) const {
  detail::require_all_true(t.mul(2.0 * theta * theta / chi).lt(1.0),
                           "InverseGaussian::cgf requires every t element to satisfy 2 * theta^2 * t / chi < 1");
  return t.mul(-2.0 * theta * theta / chi).add(1.0).sqrt().mul(-chi / theta).add(chi / theta);
}

Series InverseGaussian::cumulant(std::int64_t order) const {
  return detail::scalar_f64(inverse_gaussian_cumulant_scalar(theta, chi, order));
}

Series Lognormal::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("lognormal", lognormal_params(mean_log, stddev_log), sizes, cfg, seed);
}

Series Lognormal::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Lognormal::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("lognormal", lognormal_params(mean_log, stddev_log), like, seed), like);
}

Series Lognormal::pdf(const Series& x) const { return detail::pdf_with_params("lognormal", x, lognormal_params(mean_log, stddev_log)); }

Series Lognormal::logpdf(const Series& x) const {
  return detail::logpdf_with_params("lognormal", x, lognormal_params(mean_log, stddev_log));
}

Series Lognormal::logcdf(const Series& x) const {
  return detail::logcdf_with_params("lognormal", x, lognormal_params(mean_log, stddev_log));
}

Series Lognormal::cdf(const Series& x) const { return detail::cdf_with_params("lognormal", x, lognormal_params(mean_log, stddev_log)); }

Series Lognormal::cgf(const Series& t) const {
  const double mean_log_value = mean_log;
  const double stddev_log_value = stddev_log;
  detail::require_all_true(t.le(0.0), "Lognormal::cgf only supports t <= 0 because the mgf diverges for t > 0");
  return detail::map_f64_like(t, [mean_log_value, stddev_log_value](double tv) {
    if (tv == 0.0) {
      return 0.0;
    }
    const auto integrand = [mean_log_value, stddev_log_value, tv](double z) {
      const double logx = mean_log_value + stddev_log_value * z;
      return std::exp(tv * std::exp(logx) - 0.5 * z * z) / std::sqrt(2.0 * std::numbers::pi_v<double>);
    };
    const double mgf = integrate_simpson(integrand, -12.0, 12.0);
    return std::log(std::max(mgf, std::numeric_limits<double>::min()));
  });
}

Series Lognormal::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Lognormal::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = lognormal_raw_moment(mean_log, stddev_log, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
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

Series Uniform::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Uniform::cumulant");
  const double width = high - low;
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] =
        (std::pow(high, static_cast<double>(n + 1)) - std::pow(low, static_cast<double>(n + 1))) /
        (static_cast<double>(n + 1) * width);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
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

Series Exponential::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Exponential::cumulant");
  return detail::scalar_f64(detail::factorial(order - 1) / std::pow(rate, static_cast<double>(order)));
}

Series Laplace::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("laplace", laplace_params(location, scale), sizes, cfg, seed);
}

Series Laplace::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Laplace::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("laplace", laplace_params(location, scale), like, seed), like);
}

Series Laplace::pdf(const Series& x) const { return detail::pdf_with_params("laplace", x, laplace_params(location, scale)); }

Series Laplace::logpdf(const Series& x) const {
  return detail::logpdf_with_params("laplace", x, laplace_params(location, scale));
}

Series Laplace::logcdf(const Series& x) const {
  return detail::logcdf_with_params("laplace", x, laplace_params(location, scale));
}

Series Laplace::cdf(const Series& x) const { return detail::cdf_with_params("laplace", x, laplace_params(location, scale)); }

Series Laplace::cgf(const Series& t) const {
  detail::require_all_true(t.abs().lt(1.0 / scale), "Laplace::cgf requires every t element to satisfy |t| < 1 / scale");
  return t.mul(location).sub(t.mul(scale).square().neg().log1p());
}

Series Laplace::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Laplace::cumulant");
  if (order == 1) return detail::scalar_f64(location);
  if (order % 2 == 1) return detail::scalar_f64(0.0);
  const std::int64_t half_order = order / 2;
  return detail::scalar_f64(detail::factorial(order) * std::pow(scale, static_cast<double>(order)) /
                            static_cast<double>(half_order));
}

Series Logistic::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("logistic", logistic_params(location, scale), sizes, cfg, seed);
}

Series Logistic::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Logistic::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("logistic", logistic_params(location, scale), like, seed), like);
}

Series Logistic::pdf(const Series& x) const { return detail::pdf_with_params("logistic", x, logistic_params(location, scale)); }

Series Logistic::logpdf(const Series& x) const {
  return detail::logpdf_with_params("logistic", x, logistic_params(location, scale));
}

Series Logistic::logcdf(const Series& x) const {
  return detail::logcdf_with_params("logistic", x, logistic_params(location, scale));
}

Series Logistic::cdf(const Series& x) const { return detail::cdf_with_params("logistic", x, logistic_params(location, scale)); }

Series Logistic::cgf(const Series& t) const {
  detail::require_all_true(t.mul(scale).abs().lt(1.0), "Logistic::cgf requires every t element to satisfy |scale * t| < 1");
  const Series scaled = t.mul(std::numbers::pi_v<double> * scale);
  return t.mul(location).add(stable_log_x_over_sin_x(scaled));
}

Series Logistic::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Logistic::cumulant");
  if (order == 1) return detail::scalar_f64(location);
  if (order % 2 == 1) return detail::scalar_f64(0.0);
  const std::int64_t half_order = order / 2;
  const long double bernoulli = std::abs(detail::bernoulli_number(order));
  const long double factor = std::pow(2.0L, order - 1) * bernoulli / static_cast<long double>(half_order);
  const long double scaled = std::pow(std::numbers::pi_v<long double> * static_cast<long double>(scale), order);
  return detail::scalar_f64(static_cast<double>(factor * scaled));
}

Series Rayleigh::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("rayleigh", rayleigh_params(sigma), sizes, cfg, seed);
}

Series Rayleigh::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Rayleigh::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("rayleigh", rayleigh_params(sigma), like, seed), like);
}

Series Rayleigh::pdf(const Series& x) const { return detail::pdf_with_params("rayleigh", x, rayleigh_params(sigma)); }

Series Rayleigh::logpdf(const Series& x) const {
  return detail::logpdf_with_params("rayleigh", x, rayleigh_params(sigma));
}

Series Rayleigh::logcdf(const Series& x) const {
  return detail::logcdf_with_params("rayleigh", x, rayleigh_params(sigma));
}

Series Rayleigh::cdf(const Series& x) const { return detail::cdf_with_params("rayleigh", x, rayleigh_params(sigma)); }

Series Rayleigh::cgf(const Series& t) const {
  const Series scaled = t.mul(sigma);
  const Series term = scaled.mul(std::sqrt(std::numbers::pi_v<double> / 2.0))
                          .mul(scaled.square().mul(0.5).exp())
                          .mul(scaled.div(std::sqrt(2.0)).erf().add(1.0));
  return term.log1p();
}

Series Rayleigh::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Rayleigh::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = rayleigh_raw_moment(sigma, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series NormalTrunc::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("normaltrunc", normaltrunc_params(lower, upper, mean, stddev), sizes, cfg, seed);
}

Series NormalTrunc::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series NormalTrunc::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("normaltrunc", normaltrunc_params(lower, upper, mean, stddev), like, seed), like);
}

Series NormalTrunc::pdf(const Series& x) const { return detail::pdf_with_params("normaltrunc", x, normaltrunc_params(lower, upper, mean, stddev)); }

Series NormalTrunc::logpdf(const Series& x) const {
  return detail::logpdf_with_params("normaltrunc", x, normaltrunc_params(lower, upper, mean, stddev));
}

Series NormalTrunc::logcdf(const Series& x) const {
  return detail::logcdf_with_params("normaltrunc", x, normaltrunc_params(lower, upper, mean, stddev));
}

Series NormalTrunc::cdf(const Series& x) const { return detail::cdf_with_params("normaltrunc", x, normaltrunc_params(lower, upper, mean, stddev)); }

Series NormalTrunc::cgf(const Series& t) const {
  if (!(upper >= lower) || !(stddev > 0.0)) {
    throw std::domain_error("NormalTrunc::cgf requires upper >= lower and stddev > 0");
  }
  const double alpha = (lower - mean) / stddev;
  const double beta = (upper - mean) / stddev;
  const double log_denom = logsubexp_scalar(std_normal_logcdf_scalar(beta), std_normal_logcdf_scalar(alpha));
  const auto t_vals = t.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::vector<double> out;
  out.reserve(t_vals.size());
  for (double tv : t_vals) {
    const double log_num_scalar =
        logsubexp_scalar(std_normal_logcdf_scalar(beta - stddev * tv), std_normal_logcdf_scalar(alpha - stddev * tv));
    out.push_back(mean * tv + 0.5 * stddev * stddev * tv * tv + log_num_scalar - log_denom);
  }
  return pluss::codem::make_series(out, t.sizes(), Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU))
      .to(t.platform(), ScalarType::Float64);
}

Series NormalTrunc::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "NormalTrunc::cumulant");
  if (!(upper >= lower) || !(stddev > 0.0)) {
    throw std::domain_error("NormalTrunc::cumulant requires upper >= lower and stddev > 0");
  }
  const double alpha = (lower - mean) / stddev;
  const double beta = (upper - mean) / stddev;
  std::vector<double> std_raw = normaltrunc_standard_raw_moments(alpha, beta, order);
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    double moment = 0.0;
    for (std::int64_t k = 0; k <= n; ++k) {
      moment += detail::binomial_coeff(n, k) *
                std::pow(mean, static_cast<double>(n - k)) *
                std::pow(stddev, static_cast<double>(k)) *
                std_raw[static_cast<std::size_t>(k)];
    }
    raw[static_cast<std::size_t>(n)] = moment;
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series StudentT::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("t", student_t_params(degrees_of_freedom, location, scale), sizes, cfg, seed);
}

Series StudentT::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series StudentT::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("t", student_t_params(degrees_of_freedom, location, scale), like, seed), like);
}

Series StudentT::pdf(const Series& x) const { return detail::pdf_with_params("t", x, student_t_params(degrees_of_freedom, location, scale)); }

Series StudentT::logpdf(const Series& x) const {
  return detail::logpdf_with_params("t", x, student_t_params(degrees_of_freedom, location, scale));
}

Series StudentT::logcdf(const Series& x) const {
  return detail::logcdf_with_params("t", x, student_t_params(degrees_of_freedom, location, scale));
}

Series StudentT::cdf(const Series& x) const { return detail::cdf_with_params("t", x, student_t_params(degrees_of_freedom, location, scale)); }

Series StudentT::cgf(const Series& t) const {
  return detail::zero_only_cgf(t, "StudentT");
}

Series StudentT::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "StudentT::cumulant");
  if (!(degrees_of_freedom > static_cast<double>(order))) {
    throw std::domain_error("StudentT::cumulant requires degrees_of_freedom > order");
  }
  std::vector<double> centered(static_cast<std::size_t>(order) + 1, 0.0);
  centered[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    centered[static_cast<std::size_t>(n)] =
        std::pow(scale, static_cast<double>(n)) * student_t_standard_raw_moment(degrees_of_freedom, n);
  }
  const auto raw = detail::raw_moments_from_centered(centered, location);
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series FDistribution::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("f", f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom), sizes, cfg, seed);
}

Series FDistribution::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series FDistribution::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("f", f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom), like, seed), like);
}

Series FDistribution::pdf(const Series& x) const {
  return detail::pdf_with_params("f", x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::logpdf(const Series& x) const {
  return detail::logpdf_with_params("f", x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::logcdf(const Series& x) const {
  return detail::logcdf_with_params("f", x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::cdf(const Series& x) const {
  return detail::cdf_with_params("f", x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::cgf(const Series& t) const {
  return detail::zero_only_cgf(t, "FDistribution");
}

Series FDistribution::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "FDistribution::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] =
        f_raw_moment(numerator_degrees_of_freedom, denominator_degrees_of_freedom, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series Weibull::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("weibull", weibull_params(shape, scale), sizes, cfg, seed);
}

Series Weibull::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Weibull::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("weibull", weibull_params(shape, scale), like, seed), like);
}

Series Weibull::pdf(const Series& x) const { return detail::pdf_with_params("weibull", x, weibull_params(shape, scale)); }

Series Weibull::logpdf(const Series& x) const {
  return detail::logpdf_with_params("weibull", x, weibull_params(shape, scale));
}

Series Weibull::logcdf(const Series& x) const {
  return detail::logcdf_with_params("weibull", x, weibull_params(shape, scale));
}

Series Weibull::cdf(const Series& x) const { return detail::cdf_with_params("weibull", x, weibull_params(shape, scale)); }

Series Weibull::cgf(const Series& t) const {
  const double shape_value = shape;
  const double scale_value = scale;
  if (!(shape > 0.0) || !(scale > 0.0)) {
    throw std::domain_error("Weibull::cgf requires shape > 0 and scale > 0");
  }
  if (shape == 1.0) {
    detail::require_all_true(t.lt(1.0 / scale), "Weibull::cgf requires every t element to satisfy t < 1 / scale when shape = 1");
    return t.mul(-scale).log1p().neg();
  }
  if (shape < 1.0) {
    detail::require_all_true(t.le(0.0), "Weibull::cgf only supports t <= 0 when shape < 1");
    return detail::map_f64_like(t, [shape_value, scale_value](double tv) {
      if (tv == 0.0) return 0.0;
      const auto integrand = [shape_value, scale_value, tv](double y) {
        return std::exp(tv * scale_value * std::pow(y, 1.0 / shape_value) - y);
      };
      const double mgf = integrate_simpson(integrand, 0.0, 80.0);
      return std::log(std::max(mgf, std::numeric_limits<double>::min()));
    });
  }
  return detail::map_f64_like(t, [shape_value, scale_value](double tv) {
    return log_mgf_from_moments_series([shape_value, scale_value](std::int64_t n) {
      return static_cast<long double>(weibull_raw_moment(shape_value, scale_value, n));
    }, tv, "Weibull");
  });
}

Series Weibull::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Weibull::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = weibull_raw_moment(shape, scale, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series Beta::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params("beta", beta_params(low, high, alpha, beta), sizes, cfg, seed);
}

Series Beta::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Beta::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params("beta", beta_params(low, high, alpha, beta), like, seed), like);
}

Series Beta::pdf(const Series& x) const { return detail::pdf_with_params("beta", x, beta_params(low, high, alpha, beta)); }

Series Beta::logpdf(const Series& x) const {
  return detail::logpdf_with_params("beta", x, beta_params(low, high, alpha, beta));
}

Series Beta::logcdf(const Series& x) const {
  return detail::logcdf_with_params("beta", x, beta_params(low, high, alpha, beta));
}

Series Beta::cdf(const Series& x) const { return detail::cdf_with_params("beta", x, beta_params(low, high, alpha, beta)); }

Series Beta::cgf(const Series& t) const {
  const double low_value = low;
  const double high_value = high;
  const double alpha_value = alpha;
  const double beta_value = beta;
  return detail::map_f64_like(t, [low_value, high_value, alpha_value, beta_value](double tv) {
    return log_mgf_from_moments_series([low_value, high_value, alpha_value, beta_value](std::int64_t n) {
      return static_cast<long double>(beta_raw_moment(low_value, high_value, alpha_value, beta_value, n));
    }, tv, "Beta");
  });
}

Series Beta::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Beta::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = beta_raw_moment(low, high, alpha, beta, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

} // namespace pluss::subsp::stat::distr
