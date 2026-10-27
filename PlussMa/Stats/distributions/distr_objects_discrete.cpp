#include "distr_fit_common.h"
#include "pluss/tils/aux/scalar_cast.h"

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

double series_scalar_param_(const Series& param, const char* op, const char* name) {
  PLUSS_CHECK(param.defined(), std::string(op) + ": missing " + name + " Series parameter");
  PLUSS_CHECK(param.numel() == 1, std::string(op) + ": " + name + " Series parameter must be scalar for sampling");
  return pluss::tils::to_scalar<double>(param.to(ScalarType::Float64));
}

Series bernoulli_log_mgf_kernel(const Series& t, double p) {
  return t.expm1().mul(p).log1p();
}

Series cumulant_from_raw_moments_series_(const std::vector<Series>& raw,
                                         std::int64_t order,
                                         std::string_view op) {
  detail::require_positive_order(order, op);
  PLUSS_CHECK(!raw.empty(), std::string(op) + ": raw moments must be non-empty");
  PLUSS_CHECK(static_cast<std::size_t>(order) < raw.size(),
              std::string(op) + ": raw moments must include moment of requested order");
  std::vector<Series> cumulants(static_cast<std::size_t>(order) + 1, pluss::series::undefined());
  cumulants[0] = raw[0].mul(0.0);
  for (std::int64_t n = 1; n <= order; ++n) {
    Series value = raw[static_cast<std::size_t>(n)];
    for (std::int64_t m = 1; m < n; ++m) {
      const double coeff = detail::binomial_coeff(n - 1, m - 1);
      value = value.sub(cumulants[static_cast<std::size_t>(m)].mul(raw[static_cast<std::size_t>(n - m)]).mul(coeff));
    }
    cumulants[static_cast<std::size_t>(n)] = value;
  }
  return cumulants[static_cast<std::size_t>(order)].to(raw[0].platform(), ScalarType::Float64);
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

Series negbinom_cumulant_series_(const Series& r,
                                 const Series& p,
                                 std::int64_t order,
                                 bool allow_p_one,
                                 std::string_view op) {
  detail::require_positive_order(order, op);
  const Series r64 = r.to(ScalarType::Float64).contiguous();
  const Series p64 = p.to(r64.platform(), ScalarType::Float64).contiguous();
  detail::require_all_true(r64.gt(0.0), std::string(op) + ": r must be > 0");
  if (allow_p_one) {
    detail::require_all_true(p64.gt(0.0).logical_and(p64.le(1.0)),
                             std::string(op) + ": p must lie in (0, 1]");
  } else {
    detail::require_all_true(p64.gt(0.0).logical_and(p64.lt(1.0)),
                             std::string(op) + ": p must lie in (0, 1)");
  }
  const Series q = p64.mul(-1.0).add(1.0);
  if (order == 1) {
    return r64.mul(q).div(p64);
  }
  Series total = p64.mul(0.0);
  for (std::int64_t k = 1; k <= order - 1; ++k) {
    const double coeff = stirling_second_kind(order - 1, k) * factorial_int(k);
    if (coeff == 0.0) {
      continue;
    }
    total = total.add(q.pow(static_cast<double>(k))
                          .div(p64.pow(static_cast<double>(k + 1)))
                          .mul(coeff));
  }
  return r64.mul(total);
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
  if (p_series.defined()) {
    const double p_value = series_scalar_param_(p_series, "Bernoulli::sample", "p_series");
    return detail::sample_with_params(dst::Bernoulli, bernoulli_params(p_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Bernoulli, bernoulli_params(p), sizes, cfg, seed);
}

Series Bernoulli::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Bernoulli::sample_like(const Series& like, std::uint64_t seed) const {
  if (p_series.defined()) {
    const double p_value = series_scalar_param_(p_series, "Bernoulli::sample_like", "p_series");
    return detail::preserve_like(detail::sample_like_with_params(dst::Bernoulli, bernoulli_params(p_value), like, seed), like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Bernoulli, bernoulli_params(p), like, seed), like);
}

Series Bernoulli::pdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Bernoulli, x, pluss::sign_list(p_series));
  }
  return detail::pdf_with_params(dst::Bernoulli, x, bernoulli_params(p));
}

