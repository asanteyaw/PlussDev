#include "distr_fit_common.h"
#include "pluss/subsp/math/integ.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <numbers>
#include <span>
#include <string>
#include <vector>

namespace pluss::subsp::stats::distr {
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

double series_scalar_param_(const Series& param, const char* op, const char* name) {
  PLUSS_CHECK(param.defined(), std::string(op) + ": missing " + name + " Series parameter");
  PLUSS_CHECK(param.numel() == 1, std::string(op) + ": " + name + " Series parameter must be scalar for sampling");
  return param.to(ScalarType::Float64).item<double>();
}

std::int64_t series_scalar_int_param_(const Series& param, const char* op, const char* name) {
  const double value = series_scalar_param_(param, op, name);
  const double rounded = std::round(value);
  PLUSS_CHECK(std::abs(value - rounded) <= 1e-9,
              std::string(op) + ": " + name + " Series parameter must be an integer");
  return static_cast<std::int64_t>(rounded);
}

Series pack_2x2_(const Series& a, const Series& b, const Series& c, const Series& d) {
  const auto platform = a.platform();
  const Series row0 = a.reshape({1}).unite({b.reshape({1})}, 0).reshape({1, 2});
  const Series row1 = c.reshape({1}).unite({d.reshape({1})}, 0).reshape({1, 2});
  return row0.unite({row1}, 0).to(platform, ScalarType::Float64);
}

Series matrix_2x2_inverse_(const Series& m, const char* op) {
  const Series m64 = m.to(m.platform(), ScalarType::Float64);
  PLUSS_CHECK(m64.dim() == 2 && m64.sizes()[0] == 2 && m64.sizes()[1] == 2,
              std::string(op) + ": expected a 2x2 matrix");
  const Series row0 = m64.select(0, 0).reshape({2});
  const Series row1 = m64.select(0, 1).reshape({2});
  const Series a = row0.select(0, 0).reshape({1});
  const Series b = row0.select(0, 1).reshape({1});
  const Series c = row1.select(0, 0).reshape({1});
  const Series d = row1.select(0, 1).reshape({1});
  const Series det = a.mul(d).sub(b.mul(c));
  PLUSS_CHECK(det.item<double>() > 0.0,
              std::string(op) + ": expected positive-definite 2x2 matrix");
  const Series inv_det = det.reciprocal();
  return pack_2x2_(
      d.mul(inv_det),
      b.mul(inv_det).mul(-1.0),
      c.mul(inv_det).mul(-1.0),
      a.mul(inv_det));
}

Series stderr_from_cov_2x2_(const Series& cov, const char* op) {
  const Series cov64 = cov.to(cov.platform(), ScalarType::Float64);
  PLUSS_CHECK(cov64.dim() == 2 && cov64.sizes()[0] == 2 && cov64.sizes()[1] == 2,
              std::string(op) + ": expected a 2x2 covariance matrix");
  const Series row0 = cov64.select(0, 0).reshape({2});
  const Series row1 = cov64.select(0, 1).reshape({2});
  const Series se0 = row0.select(0, 0).reshape({1}).clamp_min(0.0).sqrt();
  const Series se1 = row1.select(0, 1).reshape({1}).clamp_min(0.0).sqrt();
  return se0.unite({se1}, 0).reshape({2}).to(cov64.platform(), ScalarType::Float64);
}

Series confint_from_params_se_2_(const Series& params,
                                 const Series& se,
                                 double level,
                                 const char* op,
                                 bool clamp_second_lower = false) {
  const Series params64 = params.to(params.platform(), ScalarType::Float64).reshape({2});
  const Series se64 = se.to(params.platform(), ScalarType::Float64).reshape({2});
  const double z = fit_detail::wald_z(level, op);
  const Series delta = se64.mul(z);
  Series lower = params64.sub(delta);
  const Series upper = params64.add(delta);
  if (clamp_second_lower) {
    const Series idx = Series::arange(0, 2, 1, ScalarType::Int64)
                           .to(params64.platform(), ScalarType::Int64);
    const Series second = idx.eq(1);
    lower = second.where(lower.clamp_min(0.0), lower);
  }
  return lower.reshape({2, 1}).unite({upper.reshape({2, 1})}, 1).to(params64.platform(), ScalarType::Float64);
}

Series stable_log_expm1_over_x(const Series& x) {
  const Series small = x.abs().lt(1e-5);
  const Series x_safe = small.where(x.mul(0.0).add(1.0), x);
  const Series general = x_safe.expm1().div(x_safe).log();
  const Series series = x.mul(0.5).add(x.mul(x).div(24.0));
  return small.where(series, general);
}

Series stable_log_x_over_sin_x(const Series& x) {
  const Series small = x.abs().lt(1e-4);
  const Series x2 = x.square();
  const Series series = x2.div(6.0).add(x2.square().div(180.0)).add(x2.square().mul(x2).div(2835.0));
  const Series general = x.div(x.sin()).log();
  return small.logical_not().where(general, series);
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
  return ::pluss::subsp::stats::distr::logcdf(dst::Normal, detail::scalar_f64(x), std::initializer_list<double>{}).item<double>();
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

// Composite Simpson's rule (n=2048 fixed nodes by default), used for the
// MGF-via-numeric-integration call sites below. Delegates to
// pluss::subsp::math::integ::quadra_scalar (PlussMa/include/pluss/subsp/
// math/integ.h) rather than reimplementing the rule locally -- this file
// and distr.cpp each had their own copy of the identical formula (only the
// node count differed) before this consolidation.
double integrate_simpson(const std::function<double(double)>& fn, double a, double b, int n = 2048) {
  pluss::subsp::math::integ::QuadraScalarOptions options;
  options.nodes = n;
  return pluss::subsp::math::integ::quadra_scalar(fn, a, b, options).value;
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
      ::pluss::throw_error(std::string(name) + "::cgf series did not converge");
    }
  }
  if (!(sum > 0.0L) || !std::isfinite(static_cast<double>(sum))) {
    ::pluss::throw_error(std::string(name) + "::cgf encountered a non-positive or non-finite mgf");
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

Series cumulant_from_raw_moments_series_(const std::vector<Series>& raw,
                                         std::int64_t order,
                                         std::string_view op) {
  detail::require_positive_order(order, op);
  PLUSS_CHECK(!raw.empty(), std::string(op) + ": raw moments must be non-empty");
  PLUSS_CHECK(static_cast<std::size_t>(order) < raw.size(),
              std::string(op) + ": raw moments must include moment of requested order");
  std::vector<Series> cumulants(static_cast<std::size_t>(order) + 1, Series::undefined());
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

std::vector<double> fit_data_f64_(const Series& x, const char* op) {
  return fit_detail::data_f64(x, op);
}

double wald_z_value_(double level, const char* op) {
  return fit_detail::wald_z(level, op);
}

std::array<double, 2> log_resid_stats_(const std::vector<double>& values, double location, const char* op) {
  return std::accumulate(values.begin(), values.end(), std::array<double, 2>{0.0, 0.0}, [location, op](std::array<double, 2> acc, double v) {
    PLUSS_CHECK(v > 0.0, std::string(op) + ": x must be > 0");
    const double r = std::log(v) - location;
    acc[0] += r;
    acc[1] += r * r;
    return acc;
  });
}

double sumsq_nonnegative_(const std::vector<double>& values, const char* op) {
  return std::accumulate(values.begin(), values.end(), 0.0, [op](double acc, double v) {
    PLUSS_CHECK(v >= 0.0, std::string(op) + ": x must be >= 0");
    return acc + v * v;
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

Normal NormalFit::model() const {
  return Normal{mean, stddev};
}

Series NormalFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series NormalFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series NormalFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series NormalFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "NormalFit::score");
  PLUSS_CHECK(stddev > 0.0, "NormalFit::score: stddev must be > 0");
  const double inv_var = 1.0 / (stddev * stddev);
  const double inv_std = 1.0 / stddev;

  double sum_resid = 0.0;
  double sum_sq = 0.0;
  for (double v : values) {
    const double r = v - mean;
    sum_resid += r;
    sum_sq += r * r;
  }

  const double s_mu = sum_resid * inv_var;
  const double s_std = -static_cast<double>(values.size()) * inv_std + sum_sq * inv_var * inv_std;
  const std::array<double, 2> scores = {s_mu, s_std};
  return pluss::codem::make_series(std::span<const double>(scores.data(), scores.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series NormalFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "NormalFit::fisher_inf");
  PLUSS_CHECK(stddev > 0.0, "NormalFit::fisher_inf: stddev must be > 0");
  const double n = static_cast<double>(values.size());
  const double inv_var = 1.0 / (stddev * stddev);
  const std::array<double, 4> fisher = {n * inv_var, 0.0, 0.0, 2.0 * n * inv_var};
  return pluss::codem::make_series(std::span<const double>(fisher.data(), fisher.size()),
                                   {2, 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series NormalFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "NormalFit::observed_inf");
  PLUSS_CHECK(stddev > 0.0, "NormalFit::observed_inf: stddev must be > 0");
  const double inv_var = 1.0 / (stddev * stddev);
  const double inv_std3 = inv_var / stddev;

  double sum_resid = 0.0;
  double sum_sq = 0.0;
  for (double v : values) {
    const double r = v - mean;
    sum_resid += r;
    sum_sq += r * r;
  }

  const double n = static_cast<double>(values.size());
  const double i11 = n * inv_var;
  const double i12 = 2.0 * sum_resid * inv_std3;
  const double i22 = -n * inv_var + 3.0 * sum_sq * inv_var * inv_var;
  const std::array<double, 4> observed = {i11, i12, i12, i22};
  return pluss::codem::make_series(std::span<const double>(observed.data(), observed.size()),
                                   {2, 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series NormalFit::vcov(const Series& x) const {
  return matrix_2x2_inverse_(fisher_inf(x), "NormalFit::vcov").to(x.platform(), ScalarType::Float64);
}

Series NormalFit::stderr(const Series& x) const {
  return stderr_from_cov_2x2_(vcov(x), "NormalFit::stderr").to(x.platform(), ScalarType::Float64);
}

Series NormalFit::confint(const Series& x, double level) const {
  const Series params = fit_detail::vec_series(std::vector<double>{mean, stddev}, {2}, x);
  return confint_from_params_se_2_(
             params,
             stderr(x),
             level,
             "NormalFit::confint",
             true)
      .to(x.platform(), ScalarType::Float64);
}

NormalFit Normal::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Normal::MLE");
  const double n = static_cast<double>(values.size());
  const double mean_hat = std::accumulate(values.begin(), values.end(), 0.0) / n;
  double sum_sq = 0.0;
  for (double v : values) {
    const double r = v - mean_hat;
    sum_sq += r * r;
  }
  const double std_hat = std::sqrt(sum_sq / n); // MLE uses denominator n
  PLUSS_CHECK(std_hat > 0.0, "Normal::MLE: sample stddev is zero; this degenerate fit is unsupported");
  return NormalFit{mean_hat, std_hat};
}

Series Normal::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (mean_series.defined() || stddev_series.defined()) {
    const double mean_value = series_scalar_param_(mean_series, "Normal::sample", "mean_series");
    const double stddev_value = series_scalar_param_(stddev_series, "Normal::sample", "stddev_series");
    return detail::sample_with_params(dst::Normal, normal_params(mean_value, stddev_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Normal, normal_params(mean, stddev), sizes, cfg, seed);
}

Series Normal::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Normal::sample_like(const Series& like, std::uint64_t seed) const {
  if (mean_series.defined() || stddev_series.defined()) {
    const double mean_value = series_scalar_param_(mean_series, "Normal::sample_like", "mean_series");
    const double stddev_value = series_scalar_param_(stddev_series, "Normal::sample_like", "stddev_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Normal, normal_params(mean_value, stddev_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Normal, normal_params(mean, stddev), like, seed), like);
}

Series Normal::pdf(const Series& x) const {
  if (mean_series.defined() || stddev_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Normal, x, pluss::sign_list(mean_series, stddev_series));
  }
  return detail::pdf_with_params(dst::Normal, x, normal_params(mean, stddev));
}

Series Normal::logpdf(const Series& x) const {
  if (mean_series.defined() || stddev_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Normal, x, pluss::sign_list(mean_series, stddev_series));
  }
  return detail::logpdf_with_params(dst::Normal, x, normal_params(mean, stddev));
}

Series Normal::logcdf(const Series& x) const {
  if (mean_series.defined() || stddev_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Normal, x, pluss::sign_list(mean_series, stddev_series));
  }
  return detail::logcdf_with_params(dst::Normal, x, normal_params(mean, stddev));
}

Series Normal::cdf(const Series& x) const {
  if (mean_series.defined() || stddev_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Normal, x, pluss::sign_list(mean_series, stddev_series));
  }
  return detail::cdf_with_params(dst::Normal, x, normal_params(mean, stddev));
}

Series Normal::cgf(const Series& t) const {
  PLUSS_CHECK(!(mean_series.defined() || stddev_series.defined()),
              "Normal::cgf: Series parameters are unsupported here; use scalar-object parameters");
  return t.mul(mean).add(t.mul(t).mul(0.5 * stddev * stddev));
}

Series Normal::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(mean_series.defined() || stddev_series.defined()),
              "Normal::cumulant: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_positive_order(order, "Normal::cumulant");
  if (order == 1) return detail::scalar_f64(mean);
  if (order == 2) return detail::scalar_f64(stddev * stddev);
  return detail::scalar_f64(0.0);
}

Series Gamma::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    const double shape_value = series_scalar_param_(shape_series, "Gamma::sample", "shape_series");
    const double scale_value = series_scalar_param_(scale_series, "Gamma::sample", "scale_series");
    const double location_value = series_scalar_param_(location_series, "Gamma::sample", "location_series");
    return detail::sample_with_params(dst::Gamma, gamma_params(shape_value, scale_value, location_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Gamma, gamma_params(shape, scale, location), sizes, cfg, seed);
}

Series Gamma::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Gamma::sample_like(const Series& like, std::uint64_t seed) const {
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    const double shape_value = series_scalar_param_(shape_series, "Gamma::sample_like", "shape_series");
    const double scale_value = series_scalar_param_(scale_series, "Gamma::sample_like", "scale_series");
    const double location_value = series_scalar_param_(location_series, "Gamma::sample_like", "location_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Gamma, gamma_params(shape_value, scale_value, location_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Gamma, gamma_params(shape, scale, location), like, seed), like);
}

Series Gamma::pdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Gamma, x, pluss::sign_list(location_series, scale_series, shape_series));
  }
  return detail::pdf_with_params(dst::Gamma, x, gamma_params(shape, scale, location));
}

Series Gamma::logpdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Gamma, x, pluss::sign_list(location_series, scale_series, shape_series));
  }
  return detail::logpdf_with_params(dst::Gamma, x, gamma_params(shape, scale, location));
}

Series Gamma::logcdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Gamma, x, pluss::sign_list(location_series, scale_series, shape_series));
  }
  return detail::logcdf_with_params(dst::Gamma, x, gamma_params(shape, scale, location));
}

