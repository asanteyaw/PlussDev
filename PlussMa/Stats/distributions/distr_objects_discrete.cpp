#include "distr_fit_common.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <span>
#include <string>

namespace pluss::subsp::stats::distr {
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

std::vector<double> fit_data_f64_(const Series& x, const char* op) {
  return fit_detail::data_f64(x, op);
}

double wald_z_value_(double level, const char* op) {
  return fit_detail::wald_z(level, op);
}

bool is_integerish_(double value) {
  return std::abs(value - std::round(value)) <= 1e-9;
}

double binary_sum_(const std::vector<double>& values, const char* op) {
  return std::accumulate(values.begin(), values.end(), 0.0, [op](double acc, double v) {
    PLUSS_CHECK(is_integerish_(v), std::string(op) + ": x must contain integer values");
    PLUSS_CHECK(v == 0.0 || v == 1.0, std::string(op) + ": x must contain only 0/1 values");
    return acc + v;
  });
}

double bounded_count_sum_(const std::vector<double>& values, std::int64_t count, const char* op) {
  PLUSS_CHECK(count > 0, std::string(op) + ": count must be > 0");
  const double upper = static_cast<double>(count);
  return std::accumulate(values.begin(), values.end(), 0.0, [op, upper](double acc, double v) {
    PLUSS_CHECK(is_integerish_(v), std::string(op) + ": x must contain integer values");
    PLUSS_CHECK(v >= 0.0 && v <= upper, std::string(op) + ": x must lie in [0, count]");
    return acc + v;
  });
}

double geometric_sum_(const std::vector<double>& values, const char* op) {
  return std::accumulate(values.begin(), values.end(), 0.0, [op](double acc, double v) {
    PLUSS_CHECK(is_integerish_(v), std::string(op) + ": x must contain integer values");
    PLUSS_CHECK(v >= 1.0, std::string(op) + ": x must be >= 1");
    return acc + v;
  });
}

double poisson_count_sum_(const std::vector<double>& values, const char* op) {
  return std::accumulate(values.begin(), values.end(), 0.0, [op](double acc, double v) {
    PLUSS_CHECK(v >= 0.0, std::string(op) + ": x must be >= 0");
    PLUSS_CHECK(is_integerish_(v), std::string(op) + ": x must contain integer counts");
    return acc + v;
  });
}

constexpr double kFitPenalty = -1e300;
constexpr double kMinInfo = 1e-12;

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

} // namespace

Series Bernoulli::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::Bernoulli, bernoulli_params(p), sizes, cfg, seed);
}

Series Bernoulli::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Bernoulli::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Bernoulli, bernoulli_params(p), like, seed), like);
}

Series Bernoulli::pdf(const Series& x) const { return detail::pdf_with_params(dst::Bernoulli, x, bernoulli_params(p)); }

Series Bernoulli::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Bernoulli, x, bernoulli_params(p));
}

Series Bernoulli::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Bernoulli, x, bernoulli_params(p));
}

Series Bernoulli::cdf(const Series& x) const { return detail::cdf_with_params(dst::Bernoulli, x, bernoulli_params(p)); }

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

Bernoulli BernoulliFit::model() const {
  return Bernoulli{p};
}

Series BernoulliFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series BernoulliFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "BernoulliFit::score");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "BernoulliFit::score: p must be in (0, 1)");
  const double sum_x = binary_sum_(values, "BernoulliFit::score");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(sum_x / p - (n - sum_x) / (1.0 - p)).to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "BernoulliFit::fisher_inf");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "BernoulliFit::fisher_inf: p must be in (0, 1)");
  static_cast<void>(binary_sum_(values, "BernoulliFit::fisher_inf"));
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(n / (p * (1.0 - p))).to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "BernoulliFit::observed_inf");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "BernoulliFit::observed_inf: p must be in (0, 1)");
  const double sum_x = binary_sum_(values, "BernoulliFit::observed_inf");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(sum_x / (p * p) + (n - sum_x) / ((1.0 - p) * (1.0 - p)))
      .to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::vcov(const Series& x) const {
  const double info = fisher_inf(x).item<double>();
  PLUSS_CHECK(info > 0.0, "BernoulliFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "BernoulliFit::confint");
  const double se = stderr(x).item<double>();
  const std::array<double, 2> ci = {std::clamp(p - z * se, 0.0, 1.0), std::clamp(p + z * se, 0.0, 1.0)};
  return pluss::codem::make_series(std::span<const double>(ci.data(), ci.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

BernoulliFit Bernoulli::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Bernoulli::MLE");
  const double sum_x = binary_sum_(values, "Bernoulli::MLE");
  const double n = static_cast<double>(values.size());
  const double p_hat = sum_x / n;
  PLUSS_CHECK(p_hat > 0.0 && p_hat < 1.0, "Bernoulli::MLE: boundary fit (p=0 or p=1) is unsupported");
  return BernoulliFit{p_hat};
}

Series Binomial::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::Binomial, binomial_params(count, p), sizes, cfg, seed);
}

Series Binomial::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Binomial::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Binomial, binomial_params(count, p), like, seed), like);
}