Series Bernoulli::logpdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Bernoulli, x, pluss::sign_list(p_series));
  }
  return detail::logpdf_with_params(dst::Bernoulli, x, bernoulli_params(p));
}

Series Bernoulli::logcdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Bernoulli, x, pluss::sign_list(p_series));
  }
  return detail::logcdf_with_params(dst::Bernoulli, x, bernoulli_params(p));
}

Series Bernoulli::cdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Bernoulli, x, pluss::sign_list(p_series));
  }
  return detail::cdf_with_params(dst::Bernoulli, x, bernoulli_params(p));
}

Series Bernoulli::cgf(const Series& t) const {
  PLUSS_CHECK(!p_series.defined(),
              "Bernoulli::cgf: Series parameter is unsupported here; use scalar-object parameter");
  return bernoulli_log_mgf_kernel(t, p);
}

Series Bernoulli::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Bernoulli::cumulant");
  if (p_series.defined()) {
    const Series ps = p_series.to(ScalarType::Float64).contiguous();
    detail::require_all_true(ps.ge(0.0).logical_and(ps.le(1.0)),
                             "Bernoulli::cumulant: p_series must lie in [0, 1]");
    std::vector<Series> raw(static_cast<std::size_t>(order) + 1, pluss::series::undefined());
    raw[0] = ps.mul(0.0).add(1.0);
    for (std::int64_t n = 1; n <= order; ++n) {
      raw[static_cast<std::size_t>(n)] = ps;
    }
    return cumulant_from_raw_moments_series_(raw, order, "Bernoulli::cumulant");
  }
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
  const double info = pluss::tils::to_scalar<double>(fisher_inf(x));
  PLUSS_CHECK(info > 0.0, "BernoulliFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series BernoulliFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "BernoulliFit::confint");
  const double se = pluss::tils::to_scalar<double>(stderr(x));
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
  if (count_series.defined() || p_series.defined()) {
    const double count_value = series_scalar_param_(count_series, "Binomial::sample", "count_series");
    const double p_value = series_scalar_param_(p_series, "Binomial::sample", "p_series");
    return detail::sample_with_params(dst::Binomial,
                                      binomial_params(static_cast<std::int64_t>(std::llround(count_value)), p_value),
                                      sizes,
                                      cfg,
                                      seed);
  }
  return detail::sample_with_params(dst::Binomial, binomial_params(count, p), sizes, cfg, seed);
}

Series Binomial::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Binomial::sample_like(const Series& like, std::uint64_t seed) const {
  if (count_series.defined() || p_series.defined()) {
    const double count_value = series_scalar_param_(count_series, "Binomial::sample_like", "count_series");
    const double p_value = series_scalar_param_(p_series, "Binomial::sample_like", "p_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Binomial,
                                        binomial_params(static_cast<std::int64_t>(std::llround(count_value)), p_value),
                                        like,
                                        seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Binomial, binomial_params(count, p), like, seed), like);
}

Series Binomial::pdf(const Series& x) const {
  if (count_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Binomial, x, pluss::sign_list(count_series, p_series));
  }
  return detail::pdf_with_params(dst::Binomial, x, binomial_params(count, p));
}

Series Binomial::logpdf(const Series& x) const {
  if (count_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Binomial, x, pluss::sign_list(count_series, p_series));
  }
  return detail::logpdf_with_params(dst::Binomial, x, binomial_params(count, p));
}

Series Binomial::logcdf(const Series& x) const {
  if (count_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Binomial, x, pluss::sign_list(count_series, p_series));
  }
  return detail::logcdf_with_params(dst::Binomial, x, binomial_params(count, p));
}

Series Binomial::cdf(const Series& x) const {
  if (count_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Binomial, x, pluss::sign_list(count_series, p_series));
  }
  return detail::cdf_with_params(dst::Binomial, x, binomial_params(count, p));
}