Series Gamma::cdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Gamma, x, pluss::sign_list(location_series, scale_series, shape_series));
  }
  return detail::cdf_with_params(dst::Gamma, x, gamma_params(shape, scale, location));
}

Series Gamma::cgf(const Series& t) const {
  PLUSS_CHECK(!(shape_series.defined() || scale_series.defined() || location_series.defined()),
              "Gamma::cgf: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_all_true(t.lt(1.0 / scale), "Gamma::cgf requires every t element to satisfy t < 1 / scale");
  return t.mul(location).sub(t.mul(scale).neg().log1p().mul(shape));
}

Series Gamma::cumulant(std::int64_t order) const {
  detail::require_positive_order(order, "Gamma::cumulant");
  if (shape_series.defined() || scale_series.defined() || location_series.defined()) {
    const Series like = shape_series.defined() ? shape_series : (scale_series.defined() ? scale_series : location_series);
    const Series shape_param = shape_series.defined()
                                   ? shape_series.to(ScalarType::Float64).contiguous()
                                   : like.to(ScalarType::Float64).mul(0.0).add(shape);
    const Series scale_param = scale_series.defined()
                                   ? scale_series.to(ScalarType::Float64).contiguous()
                                   : like.to(ScalarType::Float64).mul(0.0).add(scale);
    const Series location_param = location_series.defined()
                                      ? location_series.to(ScalarType::Float64).contiguous()
                                      : like.to(ScalarType::Float64).mul(0.0).add(location);
    detail::require_all_true(shape_param.gt(0.0), "Gamma::cumulant: shape_series must be > 0");
    detail::require_all_true(scale_param.gt(0.0), "Gamma::cumulant: scale_series must be > 0");
    if (order == 1) {
      return location_param.add(shape_param.mul(scale_param));
    }
    return shape_param.mul(scale_param.pow(static_cast<double>(order)))
        .mul(detail::factorial(order - 1))
        .to(shape_param.platform(), ScalarType::Float64);
  }
  if (order == 1) {
    return detail::scalar_f64(location + shape * scale);
  }
  return detail::scalar_f64(shape * detail::factorial(order - 1) * std::pow(scale, static_cast<double>(order)));
}

Series ChiSquare::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (degrees_of_freedom_series.defined()) {
    const std::int64_t dof_value =
        series_scalar_int_param_(degrees_of_freedom_series, "ChiSquare::sample", "degrees_of_freedom_series");
    return detail::sample_with_params(dst::ChiSquare, chisq_params(dof_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::ChiSquare, chisq_params(degrees_of_freedom), sizes, cfg, seed);
}

Series ChiSquare::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series ChiSquare::sample_like(const Series& like, std::uint64_t seed) const {
  if (degrees_of_freedom_series.defined()) {
    const std::int64_t dof_value =
        series_scalar_int_param_(degrees_of_freedom_series, "ChiSquare::sample_like", "degrees_of_freedom_series");
    return detail::preserve_like(detail::sample_like_with_params(dst::ChiSquare, chisq_params(dof_value), like, seed), like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::ChiSquare, chisq_params(degrees_of_freedom), like, seed), like);
}

Series ChiSquare::pdf(const Series& x) const {
  if (degrees_of_freedom_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::ChiSquare, x, pluss::sign_list(degrees_of_freedom_series));
  }
  return detail::pdf_with_params(dst::ChiSquare, x, chisq_params(degrees_of_freedom));
}

Series ChiSquare::logpdf(const Series& x) const {
  if (degrees_of_freedom_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::ChiSquare, x, pluss::sign_list(degrees_of_freedom_series));
  }
  return detail::logpdf_with_params(dst::ChiSquare, x, chisq_params(degrees_of_freedom));
}