Series Binomial::pdf(const Series& x) const { return detail::pdf_with_params(dst::Binomial, x, binomial_params(count, p)); }

Series Binomial::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Binomial, x, binomial_params(count, p));
}

Series Binomial::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Binomial, x, binomial_params(count, p));
}

Series Binomial::cdf(const Series& x) const { return detail::cdf_with_params(dst::Binomial, x, binomial_params(count, p)); }

Series Binomial::cgf(const Series& t) const {
  return bernoulli_log_mgf_kernel(t, p).mul(static_cast<double>(count));
}

Series Binomial::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Binomial::cumulant");
  return Bernoulli{p}.cumulant(order).mul(static_cast<double>(count));
}

Binomial BinomialFit::model() const {
  return Binomial{count, p};
}

Series BinomialFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series BinomialFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "BinomialFit::score");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "BinomialFit::score: p must be in (0, 1)");
  const double sum_x = bounded_count_sum_(values, count, "BinomialFit::score");
  const double n_obs = static_cast<double>(values.size());
  const double total = n_obs * static_cast<double>(count);
  return detail::scalar_f64(sum_x / p - (total - sum_x) / (1.0 - p)).to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "BinomialFit::fisher_inf");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "BinomialFit::fisher_inf: p must be in (0, 1)");
  static_cast<void>(bounded_count_sum_(values, count, "BinomialFit::fisher_inf"));
  const double n_obs = static_cast<double>(values.size());
  return detail::scalar_f64(n_obs * static_cast<double>(count) / (p * (1.0 - p)))
      .to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "BinomialFit::observed_inf");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "BinomialFit::observed_inf: p must be in (0, 1)");
  const double sum_x = bounded_count_sum_(values, count, "BinomialFit::observed_inf");
  const double n_obs = static_cast<double>(values.size());
  const double total = n_obs * static_cast<double>(count);
  return detail::scalar_f64(sum_x / (p * p) + (total - sum_x) / ((1.0 - p) * (1.0 - p)))
      .to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::vcov(const Series& x) const {
  const double info = fisher_inf(x).item<double>();
  PLUSS_CHECK(info > 0.0, "BinomialFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "BinomialFit::confint");
  const double se = stderr(x).item<double>();
  const std::array<double, 2> ci = {std::clamp(p - z * se, 0.0, 1.0), std::clamp(p + z * se, 0.0, 1.0)};
  return pluss::codem::make_series(std::span<const double>(ci.data(), ci.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

BinomialFit Binomial::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Binomial::MLE");
  const double sum_x = bounded_count_sum_(values, count, "Binomial::MLE");
  const double n_obs = static_cast<double>(values.size());
  const double p_hat = sum_x / (n_obs * static_cast<double>(count));
  PLUSS_CHECK(p_hat > 0.0 && p_hat < 1.0, "Binomial::MLE: boundary fit (p=0 or p=1) is unsupported");
  return BinomialFit{count, p_hat};
}

Series Poisson::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::Poisson, poisson_params(lambda), sizes, cfg, seed);
}

Series Poisson::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Poisson::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Poisson, poisson_params(lambda), like, seed), like);
}

Series Poisson::pdf(const Series& x) const { return detail::pdf_with_params(dst::Poisson, x, poisson_params(lambda)); }

Series Poisson::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Poisson, x, poisson_params(lambda));
}

Series Poisson::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Poisson, x, poisson_params(lambda));
}

Series Poisson::cdf(const Series& x) const { return detail::cdf_with_params(dst::Poisson, x, poisson_params(lambda)); }

Series Poisson::cgf(const Series& t) const {
  return t.exp().sub(1.0).mul(lambda);
}

Series Poisson::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Poisson::cumulant");
  return detail::scalar_f64(lambda);
}