Series Binomial::cgf(const Series& t) const {
  PLUSS_CHECK(!(count_series.defined() || p_series.defined()),
              "Binomial::cgf: Series parameters are unsupported here; use scalar-object parameters");
  return bernoulli_log_mgf_kernel(t, p).mul(static_cast<double>(count));
}

Series Binomial::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Binomial::cumulant");
  if (count_series.defined() || p_series.defined()) {
    const Series like = p_series.defined() ? p_series : count_series;
    const Series n_series = count_series.defined()
                                ? count_series.to(ScalarType::Float64).contiguous()
                                : like.to(ScalarType::Float64).mul(0.0).add(static_cast<double>(count));
    const Series p_param = p_series.defined()
                               ? p_series.to(ScalarType::Float64).contiguous()
                               : like.to(ScalarType::Float64).mul(0.0).add(p);
    detail::require_all_true(n_series.ge(0.0), "Binomial::cumulant: count_series must be >= 0");
    detail::require_all_true(p_param.ge(0.0).logical_and(p_param.le(1.0)),
                             "Binomial::cumulant: p_series must lie in [0, 1]");
    return Bernoulli{p_param}.cumulant(order).mul(n_series).to(n_series.platform(), ScalarType::Float64);
  }
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
  const double info = pluss::tils::to_scalar<double>(fisher_inf(x));
  PLUSS_CHECK(info > 0.0, "BinomialFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series BinomialFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "BinomialFit::confint");
  const double se = pluss::tils::to_scalar<double>(stderr(x));
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
  if (lambda_series.defined()) {
    const double lambda_value = series_scalar_param_(lambda_series, "Poisson::sample", "lambda_series");
    return detail::sample_with_params(dst::Poisson, poisson_params(lambda_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Poisson, poisson_params(lambda), sizes, cfg, seed);
}

Series Poisson::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Poisson::sample_like(const Series& like, std::uint64_t seed) const {
  if (lambda_series.defined()) {
    const double lambda_value = series_scalar_param_(lambda_series, "Poisson::sample_like", "lambda_series");
    return detail::preserve_like(detail::sample_like_with_params(dst::Poisson, poisson_params(lambda_value), like, seed), like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Poisson, poisson_params(lambda), like, seed), like);
}

Series Poisson::pdf(const Series& x) const {
  if (lambda_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Poisson, x, pluss::sign_list(lambda_series));
  }
  return detail::pdf_with_params(dst::Poisson, x, poisson_params(lambda));
}

Series Poisson::logpdf(const Series& x) const {
  if (lambda_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Poisson, x, pluss::sign_list(lambda_series));
  }
  return detail::logpdf_with_params(dst::Poisson, x, poisson_params(lambda));
}

Series Poisson::logcdf(const Series& x) const {
  if (lambda_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Poisson, x, pluss::sign_list(lambda_series));
  }
  return detail::logcdf_with_params(dst::Poisson, x, poisson_params(lambda));
}

Series Poisson::cdf(const Series& x) const {
  if (lambda_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Poisson, x, pluss::sign_list(lambda_series));
  }
  return detail::cdf_with_params(dst::Poisson, x, poisson_params(lambda));
}

Series Poisson::cgf(const Series& t) const {
  PLUSS_CHECK(!lambda_series.defined(),
              "Poisson::cgf: Series parameter is unsupported here; use scalar-object parameter");
  return t.exp().sub(1.0).mul(lambda);
}