Series ChiSquare::logcdf(const Series& x) const {
  if (degrees_of_freedom_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::ChiSquare, x, pluss::sign_list(degrees_of_freedom_series));
  }
  return detail::logcdf_with_params(dst::ChiSquare, x, chisq_params(degrees_of_freedom));
}

Series ChiSquare::cdf(const Series& x) const {
  if (degrees_of_freedom_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::ChiSquare, x, pluss::sign_list(degrees_of_freedom_series));
  }
  return detail::cdf_with_params(dst::ChiSquare, x, chisq_params(degrees_of_freedom));
}

Series ChiSquare::cgf(const Series& t) const {
  PLUSS_CHECK(!degrees_of_freedom_series.defined(),
              "ChiSquare::cgf: Series parameter is unsupported here; use scalar-object parameter");
  const double half_df = 0.5 * static_cast<double>(degrees_of_freedom);
  detail::require_all_true(t.lt(0.5), "ChiSquare::cgf requires every t element to satisfy t < 1/2");
  return t.mul(-2.0).log1p().mul(-half_df);
}

Series ChiSquare::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!degrees_of_freedom_series.defined(),
              "ChiSquare::cumulant: Series parameter is unsupported here; use scalar-object parameter");
  detail::require_positive_order(order, "ChiSquare::cumulant");
  return detail::scalar_f64(static_cast<double>(degrees_of_freedom) *
                            detail::factorial(order - 1) * std::pow(2.0, static_cast<double>(order - 1)));
}

Series InverseGaussian::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (theta_series.defined() || chi_series.defined()) {
    const double theta_value = series_scalar_param_(theta_series, "InverseGaussian::sample", "theta_series");
    const double chi_value = series_scalar_param_(chi_series, "InverseGaussian::sample", "chi_series");
    return detail::sample_with_params(dst::InverseGaussian, inverse_gaussian_params(theta_value, chi_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::InverseGaussian, inverse_gaussian_params(theta, chi), sizes, cfg, seed);
}

Series InverseGaussian::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series InverseGaussian::sample_like(const Series& like, std::uint64_t seed) const {
  if (theta_series.defined() || chi_series.defined()) {
    const double theta_value = series_scalar_param_(theta_series, "InverseGaussian::sample_like", "theta_series");
    const double chi_value = series_scalar_param_(chi_series, "InverseGaussian::sample_like", "chi_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::InverseGaussian, inverse_gaussian_params(theta_value, chi_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::InverseGaussian, inverse_gaussian_params(theta, chi), like, seed), like);
}

Series InverseGaussian::pdf(const Series& x) const {
  if (theta_series.defined() || chi_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::InverseGaussian, x, pluss::sign_list(theta_series, chi_series));
  }
  return detail::pdf_with_params(dst::InverseGaussian, x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::logpdf(const Series& x) const {
  if (theta_series.defined() || chi_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::InverseGaussian, x, pluss::sign_list(theta_series, chi_series));
  }
  return detail::logpdf_with_params(dst::InverseGaussian, x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::logcdf(const Series& x) const {
  if (theta_series.defined() || chi_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::InverseGaussian, x, pluss::sign_list(theta_series, chi_series));
  }
  return detail::logcdf_with_params(dst::InverseGaussian, x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::cdf(const Series& x) const {
  if (theta_series.defined() || chi_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::InverseGaussian, x, pluss::sign_list(theta_series, chi_series));
  }
  return detail::cdf_with_params(dst::InverseGaussian, x, inverse_gaussian_params(theta, chi));
}

Series InverseGaussian::cgf(const Series& t) const {
  PLUSS_CHECK(!(theta_series.defined() || chi_series.defined()),
              "InverseGaussian::cgf: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_all_true(t.mul(2.0 * theta * theta / chi).lt(1.0),
                           "InverseGaussian::cgf requires every t element to satisfy 2 * theta^2 * t / chi < 1");
  return t.mul(-2.0 * theta * theta / chi).add(1.0).sqrt().mul(-chi / theta).add(chi / theta);
}

Series InverseGaussian::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(theta_series.defined() || chi_series.defined()),
              "InverseGaussian::cumulant: Series parameters are unsupported here; use scalar-object parameters");
  return detail::scalar_f64(inverse_gaussian_cumulant_scalar(theta, chi, order));
}

Series Lognormal::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    const double mean_log_value = series_scalar_param_(mean_log_series, "Lognormal::sample", "mean_log_series");
    const double stddev_log_value = series_scalar_param_(stddev_log_series, "Lognormal::sample", "stddev_log_series");
    return detail::sample_with_params(dst::Lognormal, lognormal_params(mean_log_value, stddev_log_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Lognormal, lognormal_params(mean_log, stddev_log), sizes, cfg, seed);
}

Series Lognormal::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Lognormal::sample_like(const Series& like, std::uint64_t seed) const {
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    const double mean_log_value = series_scalar_param_(mean_log_series, "Lognormal::sample_like", "mean_log_series");
    const double stddev_log_value = series_scalar_param_(stddev_log_series, "Lognormal::sample_like", "stddev_log_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Lognormal, lognormal_params(mean_log_value, stddev_log_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Lognormal, lognormal_params(mean_log, stddev_log), like, seed), like);
}

Series Lognormal::pdf(const Series& x) const {
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Lognormal, x, pluss::sign_list(mean_log_series, stddev_log_series));
  }
  return detail::pdf_with_params(dst::Lognormal, x, lognormal_params(mean_log, stddev_log));
}

Series Lognormal::logpdf(const Series& x) const {
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Lognormal, x, pluss::sign_list(mean_log_series, stddev_log_series));
  }
  return detail::logpdf_with_params(dst::Lognormal, x, lognormal_params(mean_log, stddev_log));
}

Series Lognormal::logcdf(const Series& x) const {
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Lognormal, x, pluss::sign_list(mean_log_series, stddev_log_series));
  }
  return detail::logcdf_with_params(dst::Lognormal, x, lognormal_params(mean_log, stddev_log));
}

Series Lognormal::cdf(const Series& x) const {
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Lognormal, x, pluss::sign_list(mean_log_series, stddev_log_series));
  }
  return detail::cdf_with_params(dst::Lognormal, x, lognormal_params(mean_log, stddev_log));
}