Poisson PoissonFit::model() const {
  return Poisson{lambda};
}

Series PoissonFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series PoissonFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "PoissonFit::score");
  PLUSS_CHECK(lambda > 0.0, "PoissonFit::score: lambda must be > 0");
  const double sum_x = poisson_count_sum_(values, "PoissonFit::score");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(sum_x / lambda - n).to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "PoissonFit::fisher_inf");
  PLUSS_CHECK(lambda > 0.0, "PoissonFit::fisher_inf: lambda must be > 0");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(n / lambda).to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "PoissonFit::observed_inf");
  PLUSS_CHECK(lambda > 0.0, "PoissonFit::observed_inf: lambda must be > 0");
  const double sum_x = poisson_count_sum_(values, "PoissonFit::observed_inf");
  return detail::scalar_f64(sum_x / (lambda * lambda)).to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::vcov(const Series& x) const {
  const double info = fisher_inf(x).item<double>();
  PLUSS_CHECK(info > 0.0, "PoissonFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "PoissonFit::confint");
  const double se = stderr(x).item<double>();
  const std::array<double, 2> ci = {std::max(0.0, lambda - z * se), lambda + z * se};
  return pluss::codem::make_series(std::span<const double>(ci.data(), ci.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

PoissonFit Poisson::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Poisson::MLE");
  const double sum_x = poisson_count_sum_(values, "Poisson::MLE");
  const double lambda_hat = sum_x / static_cast<double>(values.size());
  PLUSS_CHECK(lambda_hat > 0.0, "Poisson::MLE: fitted lambda is 0; this boundary fit is unsupported");
  return PoissonFit{lambda_hat};
}

Series Geometric::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::Geometric, geometric_params(p), sizes, cfg, seed);
}

Series Geometric::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Geometric::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::Geometric, geometric_params(p), like, seed), like);
}

Series Geometric::pdf(const Series& x) const { return detail::pdf_with_params(dst::Geometric, x, geometric_params(p)); }

Series Geometric::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::Geometric, x, geometric_params(p));
}

Series Geometric::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::Geometric, x, geometric_params(p));
}

Series Geometric::cdf(const Series& x) const { return detail::cdf_with_params(dst::Geometric, x, geometric_params(p)); }

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

Geometric GeometricFit::model() const {
  return Geometric{p};
}

Series GeometricFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series GeometricFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "GeometricFit::score");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "GeometricFit::score: p must be in (0, 1)");
  const double sum_x = geometric_sum_(values, "GeometricFit::score");
  const double n_obs = static_cast<double>(values.size());
  const double sum_k = sum_x - n_obs;
  return detail::scalar_f64(n_obs / p - sum_k / (1.0 - p)).to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "GeometricFit::fisher_inf");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "GeometricFit::fisher_inf: p must be in (0, 1)");
  static_cast<void>(geometric_sum_(values, "GeometricFit::fisher_inf"));
  const double n_obs = static_cast<double>(values.size());
  return detail::scalar_f64(n_obs / (p * p * (1.0 - p))).to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "GeometricFit::observed_inf");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "GeometricFit::observed_inf: p must be in (0, 1)");
  const double sum_x = geometric_sum_(values, "GeometricFit::observed_inf");
  const double n_obs = static_cast<double>(values.size());
  const double sum_k = sum_x - n_obs;
  return detail::scalar_f64(n_obs / (p * p) + sum_k / ((1.0 - p) * (1.0 - p)))
      .to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::vcov(const Series& x) const {
  const double info = fisher_inf(x).item<double>();
  PLUSS_CHECK(info > 0.0, "GeometricFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "GeometricFit::confint");
  const double se = stderr(x).item<double>();
  const std::array<double, 2> ci = {std::clamp(p - z * se, 0.0, 1.0), std::clamp(p + z * se, 0.0, 1.0)};
  return pluss::codem::make_series(std::span<const double>(ci.data(), ci.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

GeometricFit Geometric::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Geometric::MLE");
  const double sum_x = geometric_sum_(values, "Geometric::MLE");
  const double n_obs = static_cast<double>(values.size());
  const double p_hat = n_obs / sum_x;
  PLUSS_CHECK(p_hat > 0.0 && p_hat < 1.0, "Geometric::MLE: boundary fit (p=1) is unsupported");
  return GeometricFit{p_hat};
}

Series NegativeBinomial::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::NegativeBinomial, negbinom_params(r, p), sizes, cfg, seed);
}

Series NegativeBinomial::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series NegativeBinomial::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::NegativeBinomial, negbinom_params(r, p), like, seed), like);
}