Series Poisson::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Poisson::cumulant");
  if (lambda_series.defined()) {
    const Series lambda_param = lambda_series.to(ScalarType::Float64).contiguous();
    detail::require_all_true(lambda_param.ge(0.0),
                             "Poisson::cumulant: lambda_series must be >= 0");
    return lambda_param;
  }
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
  const double info = pluss::tils::to_scalar<double>(fisher_inf(x));
  PLUSS_CHECK(info > 0.0, "PoissonFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series PoissonFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "PoissonFit::confint");
  const double se = pluss::tils::to_scalar<double>(stderr(x));
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
  if (p_series.defined()) {
    const double p_value = series_scalar_param_(p_series, "Geometric::sample", "p_series");
    return detail::sample_with_params(dst::Geometric, geometric_params(p_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Geometric, geometric_params(p), sizes, cfg, seed);
}

Series Geometric::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Geometric::sample_like(const Series& like, std::uint64_t seed) const {
  if (p_series.defined()) {
    const double p_value = series_scalar_param_(p_series, "Geometric::sample_like", "p_series");
    return detail::preserve_like(detail::sample_like_with_params(dst::Geometric, geometric_params(p_value), like, seed), like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Geometric, geometric_params(p), like, seed), like);
}

Series Geometric::pdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Geometric, x, pluss::sign_list(p_series));
  }
  return detail::pdf_with_params(dst::Geometric, x, geometric_params(p));
}

Series Geometric::logpdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Geometric, x, pluss::sign_list(p_series));
  }
  return detail::logpdf_with_params(dst::Geometric, x, geometric_params(p));
}

Series Geometric::logcdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Geometric, x, pluss::sign_list(p_series));
  }
  return detail::logcdf_with_params(dst::Geometric, x, geometric_params(p));
}

Series Geometric::cdf(const Series& x) const {
  if (p_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Geometric, x, pluss::sign_list(p_series));
  }
  return detail::cdf_with_params(dst::Geometric, x, geometric_params(p));
}

Series Geometric::cgf(const Series& t) const {
  PLUSS_CHECK(!p_series.defined(),
              "Geometric::cgf: Series parameter is unsupported here; use scalar-object parameter");
  detail::require_all_true(t.lt(-std::log1p(-p)), "Geometric::cgf requires every t element to satisfy t < -log(1 - p)");
  return t.add(std::log(p)).sub(t.exp().mul(1.0 - p).mul(-1.0).add(1.0).log());
}

Series Geometric::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Geometric::cumulant");
  if (p_series.defined()) {
    const Series p_param = p_series.to(ScalarType::Float64).contiguous();
    detail::require_all_true(p_param.gt(0.0).logical_and(p_param.le(1.0)),
                             "Geometric::cumulant: p_series must lie in (0, 1]");
    if (order == 1) {
      return p_param.reciprocal();
    }
    const Series one = p_param.mul(0.0).add(1.0);
    return negbinom_cumulant_series_(one, p_param, order, /*allow_p_one=*/true, "Geometric::cumulant");
  }
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
  const double info = pluss::tils::to_scalar<double>(fisher_inf(x));
  PLUSS_CHECK(info > 0.0, "GeometricFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series GeometricFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "GeometricFit::confint");
  const double se = pluss::tils::to_scalar<double>(stderr(x));
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
  if (r_series.defined() || p_series.defined()) {
    const double r_value = series_scalar_param_(r_series, "NegativeBinomial::sample", "r_series");
    const double p_value = series_scalar_param_(p_series, "NegativeBinomial::sample", "p_series");
    return detail::sample_with_params(dst::NegativeBinomial, negbinom_params(r_value, p_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::NegativeBinomial, negbinom_params(r, p), sizes, cfg, seed);
}

Series NegativeBinomial::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series NegativeBinomial::sample_like(const Series& like, std::uint64_t seed) const {
  if (r_series.defined() || p_series.defined()) {
    const double r_value = series_scalar_param_(r_series, "NegativeBinomial::sample_like", "r_series");
    const double p_value = series_scalar_param_(p_series, "NegativeBinomial::sample_like", "p_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::NegativeBinomial, negbinom_params(r_value, p_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::NegativeBinomial, negbinom_params(r, p), like, seed), like);
}

Series NegativeBinomial::pdf(const Series& x) const {
  if (r_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::NegativeBinomial, x, pluss::sign_list(r_series, p_series));
  }
  return detail::pdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p));
}

Series NegativeBinomial::logpdf(const Series& x) const {
  if (r_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::NegativeBinomial, x, pluss::sign_list(r_series, p_series));
  }
  return detail::logpdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p));
}