Series Lognormal::cgf(const Series& t) const {
  PLUSS_CHECK(!(mean_log_series.defined() || stddev_log_series.defined()),
              "Lognormal::cgf: Series parameters are unsupported here; use scalar-object parameters");
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
  if (mean_log_series.defined() || stddev_log_series.defined()) {
    const Series like = mean_log_series.defined() ? mean_log_series : stddev_log_series;
    const Series mean_param = mean_log_series.defined()
                                  ? mean_log_series.to(ScalarType::Float64).contiguous()
                                  : like.to(ScalarType::Float64).mul(0.0).add(mean_log);
    const Series std_param = stddev_log_series.defined()
                                 ? stddev_log_series.to(ScalarType::Float64).contiguous()
                                 : like.to(ScalarType::Float64).mul(0.0).add(stddev_log);
    detail::require_all_true(std_param.ge(0.0), "Lognormal::cumulant: stddev_log_series must be >= 0");
    const Series one = mean_param.mul(0.0).add(1.0);
    const Series sigma2 = std_param.square();
    std::vector<Series> raw(static_cast<std::size_t>(order) + 1, Series::undefined());
    raw[0] = one;
    for (std::int64_t n = 1; n <= order; ++n) {
      const double nd = static_cast<double>(n);
      raw[static_cast<std::size_t>(n)] = mean_param.mul(nd).add(sigma2.mul(0.5 * nd * nd)).exp();
    }
    return cumulant_from_raw_moments_series_(raw, order, "Lognormal::cumulant");
  }
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = lognormal_raw_moment(mean_log, stddev_log, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Lognormal LognormalFit::model() const {
  return Lognormal{mean_log, stddev_log};
}

Series LognormalFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series LognormalFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "LognormalFit::score");
  PLUSS_CHECK(stddev_log > 0.0, "LognormalFit::score: stddev_log must be > 0");
  const double inv_var = 1.0 / (stddev_log * stddev_log);
  const double inv_std = 1.0 / stddev_log;
  const auto stats = log_resid_stats_(values, mean_log, "LognormalFit::score");
  const double sum_resid = stats[0];
  const double sum_sq = stats[1];

  const double s_mean = sum_resid * inv_var;
  const double s_std = -static_cast<double>(values.size()) * inv_std + sum_sq * inv_var * inv_std;
  const std::array<double, 2> out = {s_mean, s_std};
  return pluss::codem::make_series(std::span<const double>(out.data(), out.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "LognormalFit::fisher_inf");
  PLUSS_CHECK(stddev_log > 0.0, "LognormalFit::fisher_inf: stddev_log must be > 0");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; }),
              "LognormalFit::fisher_inf: x must be > 0");
  const double n = static_cast<double>(values.size());
  const double inv_var = 1.0 / (stddev_log * stddev_log);
  const std::array<double, 4> fisher = {n * inv_var, 0.0, 0.0, 2.0 * n * inv_var};
  return pluss::codem::make_series(std::span<const double>(fisher.data(), fisher.size()),
                                   {2, 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "LognormalFit::observed_inf");
  PLUSS_CHECK(stddev_log > 0.0, "LognormalFit::observed_inf: stddev_log must be > 0");
  const double inv_var = 1.0 / (stddev_log * stddev_log);
  const double inv_std3 = inv_var / stddev_log;
  const auto stats = log_resid_stats_(values, mean_log, "LognormalFit::observed_inf");
  const double sum_resid = stats[0];
  const double sum_sq = stats[1];

  const double n = static_cast<double>(values.size());
  const double i11 = n * inv_var;
  const double i12 = 2.0 * sum_resid * inv_std3;
  const double i22 = -n * inv_var + 3.0 * sum_sq * inv_var * inv_var;
  const std::array<double, 4> observed = {i11, i12, i12, i22};
  return pluss::codem::make_series(std::span<const double>(observed.data(), observed.size()),
                                   {2, 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::vcov(const Series& x) const {
  return matrix_2x2_inverse_(fisher_inf(x), "LognormalFit::vcov").to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::stderr(const Series& x) const {
  return stderr_from_cov_2x2_(vcov(x), "LognormalFit::stderr").to(x.platform(), ScalarType::Float64);
}

Series LognormalFit::confint(const Series& x, double level) const {
  const Series params = fit_detail::vec_series(
      std::vector<double>{mean_log, stddev_log},
      {2},
      x);
  return confint_from_params_se_2_(
             params,
             stderr(x),
             level,
             "LognormalFit::confint",
             true)
      .to(x.platform(), ScalarType::Float64);
}

LognormalFit Lognormal::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Lognormal::MLE");
  const double n = static_cast<double>(values.size());
  const double mean_hat = std::accumulate(values.begin(), values.end(), 0.0, [](double acc, double v) {
                            PLUSS_CHECK(v > 0.0, "Lognormal::MLE: x must be > 0");
                            return acc + std::log(v);
                          }) /
                          n;
  const double sum_sq = std::transform_reduce(values.begin(),
                                              values.end(),
                                              0.0,
                                              std::plus<>{},
                                              [mean_hat](double v) {
                                                PLUSS_CHECK(v > 0.0, "Lognormal::MLE: x must be > 0");
                                                const double r = std::log(v) - mean_hat;
                                                return r * r;
                                              });
  const double std_hat = std::sqrt(sum_sq / n);
  PLUSS_CHECK(std_hat > 0.0, "Lognormal::MLE: sample log-stddev is zero; this degenerate fit is unsupported");
  return LognormalFit{mean_hat, std_hat};
}

Series Uniform::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (low_series.defined() || high_series.defined()) {
    const double low_value = series_scalar_param_(low_series, "Uniform::sample", "low_series");
    const double high_value = series_scalar_param_(high_series, "Uniform::sample", "high_series");
    return detail::sample_with_params(dst::Uniform, uniform_params(low_value, high_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Uniform, uniform_params(low, high), sizes, cfg, seed);
}

Series Uniform::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Uniform::sample_like(const Series& like, std::uint64_t seed) const {
  if (low_series.defined() || high_series.defined()) {
    const double low_value = series_scalar_param_(low_series, "Uniform::sample_like", "low_series");
    const double high_value = series_scalar_param_(high_series, "Uniform::sample_like", "high_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Uniform, uniform_params(low_value, high_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Uniform, uniform_params(low, high), like, seed), like);
}

Series Uniform::pdf(const Series& x) const {
  if (low_series.defined() || high_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Uniform, x, pluss::sign_list(low_series, high_series));
  }
  return detail::pdf_with_params(dst::Uniform, x, uniform_params(low, high));
}

Series Uniform::logpdf(const Series& x) const {
  if (low_series.defined() || high_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Uniform, x, pluss::sign_list(low_series, high_series));
  }
  return detail::logpdf_with_params(dst::Uniform, x, uniform_params(low, high));
}

Series Uniform::logcdf(const Series& x) const {
  if (low_series.defined() || high_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Uniform, x, pluss::sign_list(low_series, high_series));
  }
  return detail::logcdf_with_params(dst::Uniform, x, uniform_params(low, high));
}

Series Uniform::cdf(const Series& x) const {
  if (low_series.defined() || high_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Uniform, x, pluss::sign_list(low_series, high_series));
  }
  return detail::cdf_with_params(dst::Uniform, x, uniform_params(low, high));
}

Series Uniform::cgf(const Series& t) const {
  PLUSS_CHECK(!(low_series.defined() || high_series.defined()),
              "Uniform::cgf: Series parameters are unsupported here; use scalar-object parameters");
  const double width = high - low;
  const Series scaled = t.mul(width);
  return t.mul(low).add(stable_log_expm1_over_x(scaled));
}

Series Uniform::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(low_series.defined() || high_series.defined()),
              "Uniform::cumulant: Series parameters are unsupported here; use scalar-object parameters");
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

Uniform UniformFit::model() const {
  return Uniform{low, high};
}

Series UniformFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series UniformFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series UniformFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series UniformFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "UniformFit::score");
  PLUSS_CHECK(high > low, "UniformFit::score: high must be > low");
  const bool inside = std::all_of(values.begin(),
                                  values.end(),
                                  [this](double v) { return v >= low && v <= high; });
  PLUSS_CHECK(inside, "UniformFit::score: x must lie in [low, high]");
  const std::array<double, 2> s = {0.0, 0.0};
  return pluss::codem::make_series(std::span<const double>(s.data(), s.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series UniformFit::fisher_inf(const Series& x) const {
  return matrix_2x2_inverse_(vcov(x), "UniformFit::fisher_inf").to(x.platform(), ScalarType::Float64);
}

Series UniformFit::observed_inf(const Series& x) const {
  return fisher_inf(x);
}

Series UniformFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "UniformFit::vcov");
  PLUSS_CHECK(high > low, "UniformFit::vcov: high must be > low");
  const double n = static_cast<double>(values.size());
  const double width = high - low;
  const double denom = (n + 1.0) * (n + 1.0) * (n + 2.0);
  const double var = width * width * n / denom;
  const double cov = width * width / denom;
  const std::array<double, 4> out = {var, cov, cov, var};
  return pluss::codem::make_series(std::span<const double>(out.data(), out.size()),
                                   {2, 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

Series UniformFit::stderr(const Series& x) const {
  return stderr_from_cov_2x2_(vcov(x), "UniformFit::stderr").to(x.platform(), ScalarType::Float64);
}

Series UniformFit::confint(const Series& x, double level) const {
  const Series params = fit_detail::vec_series(std::vector<double>{low, high}, {2}, x);
  return confint_from_params_se_2_(
             params,
             stderr(x),
             level,
             "UniformFit::confint")
      .to(x.platform(), ScalarType::Float64);
}

UniformFit Uniform::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Uniform::MLE");
  const auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
  const double low_hat = *min_it;
  const double high_hat = *max_it;
  PLUSS_CHECK(high_hat > low_hat, "Uniform::MLE: sample range is zero; this degenerate fit is unsupported");
  return UniformFit{low_hat, high_hat};
}

Series Exponential::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (rate_series.defined()) {
    const double rate_value = series_scalar_param_(rate_series, "Exponential::sample", "rate_series");
    return detail::sample_with_params(dst::Exponential, exponential_params(rate_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Exponential, exponential_params(rate), sizes, cfg, seed);
}

Series Exponential::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Exponential::sample_like(const Series& like, std::uint64_t seed) const {
  if (rate_series.defined()) {
    const double rate_value = series_scalar_param_(rate_series, "Exponential::sample_like", "rate_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Exponential, exponential_params(rate_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Exponential, exponential_params(rate), like, seed), like);
}

Series Exponential::pdf(const Series& x) const {
  if (rate_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Exponential, x, pluss::sign_list(rate_series));
  }
  return detail::pdf_with_params(dst::Exponential, x, exponential_params(rate));
}

Series Exponential::logpdf(const Series& x) const {
  if (rate_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Exponential, x, pluss::sign_list(rate_series));
  }
  return detail::logpdf_with_params(dst::Exponential, x, exponential_params(rate));
}

Series Exponential::logcdf(const Series& x) const {
  if (rate_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Exponential, x, pluss::sign_list(rate_series));
  }
  return detail::logcdf_with_params(dst::Exponential, x, exponential_params(rate));
}

Series Exponential::cdf(const Series& x) const {
  if (rate_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Exponential, x, pluss::sign_list(rate_series));
  }
  return detail::cdf_with_params(dst::Exponential, x, exponential_params(rate));
}

Series Exponential::cgf(const Series& t) const {
  PLUSS_CHECK(!rate_series.defined(),
              "Exponential::cgf: Series parameter is unsupported here; use scalar-object parameter");
  detail::require_all_true(t.lt(rate), "Exponential::cgf requires every t element to satisfy t < rate");
  return t.div(-rate).log1p().neg();
}

Series Exponential::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!rate_series.defined(),
              "Exponential::cumulant: Series parameter is unsupported here; use scalar-object parameter");
  detail::require_positive_order(order, "Exponential::cumulant");
  return detail::scalar_f64(detail::factorial(order - 1) / std::pow(rate, static_cast<double>(order)));
}