Series NegativeBinomial::pdf(const Series& x) const { return detail::pdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p)); }

Series NegativeBinomial::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p));
}

Series NegativeBinomial::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p));
}

Series NegativeBinomial::cdf(const Series& x) const { return detail::cdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p)); }

Series NegativeBinomial::cgf(const Series& t) const {
  detail::require_all_true(t.lt(-std::log1p(-p)), "NegativeBinomial::cgf requires every t element to satisfy t < -log(1 - p)");
  return detail::scalar_f64(std::log(p)).mul(r).sub(t.exp().mul(1.0 - p).mul(-1.0).add(1.0).log().mul(r));
}

Series NegativeBinomial::cumulant(std::int64_t order) const {
  return detail::scalar_f64(negbinom_cumulant_scalar(r, p, order));
}

NegativeBinomial NegativeBinomialFit::model() const {
  return NegativeBinomial{r, p};
}

Series NegativeBinomialFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series NegativeBinomialFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series NegativeBinomialFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series NegativeBinomialFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "NegativeBinomialFit::score");
  const auto fn = [&values](const std::vector<double>& params) -> double {
    if (params.size() != 2 || !(params[0] > 0.0) || !(params[1] > 0.0 && params[1] < 1.0)) return kFitPenalty;
    const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
    if (!valid) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return NegativeBinomial{params[0], params[1]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad({r, p}, fn), {2}, x);
}

Series NegativeBinomialFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series NegativeBinomialFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "NegativeBinomialFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& params) -> double {
    if (params.size() != 2 || !(params[0] > 0.0) || !(params[1] > 0.0 && params[1] < 1.0)) return kFitPenalty;
    const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
    if (!valid) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return NegativeBinomial{params[0], params[1]}.logpdf(xs).sum().item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_({r, p}, fn), x);
}

Series NegativeBinomialFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "NegativeBinomialFit::vcov");
  const auto fn = [&values](const std::vector<double>& params) -> double {
    if (params.size() != 2 || !(params[0] > 0.0) || !(params[1] > 0.0 && params[1] < 1.0)) return kFitPenalty;
    const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
    if (!valid) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return NegativeBinomial{params[0], params[1]}.logpdf(xs).sum().item<double>();
  };
  return vcov_diag_matrix_series_(info_diag_from_loglik_({r, p}, fn), x, "NegativeBinomialFit::vcov");
}

Series NegativeBinomialFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "NegativeBinomialFit::stderr");
  const auto fn = [&values](const std::vector<double>& params) -> double {
    if (params.size() != 2 || !(params[0] > 0.0) || !(params[1] > 0.0 && params[1] < 1.0)) return kFitPenalty;
    const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
    if (!valid) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return NegativeBinomial{params[0], params[1]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_({r, p}, fn)), {2}, x);
}

Series NegativeBinomialFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "NegativeBinomialFit::confint");
  const auto fn = [&values](const std::vector<double>& params) -> double {
    if (params.size() != 2 || !(params[0] > 0.0) || !(params[1] > 0.0 && params[1] < 1.0)) return kFitPenalty;
    const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
    if (!valid) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return NegativeBinomial{params[0], params[1]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> params = {r, p};
  auto ci = confint_matrix_series_(params, stderr_from_info_diag_(info_diag_from_loglik_(params, fn)), x, level, "NegativeBinomialFit::confint");
  auto flat = ci.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  flat[2] = std::clamp(flat[2], 0.0, 1.0);
  flat[3] = std::clamp(flat[3], 0.0, 1.0);
  return fit_detail::vec_series_cpu(flat, {2, 2}).to(x.platform(), ScalarType::Float64);
}

NegativeBinomialFit NegativeBinomial::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "NegativeBinomial::MLE");
  const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
  PLUSS_CHECK(valid, "NegativeBinomial::MLE: x must contain non-negative integer counts");
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
  const double r_hat = (var > mean + 1e-9) ? std::max(1e-8, mean * mean / (var - mean)) : 1e6;
  const double p_hat = std::clamp(r_hat / (r_hat + std::max(mean, 1e-12)), 1e-8, 1.0 - 1e-8);
  return NegativeBinomialFit{r_hat, p_hat};
}

} // namespace pluss::subsp::stats::distr