Series NegativeBinomial::logcdf(const Series& x) const {
  if (r_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::NegativeBinomial, x, pluss::sign_list(r_series, p_series));
  }
  return detail::logcdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p));
}

Series NegativeBinomial::cdf(const Series& x) const {
  if (r_series.defined() || p_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::NegativeBinomial, x, pluss::sign_list(r_series, p_series));
  }
  return detail::cdf_with_params(dst::NegativeBinomial, x, negbinom_params(r, p));
}

Series NegativeBinomial::cgf(const Series& t) const {
  PLUSS_CHECK(!(r_series.defined() || p_series.defined()),
              "NegativeBinomial::cgf: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_all_true(t.lt(-std::log1p(-p)), "NegativeBinomial::cgf requires every t element to satisfy t < -log(1 - p)");
  return detail::scalar_f64(std::log(p)).mul(r).sub(t.exp().mul(1.0 - p).mul(-1.0).add(1.0).log().mul(r));
}

Series NegativeBinomial::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "NegativeBinomial::cumulant");
  if (r_series.defined() || p_series.defined()) {
    const Series like = r_series.defined() ? r_series : p_series;
    const Series r_param = r_series.defined()
                               ? r_series.to(ScalarType::Float64).contiguous()
                               : like.to(ScalarType::Float64).mul(0.0).add(r);
    const Series p_param = p_series.defined()
                               ? p_series.to(ScalarType::Float64).contiguous()
                               : like.to(ScalarType::Float64).mul(0.0).add(p);
    return negbinom_cumulant_series_(r_param, p_param, order, /*allow_p_one=*/false, "NegativeBinomial::cumulant");
  }
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

PLUSS_DISTR_FIT_STATS(NegativeBinomialFit,
                      (std::vector<double>{r, p}),
                      (std::vector<bool>{true, true}),
                      fit_detail::negbin_loglik)


NegativeBinomialFit NegativeBinomial::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "NegativeBinomial::MLE");
  const auto valid = std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0 && is_integerish_(v); });
  PLUSS_CHECK(valid, "NegativeBinomial::MLE: x must contain non-negative integer counts");
  const double n = static_cast<double>(values.size());
  const double mean = std::accumulate(values.begin(), values.end(), 0.0) / n;
  PLUSS_CHECK(mean > 0.0, "NegativeBinomial::MLE: the counts are all zero");
  const double var = std::transform_reduce(values.begin(), values.end(), 0.0, std::plus<>{}, [mean](double v) {
                       const double d = v - mean;
                       return d * d;
                     }) / n;
  // Given r the estimate of p is r / (r + mean); r solves the profile score  sum(digamma(x + r) - digamma(r)) + n log(r / (r + mean)) = 0.
  // Counts that are not over-dispersed (variance <= mean) have no finite maximum: r is then returned at a large cap (the Poisson limit).
  constexpr double kMaxR = 1e8;
  double r = (var > mean + 1e-9) ? mean * mean / (var - mean) : kMaxR;
  if (r < kMaxR) {
    const auto score = [&](double rr) {
      double acc = 0.0;
      for (double v : values) acc += fit_detail::digamma(v + rr) - fit_detail::digamma(rr);
      return acc + n * std::log(rr / (rr + mean));
    };
    // bracket the root, then bisect on log r (the score is positive left of the maximum and negative right of it)
    double lo = std::log(1e-6), hi = std::log(kMaxR);
    if (score(std::exp(hi)) > 0.0) {
      r = kMaxR;
    } else {
      for (int it = 0; it < 200; ++it) {
        const double mid = 0.5 * (lo + hi);
        if (score(std::exp(mid)) > 0.0) lo = mid; else hi = mid;
        if (hi - lo < 1e-14) break;
      }
      r = std::exp(0.5 * (lo + hi));
    }
  }
  const double p_hat = std::clamp(r / (r + mean), 1e-12, 1.0 - 1e-12);
  return NegativeBinomialFit{r, p_hat};
}

} // namespace pluss::subsp::stats::distr