Exponential ExponentialFit::model() const {
  return Exponential{rate};
}

Series ExponentialFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series ExponentialFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series ExponentialFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series ExponentialFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "ExponentialFit::score");
  PLUSS_CHECK(rate > 0.0, "ExponentialFit::score: rate must be > 0");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0; }),
              "ExponentialFit::score: x must be >= 0");
  const double n = static_cast<double>(values.size());
  const double sum_x = std::accumulate(values.begin(), values.end(), 0.0);
  return detail::scalar_f64(n / rate - sum_x).to(x.platform(), ScalarType::Float64);
}

Series ExponentialFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "ExponentialFit::fisher_inf");
  PLUSS_CHECK(rate > 0.0, "ExponentialFit::fisher_inf: rate must be > 0");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(n / (rate * rate)).to(x.platform(), ScalarType::Float64);
}

Series ExponentialFit::observed_inf(const Series& x) const {
  return fisher_inf(x);
}

Series ExponentialFit::vcov(const Series& x) const {
  const double info = fisher_inf(x).item<double>();
  PLUSS_CHECK(info > 0.0, "ExponentialFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series ExponentialFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series ExponentialFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "ExponentialFit::confint");
  const double se = stderr(x).item<double>();
  const std::array<double, 2> ci = {std::max(0.0, rate - z * se), rate + z * se};
  return pluss::codem::make_series(std::span<const double>(ci.data(), ci.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

ExponentialFit Exponential::MLE(const Series& x) const {
  const auto values = fit_data_f64_(x, "Exponential::MLE");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0; }),
              "Exponential::MLE: x must be >= 0");
  const double n = static_cast<double>(values.size());
  const double sum_x = std::accumulate(values.begin(), values.end(), 0.0);
  PLUSS_CHECK(sum_x > 0.0, "Exponential::MLE: sum(x) must be > 0");
  return ExponentialFit{n / sum_x};
}

Series Laplace::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (location_series.defined() || scale_series.defined()) {
    const double location_value = series_scalar_param_(location_series, "Laplace::sample", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Laplace::sample", "scale_series");
    return detail::sample_with_params(dst::Laplace, laplace_params(location_value, scale_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Laplace, laplace_params(location, scale), sizes, cfg, seed);
}

Series Laplace::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Laplace::sample_like(const Series& like, std::uint64_t seed) const {
  if (location_series.defined() || scale_series.defined()) {
    const double location_value = series_scalar_param_(location_series, "Laplace::sample_like", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Laplace::sample_like", "scale_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Laplace, laplace_params(location_value, scale_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Laplace, laplace_params(location, scale), like, seed), like);
}

Series Laplace::pdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Laplace, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::pdf_with_params(dst::Laplace, x, laplace_params(location, scale));
}

Series Laplace::logpdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Laplace, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::logpdf_with_params(dst::Laplace, x, laplace_params(location, scale));
}

Series Laplace::logcdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Laplace, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::logcdf_with_params(dst::Laplace, x, laplace_params(location, scale));
}

Series Laplace::cdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Laplace, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::cdf_with_params(dst::Laplace, x, laplace_params(location, scale));
}

Series Laplace::cgf(const Series& t) const {
  PLUSS_CHECK(!(location_series.defined() || scale_series.defined()),
              "Laplace::cgf: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_all_true(t.abs().lt(1.0 / scale), "Laplace::cgf requires every t element to satisfy |t| < 1 / scale");
  return t.mul(location).sub(t.mul(scale).square().neg().log1p());
}

Series Laplace::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(location_series.defined() || scale_series.defined()),
              "Laplace::cumulant: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_positive_order(order, "Laplace::cumulant");
  if (order == 1) return detail::scalar_f64(location);
  if (order % 2 == 1) return detail::scalar_f64(0.0);
  const std::int64_t half_order = order / 2;
  return detail::scalar_f64(detail::factorial(order) * std::pow(scale, static_cast<double>(order)) /
                            static_cast<double>(half_order));
}

Series Logistic::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (location_series.defined() || scale_series.defined()) {
    const double location_value = series_scalar_param_(location_series, "Logistic::sample", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Logistic::sample", "scale_series");
    return detail::sample_with_params(dst::Logistic, logistic_params(location_value, scale_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Logistic, logistic_params(location, scale), sizes, cfg, seed);
}

Series Logistic::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Logistic::sample_like(const Series& like, std::uint64_t seed) const {
  if (location_series.defined() || scale_series.defined()) {
    const double location_value = series_scalar_param_(location_series, "Logistic::sample_like", "location_series");
    const double scale_value = series_scalar_param_(scale_series, "Logistic::sample_like", "scale_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Logistic, logistic_params(location_value, scale_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Logistic, logistic_params(location, scale), like, seed), like);
}

Series Logistic::pdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Logistic, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::pdf_with_params(dst::Logistic, x, logistic_params(location, scale));
}

Series Logistic::logpdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Logistic, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::logpdf_with_params(dst::Logistic, x, logistic_params(location, scale));
}

Series Logistic::logcdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Logistic, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::logcdf_with_params(dst::Logistic, x, logistic_params(location, scale));
}

Series Logistic::cdf(const Series& x) const {
  if (location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Logistic, x, pluss::sign_list(location_series, scale_series));
  }
  return detail::cdf_with_params(dst::Logistic, x, logistic_params(location, scale));
}

Series Logistic::cgf(const Series& t) const {
  PLUSS_CHECK(!(location_series.defined() || scale_series.defined()),
              "Logistic::cgf: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_all_true(t.mul(scale).abs().lt(1.0), "Logistic::cgf requires every t element to satisfy |scale * t| < 1");
  const Series scaled = t.mul(std::numbers::pi_v<double> * scale);
  return t.mul(location).add(stable_log_x_over_sin_x(scaled));
}

Series Logistic::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(location_series.defined() || scale_series.defined()),
              "Logistic::cumulant: Series parameters are unsupported here; use scalar-object parameters");
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
  if (sigma_series.defined()) {
    const double sigma_value = series_scalar_param_(sigma_series, "Rayleigh::sample", "sigma_series");
    return detail::sample_with_params(dst::Rayleigh, rayleigh_params(sigma_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Rayleigh, rayleigh_params(sigma), sizes, cfg, seed);
}

Series Rayleigh::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Rayleigh::sample_like(const Series& like, std::uint64_t seed) const {
  if (sigma_series.defined()) {
    const double sigma_value = series_scalar_param_(sigma_series, "Rayleigh::sample_like", "sigma_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Rayleigh, rayleigh_params(sigma_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Rayleigh, rayleigh_params(sigma), like, seed), like);
}

Series Rayleigh::pdf(const Series& x) const {
  if (sigma_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Rayleigh, x, pluss::sign_list(sigma_series));
  }
  return detail::pdf_with_params(dst::Rayleigh, x, rayleigh_params(sigma));
}

Series Rayleigh::logpdf(const Series& x) const {
  if (sigma_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Rayleigh, x, pluss::sign_list(sigma_series));
  }
  return detail::logpdf_with_params(dst::Rayleigh, x, rayleigh_params(sigma));
}

Series Rayleigh::logcdf(const Series& x) const {
  if (sigma_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Rayleigh, x, pluss::sign_list(sigma_series));
  }
  return detail::logcdf_with_params(dst::Rayleigh, x, rayleigh_params(sigma));
}

Series Rayleigh::cdf(const Series& x) const {
  if (sigma_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Rayleigh, x, pluss::sign_list(sigma_series));
  }
  return detail::cdf_with_params(dst::Rayleigh, x, rayleigh_params(sigma));
}

Series Rayleigh::cgf(const Series& t) const {
  PLUSS_CHECK(!sigma_series.defined(),
              "Rayleigh::cgf: Series parameter is unsupported here; use scalar-object parameter");
  const Series scaled = t.mul(sigma);
  const Series term = scaled.mul(std::sqrt(std::numbers::pi_v<double> / 2.0))
                          .mul(scaled.square().mul(0.5).exp())
                          .mul(scaled.div(std::sqrt(2.0)).erf().add(1.0));
  return term.log1p();
}

Series Rayleigh::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!sigma_series.defined(),
              "Rayleigh::cumulant: Series parameter is unsupported here; use scalar-object parameter");
  detail::require_positive_order(order, "Rayleigh::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = rayleigh_raw_moment(sigma, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Rayleigh RayleighFit::model() const {
  return Rayleigh{sigma};
}

Series RayleighFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series RayleighFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "RayleighFit::score");
  PLUSS_CHECK(sigma > 0.0, "RayleighFit::score: sigma must be > 0");
  const double sum_x2 = sumsq_nonnegative_(values, "RayleighFit::score");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(-2.0 * n / sigma + sum_x2 / (sigma * sigma * sigma))
      .to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::fisher_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "RayleighFit::fisher_inf");
  PLUSS_CHECK(sigma > 0.0, "RayleighFit::fisher_inf: sigma must be > 0");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return v >= 0.0; }),
              "RayleighFit::fisher_inf: x must be >= 0");
  const double n = static_cast<double>(values.size());
  return detail::scalar_f64(4.0 * n / (sigma * sigma)).to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "RayleighFit::observed_inf");
  PLUSS_CHECK(sigma > 0.0, "RayleighFit::observed_inf: sigma must be > 0");
  const double sum_x2 = sumsq_nonnegative_(values, "RayleighFit::observed_inf");
  const double n = static_cast<double>(values.size());
  const double sigma2 = sigma * sigma;
  const double sigma4 = sigma2 * sigma2;
  return detail::scalar_f64(-2.0 * n / sigma2 + 3.0 * sum_x2 / sigma4).to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::vcov(const Series& x) const {
  const double info = fisher_inf(x).item<double>();
  PLUSS_CHECK(info > 0.0, "RayleighFit::vcov: fisher_inf must be > 0");
  return detail::scalar_f64(1.0 / info).to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::stderr(const Series& x) const {
  return vcov(x).sqrt().to(x.platform(), ScalarType::Float64);
}

Series RayleighFit::confint(const Series& x, double level) const {
  const double z = wald_z_value_(level, "RayleighFit::confint");
  const double se = stderr(x).item<double>();
  const std::array<double, 2> ci = {std::max(0.0, sigma - z * se), sigma + z * se};
  return pluss::codem::make_series(std::span<const double>(ci.data(), ci.size()),
                                   {2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(x.platform(), ScalarType::Float64);
}

RayleighFit Rayleigh::MLE(const Series& x) const {
  PLUSS_CHECK(!sigma_series.defined(),
              "Rayleigh::MLE: Series parameter is unsupported; fit APIs return scalar parameter estimates");
  const auto values = fit_data_f64_(x, "Rayleigh::MLE");
  const double sum_x2 = sumsq_nonnegative_(values, "Rayleigh::MLE");
  const double n = static_cast<double>(values.size());
  const double sigma_hat = std::sqrt(sum_x2 / (2.0 * n));
  PLUSS_CHECK(sigma_hat > 0.0, "Rayleigh::MLE: boundary fit (sigma=0) is unsupported");
  return RayleighFit{sigma_hat};
}

Gamma GammaFit::model() const {
  return Gamma{shape, scale, location};
}

Series GammaFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series GammaFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series GammaFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series GammaFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "GammaFit::score");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 3 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_gt_loc = std::all_of(values.begin(), values.end(), [loc = p[2]](double v) { return v > loc; });
    if (!all_gt_loc) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gamma{p[0], p[1], p[2]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {shape, scale, location};
  return fit_detail::vec_series(fit_detail::fd_grad(p, fn), {3}, x);
}

Series GammaFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series GammaFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "GammaFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 3 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_gt_loc = std::all_of(values.begin(), values.end(), [loc = p[2]](double v) { return v > loc; });
    if (!all_gt_loc) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gamma{p[0], p[1], p[2]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {shape, scale, location};
  return info_diag_matrix_series_(info_diag_from_loglik_(p, fn), x);
}

Series GammaFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "GammaFit::vcov");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 3 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_gt_loc = std::all_of(values.begin(), values.end(), [loc = p[2]](double v) { return v > loc; });
    if (!all_gt_loc) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gamma{p[0], p[1], p[2]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {shape, scale, location};
  return vcov_diag_matrix_series_(info_diag_from_loglik_(p, fn), x, "GammaFit::vcov");
}

Series GammaFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "GammaFit::stderr");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 3 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_gt_loc = std::all_of(values.begin(), values.end(), [loc = p[2]](double v) { return v > loc; });
    if (!all_gt_loc) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gamma{p[0], p[1], p[2]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {shape, scale, location};
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_(p, fn)), {3}, x);
}

Series GammaFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "GammaFit::confint");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 3 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_gt_loc = std::all_of(values.begin(), values.end(), [loc = p[2]](double v) { return v > loc; });
    if (!all_gt_loc) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Gamma{p[0], p[1], p[2]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {shape, scale, location};
  return confint_matrix_series_(p, stderr_from_info_diag_(info_diag_from_loglik_(p, fn)), x, level, "GammaFit::confint");
}

GammaFit Gamma::MLE(const Series& x) const {
  PLUSS_CHECK(!(shape_series.defined() || scale_series.defined() || location_series.defined()),
              "Gamma::MLE: Series parameters are unsupported; fit APIs return scalar parameter estimates");
  const auto values = fit_data_f64_(x, "Gamma::MLE");
  std::vector<double> shifted(values.size(), 0.0);
  std::transform(values.begin(), values.end(), shifted.begin(), [loc = location](double v) {
    PLUSS_CHECK(v > loc, "Gamma::MLE: x must satisfy x > location");
    return v - loc;
  });
  const double n = static_cast<double>(shifted.size());
  const double mean = std::accumulate(shifted.begin(), shifted.end(), 0.0) / n;
  const double var = std::transform_reduce(shifted.begin(),
                                           shifted.end(),
                                           0.0,
                                           std::plus<>{},
                                           [mean](double v) {
                                             const double d = v - mean;
                                             return d * d;
                                           }) /
                     n;
  PLUSS_CHECK(mean > 0.0 && var > 0.0, "Gamma::MLE: data imply non-positive mean/variance");
  const double shape_hat = std::max(1e-8, mean * mean / var);
  const double scale_hat = std::max(1e-8, var / mean);
  return GammaFit{shape_hat, scale_hat, location};
}

InverseGaussian InverseGaussianFit::model() const {
  return InverseGaussian{theta, chi};
}

Series InverseGaussianFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series InverseGaussianFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series InverseGaussianFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series InverseGaussianFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "InverseGaussianFit::score");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_pos = std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; });
    if (!all_pos) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return InverseGaussian{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad({theta, chi}, fn), {2}, x);
}

Series InverseGaussianFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series InverseGaussianFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "InverseGaussianFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_pos = std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; });
    if (!all_pos) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return InverseGaussian{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_({theta, chi}, fn), x);
}

Series InverseGaussianFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "InverseGaussianFit::vcov");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_pos = std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; });
    if (!all_pos) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return InverseGaussian{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return vcov_diag_matrix_series_(info_diag_from_loglik_({theta, chi}, fn), x, "InverseGaussianFit::vcov");
}

Series InverseGaussianFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "InverseGaussianFit::stderr");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_pos = std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; });
    if (!all_pos) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return InverseGaussian{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_({theta, chi}, fn)), {2}, x);
}

Series InverseGaussianFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "InverseGaussianFit::confint");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kFitPenalty;
    const auto all_pos = std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; });
    if (!all_pos) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return InverseGaussian{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {theta, chi};
  return confint_matrix_series_(p, stderr_from_info_diag_(info_diag_from_loglik_(p, fn)), x, level, "InverseGaussianFit::confint");
}

InverseGaussianFit InverseGaussian::MLE(const Series& x) const {
  PLUSS_CHECK(!(theta_series.defined() || chi_series.defined()),
              "InverseGaussian::MLE: Series parameters are unsupported; fit APIs return scalar parameter estimates");
  const auto values = fit_data_f64_(x, "InverseGaussian::MLE");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return v > 0.0; }),
              "InverseGaussian::MLE: x must be > 0");
  const double n = static_cast<double>(values.size());
  const double theta_hat = std::accumulate(values.begin(), values.end(), 0.0) / n;
  const double denom = std::accumulate(values.begin(), values.end(), 0.0, [theta_hat](double acc, double v) {
    const double d = v - theta_hat;
    return acc + (d * d) / (theta_hat * theta_hat * v);
  });
  PLUSS_CHECK(denom > 0.0, "InverseGaussian::MLE: denominator must be > 0");
  const double chi_hat = n / denom;
  PLUSS_CHECK(theta_hat > 0.0 && chi_hat > 0.0, "InverseGaussian::MLE: fitted params must be > 0");
  return InverseGaussianFit{theta_hat, chi_hat};
}

Laplace LaplaceFit::model() const {
  return Laplace{location, scale};
}

Series LaplaceFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series LaplaceFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series LaplaceFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series LaplaceFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "LaplaceFit::score");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[1] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Laplace{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad({location, scale}, fn), {2}, x);
}

Series LaplaceFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series LaplaceFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "LaplaceFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[1] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Laplace{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_({location, scale}, fn), x);
}

Series LaplaceFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "LaplaceFit::vcov");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[1] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Laplace{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return vcov_diag_matrix_series_(info_diag_from_loglik_({location, scale}, fn), x, "LaplaceFit::vcov");
}

Series LaplaceFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "LaplaceFit::stderr");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[1] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Laplace{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_({location, scale}, fn)), {2}, x);
}

Series LaplaceFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "LaplaceFit::confint");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[1] > 0.0)) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Laplace{p[0], p[1]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {location, scale};
  const Series ci = confint_matrix_series_(
      p,
      stderr_from_info_diag_(info_diag_from_loglik_(p, fn)),
      x,
      level,
      "LaplaceFit::confint")
                        .to(x.platform(), ScalarType::Float64);
  Series lower = ci.select(1, 0).reshape({2}).to(x.platform(), ScalarType::Float64);
  const Series upper = ci.select(1, 1).reshape({2}).to(x.platform(), ScalarType::Float64);
  const Series idx = Series::arange(0, 2, 1, ScalarType::Int64)
                         .to(x.platform(), ScalarType::Int64);
  const Series second = idx.eq(1);
  lower = second.where(lower.clamp_min(0.0), lower);
  return lower.reshape({2, 1}).unite({upper.reshape({2, 1})}, 1).to(x.platform(), ScalarType::Float64);
}

LaplaceFit Laplace::MLE(const Series& x) const {
  auto values = fit_data_f64_(x, "Laplace::MLE");
  std::vector<double> sorted = values;
  const std::size_t n = sorted.size();
  std::nth_element(sorted.begin(), sorted.begin() + n / 2, sorted.end());
  double location_hat = sorted[n / 2];
  if (n % 2 == 0) {
    const auto left = *std::max_element(sorted.begin(), sorted.begin() + n / 2);
    location_hat = 0.5 * (location_hat + left);
  }
  const double scale_hat = std::accumulate(values.begin(), values.end(), 0.0, [location_hat](double acc, double v) {
                              return acc + std::abs(v - location_hat);
                            }) /
                            static_cast<double>(values.size());
  PLUSS_CHECK(scale_hat > 0.0, "Laplace::MLE: fitted scale must be > 0");
  return LaplaceFit{location_hat, scale_hat};
}

Beta BetaFit::model() const {
  return Beta{low, high, alpha, beta};
}

Series BetaFit::loglik(const Series& x) const {
  return model().logpdf(x).sum().to(x.platform(), ScalarType::Float64);
}

Series BetaFit::loglik_per_obs(const Series& x) const {
  return model().logpdf(x).to(x.platform(), ScalarType::Float64);
}

Series BetaFit::nll(const Series& x) const {
  return loglik(x).neg();
}

Series BetaFit::score(const Series& x) const {
  const auto values = fit_data_f64_(x, "BetaFit::score");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[1] > p[0]) || !(p[2] > 0.0) || !(p[3] > 0.0)) return kFitPenalty;
    const auto in_bounds = std::all_of(values.begin(), values.end(), [lo = p[0], hi = p[1]](double v) { return v > lo && v < hi; });
    if (!in_bounds) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Beta{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(fit_detail::fd_grad({low, high, alpha, beta}, fn), {4}, x);
}

Series BetaFit::fisher_inf(const Series& x) const {
  return observed_inf(x);
}

Series BetaFit::observed_inf(const Series& x) const {
  const auto values = fit_data_f64_(x, "BetaFit::observed_inf");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[1] > p[0]) || !(p[2] > 0.0) || !(p[3] > 0.0)) return kFitPenalty;
    const auto in_bounds = std::all_of(values.begin(), values.end(), [lo = p[0], hi = p[1]](double v) { return v > lo && v < hi; });
    if (!in_bounds) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Beta{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return info_diag_matrix_series_(info_diag_from_loglik_({low, high, alpha, beta}, fn), x);
}

Series BetaFit::vcov(const Series& x) const {
  const auto values = fit_data_f64_(x, "BetaFit::vcov");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[1] > p[0]) || !(p[2] > 0.0) || !(p[3] > 0.0)) return kFitPenalty;
    const auto in_bounds = std::all_of(values.begin(), values.end(), [lo = p[0], hi = p[1]](double v) { return v > lo && v < hi; });
    if (!in_bounds) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Beta{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return vcov_diag_matrix_series_(info_diag_from_loglik_({low, high, alpha, beta}, fn), x, "BetaFit::vcov");
}

Series BetaFit::stderr(const Series& x) const {
  const auto values = fit_data_f64_(x, "BetaFit::stderr");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[1] > p[0]) || !(p[2] > 0.0) || !(p[3] > 0.0)) return kFitPenalty;
    const auto in_bounds = std::all_of(values.begin(), values.end(), [lo = p[0], hi = p[1]](double v) { return v > lo && v < hi; });
    if (!in_bounds) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Beta{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  return fit_detail::vec_series(stderr_from_info_diag_(info_diag_from_loglik_({low, high, alpha, beta}, fn)), {4}, x);
}

Series BetaFit::confint(const Series& x, double level) const {
  const auto values = fit_data_f64_(x, "BetaFit::confint");
  const auto fn = [&values](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[1] > p[0]) || !(p[2] > 0.0) || !(p[3] > 0.0)) return kFitPenalty;
    const auto in_bounds = std::all_of(values.begin(), values.end(), [lo = p[0], hi = p[1]](double v) { return v > lo && v < hi; });
    if (!in_bounds) return kFitPenalty;
    const Series xs = fit_detail::vec_series_cpu(values, {static_cast<std::int64_t>(values.size())});
    return Beta{p[0], p[1], p[2], p[3]}.logpdf(xs).sum().item<double>();
  };
  const std::vector<double> p = {low, high, alpha, beta};
  return confint_matrix_series_(p, stderr_from_info_diag_(info_diag_from_loglik_(p, fn)), x, level, "BetaFit::confint");
}

BetaFit Beta::MLE(const Series& x) const {
  PLUSS_CHECK(!(low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()),
              "Beta::MLE: Series parameters are unsupported; fit APIs return scalar parameter estimates");
  const auto values = fit_data_f64_(x, "Beta::MLE");
  const double width = high - low;
  PLUSS_CHECK(width > 0.0, "Beta::MLE: high must be > low");
  std::vector<double> z(values.size(), 0.0);
  std::transform(values.begin(), values.end(), z.begin(), [lo = low, hi = high](double v) {
    PLUSS_CHECK(v > lo && v < hi, "Beta::MLE: x must satisfy low < x < high");
    return (v - lo) / (hi - lo);
  });
  const double n = static_cast<double>(z.size());
  const double m = std::accumulate(z.begin(), z.end(), 0.0) / n;
  const double v = std::transform_reduce(z.begin(),
                                         z.end(),
                                         0.0,
                                         std::plus<>{},
                                         [m](double zi) {
                                           const double d = zi - m;
                                           return d * d;
                                         }) /
                   n;
  PLUSS_CHECK(v > 0.0, "Beta::MLE: sample variance must be > 0");
  const double common = std::max(1e-6, m * (1.0 - m) / v - 1.0);
  const double alpha_hat = std::max(1e-8, m * common);
  const double beta_hat = std::max(1e-8, (1.0 - m) * common);
  return BetaFit{low, high, alpha_hat, beta_hat};
}

Series NormalTrunc::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return detail::sample_with_params(dst::NormalTrunc, normaltrunc_params(lower, upper, mean, stddev), sizes, cfg, seed);
}

Series NormalTrunc::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series NormalTrunc::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::NormalTrunc, normaltrunc_params(lower, upper, mean, stddev), like, seed), like);
}

Series NormalTrunc::pdf(const Series& x) const { return detail::pdf_with_params(dst::NormalTrunc, x, normaltrunc_params(lower, upper, mean, stddev)); }

Series NormalTrunc::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::NormalTrunc, x, normaltrunc_params(lower, upper, mean, stddev));
}

Series NormalTrunc::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::NormalTrunc, x, normaltrunc_params(lower, upper, mean, stddev));
}

Series NormalTrunc::cdf(const Series& x) const { return detail::cdf_with_params(dst::NormalTrunc, x, normaltrunc_params(lower, upper, mean, stddev)); }

Series NormalTrunc::cgf(const Series& t) const {
  if (!(upper >= lower) || !(stddev > 0.0)) {
    throw std::domain_error("NormalTrunc::cgf requires upper >= lower and stddev > 0");
  }
  const double alpha = (lower - mean) / stddev;
  const double beta = (upper - mean) / stddev;
  const double log_denom = logsubexp_scalar(std_normal_logcdf_scalar(beta), std_normal_logcdf_scalar(alpha));
  const Series t64 = t.to(t.platform(), ScalarType::Float64);
  const std::array<double, 0> normal_params{};
  const Series log_hi = detail::logcdf_with_params(
      dst::Normal,
      t64.mul(-stddev).add(beta),
      normal_params);
  const Series log_lo = detail::logcdf_with_params(
      dst::Normal,
      t64.mul(-stddev).add(alpha),
      normal_params);
  const Series exp_delta = log_lo.sub(log_hi).exp().clamp_max(1.0 - 1e-15);
  const Series log_num = log_hi.add(exp_delta.mul(-1.0).add(1.0).log());
  return t64.mul(mean)
      .add(t64.square().mul(0.5 * stddev * stddev))
      .add(log_num)
      .sub(log_denom)
      .reshape(t.sizes());
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
  if (degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()) {
    const double dof_value =
        series_scalar_param_(degrees_of_freedom_series, "StudentT::sample", "degrees_of_freedom_series");
    const double location_value =
        series_scalar_param_(location_series, "StudentT::sample", "location_series");
    const double scale_value =
        series_scalar_param_(scale_series, "StudentT::sample", "scale_series");
    return detail::sample_with_params(dst::StudentT, student_t_params(dof_value, location_value, scale_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::StudentT, student_t_params(degrees_of_freedom, location, scale), sizes, cfg, seed);
}

Series StudentT::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series StudentT::sample_like(const Series& like, std::uint64_t seed) const {
  if (degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()) {
    const double dof_value =
        series_scalar_param_(degrees_of_freedom_series, "StudentT::sample_like", "degrees_of_freedom_series");
    const double location_value =
        series_scalar_param_(location_series, "StudentT::sample_like", "location_series");
    const double scale_value =
        series_scalar_param_(scale_series, "StudentT::sample_like", "scale_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::StudentT, student_t_params(dof_value, location_value, scale_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::StudentT, student_t_params(degrees_of_freedom, location, scale), like, seed), like);
}

Series StudentT::pdf(const Series& x) const {
  if (degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(
        dst::StudentT, x, pluss::sign_list(degrees_of_freedom_series, location_series, scale_series));
  }
  return detail::pdf_with_params(dst::StudentT, x, student_t_params(degrees_of_freedom, location, scale));
}

Series StudentT::logpdf(const Series& x) const {
  if (degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(
        dst::StudentT, x, pluss::sign_list(degrees_of_freedom_series, location_series, scale_series));
  }
  return detail::logpdf_with_params(dst::StudentT, x, student_t_params(degrees_of_freedom, location, scale));
}

Series StudentT::logcdf(const Series& x) const {
  if (degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(
        dst::StudentT, x, pluss::sign_list(degrees_of_freedom_series, location_series, scale_series));
  }
  return detail::logcdf_with_params(dst::StudentT, x, student_t_params(degrees_of_freedom, location, scale));
}

Series StudentT::cdf(const Series& x) const {
  if (degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(
        dst::StudentT, x, pluss::sign_list(degrees_of_freedom_series, location_series, scale_series));
  }
  return detail::cdf_with_params(dst::StudentT, x, student_t_params(degrees_of_freedom, location, scale));
}

Series StudentT::cgf(const Series& t) const {
  PLUSS_CHECK(!(degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()),
              "StudentT::cgf: Series parameters are unsupported here; use scalar-object parameters");
  return detail::zero_only_cgf(t, "StudentT");
}

Series StudentT::cumulant(std::int64_t order) const {
  PLUSS_CHECK(!(degrees_of_freedom_series.defined() || location_series.defined() || scale_series.defined()),
              "StudentT::cumulant: Series parameters are unsupported here; use scalar-object parameters");
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
  return detail::sample_with_params(dst::F, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom), sizes, cfg, seed);
}

Series FDistribution::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series FDistribution::sample_like(const Series& like, std::uint64_t seed) const {
  return detail::preserve_like(detail::sample_like_with_params(dst::F, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom), like, seed), like);
}

Series FDistribution::pdf(const Series& x) const {
  return detail::pdf_with_params(dst::F, x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::logpdf(const Series& x) const {
  return detail::logpdf_with_params(dst::F, x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::logcdf(const Series& x) const {
  return detail::logcdf_with_params(dst::F, x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
}

Series FDistribution::cdf(const Series& x) const {
  return detail::cdf_with_params(dst::F, x, f_params(numerator_degrees_of_freedom, denominator_degrees_of_freedom));
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
  if (shape_series.defined() || scale_series.defined()) {
    const double shape_value = series_scalar_param_(shape_series, "Weibull::sample", "shape_series");
    const double scale_value = series_scalar_param_(scale_series, "Weibull::sample", "scale_series");
    return detail::sample_with_params(dst::Weibull, weibull_params(shape_value, scale_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Weibull, weibull_params(shape, scale), sizes, cfg, seed);
}

Series Weibull::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Weibull::sample_like(const Series& like, std::uint64_t seed) const {
  if (shape_series.defined() || scale_series.defined()) {
    const double shape_value = series_scalar_param_(shape_series, "Weibull::sample_like", "shape_series");
    const double scale_value = series_scalar_param_(scale_series, "Weibull::sample_like", "scale_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Weibull, weibull_params(shape_value, scale_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Weibull, weibull_params(shape, scale), like, seed), like);
}

Series Weibull::pdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Weibull, x, pluss::sign_list(shape_series, scale_series));
  }
  return detail::pdf_with_params(dst::Weibull, x, weibull_params(shape, scale));
}

Series Weibull::logpdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Weibull, x, pluss::sign_list(shape_series, scale_series));
  }
  return detail::logpdf_with_params(dst::Weibull, x, weibull_params(shape, scale));
}

Series Weibull::logcdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Weibull, x, pluss::sign_list(shape_series, scale_series));
  }
  return detail::logcdf_with_params(dst::Weibull, x, weibull_params(shape, scale));
}

Series Weibull::cdf(const Series& x) const {
  if (shape_series.defined() || scale_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Weibull, x, pluss::sign_list(shape_series, scale_series));
  }
  return detail::cdf_with_params(dst::Weibull, x, weibull_params(shape, scale));
}

Series Weibull::cgf(const Series& t) const {
  PLUSS_CHECK(!(shape_series.defined() || scale_series.defined()),
              "Weibull::cgf: Series parameters are unsupported here; use scalar-object parameters");
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
  PLUSS_CHECK(!(shape_series.defined() || scale_series.defined()),
              "Weibull::cumulant: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_positive_order(order, "Weibull::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = weibull_raw_moment(shape, scale, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

Series Beta::sample(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  if (low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()) {
    const double low_value = series_scalar_param_(low_series, "Beta::sample", "low_series");
    const double high_value = series_scalar_param_(high_series, "Beta::sample", "high_series");
    const double alpha_value = series_scalar_param_(alpha_series, "Beta::sample", "alpha_series");
    const double beta_value = series_scalar_param_(beta_series, "Beta::sample", "beta_series");
    return detail::sample_with_params(dst::Beta, beta_params(low_value, high_value, alpha_value, beta_value), sizes, cfg, seed);
  }
  return detail::sample_with_params(dst::Beta, beta_params(low, high, alpha, beta), sizes, cfg, seed);
}

Series Beta::sample(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Beta::sample_like(const Series& like, std::uint64_t seed) const {
  if (low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()) {
    const double low_value = series_scalar_param_(low_series, "Beta::sample_like", "low_series");
    const double high_value = series_scalar_param_(high_series, "Beta::sample_like", "high_series");
    const double alpha_value = series_scalar_param_(alpha_series, "Beta::sample_like", "alpha_series");
    const double beta_value = series_scalar_param_(beta_series, "Beta::sample_like", "beta_series");
    return detail::preserve_like(
        detail::sample_like_with_params(dst::Beta, beta_params(low_value, high_value, alpha_value, beta_value), like, seed),
        like);
  }
  return detail::preserve_like(detail::sample_like_with_params(dst::Beta, beta_params(low, high, alpha, beta), like, seed), like);
}

Series Beta::pdf(const Series& x) const {
  if (low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()) {
    return ::pluss::subsp::stats::distr::pdf(dst::Beta, x, pluss::sign_list(low_series, high_series, alpha_series, beta_series));
  }
  return detail::pdf_with_params(dst::Beta, x, beta_params(low, high, alpha, beta));
}

Series Beta::logpdf(const Series& x) const {
  if (low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()) {
    return ::pluss::subsp::stats::distr::logpdf(dst::Beta, x, pluss::sign_list(low_series, high_series, alpha_series, beta_series));
  }
  return detail::logpdf_with_params(dst::Beta, x, beta_params(low, high, alpha, beta));
}

Series Beta::logcdf(const Series& x) const {
  if (low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()) {
    return ::pluss::subsp::stats::distr::logcdf(dst::Beta, x, pluss::sign_list(low_series, high_series, alpha_series, beta_series));
  }
  return detail::logcdf_with_params(dst::Beta, x, beta_params(low, high, alpha, beta));
}

Series Beta::cdf(const Series& x) const {
  if (low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()) {
    return ::pluss::subsp::stats::distr::cdf(dst::Beta, x, pluss::sign_list(low_series, high_series, alpha_series, beta_series));
  }
  return detail::cdf_with_params(dst::Beta, x, beta_params(low, high, alpha, beta));
}

Series Beta::cgf(const Series& t) const {
  PLUSS_CHECK(!(low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()),
              "Beta::cgf: Series parameters are unsupported here; use scalar-object parameters");
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
  PLUSS_CHECK(!(low_series.defined() || high_series.defined() || alpha_series.defined() || beta_series.defined()),
              "Beta::cumulant: Series parameters are unsupported here; use scalar-object parameters");
  detail::require_positive_order(order, "Beta::cumulant");
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] = beta_raw_moment(low, high, alpha, beta, n);
  }
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

} // namespace pluss::subsp::stats::distr
