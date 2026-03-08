#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::subsp::stat::distr;

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> as_f64_vec(const Series& x) {
  if (!x.defined()) {
    throw std::runtime_error("expected defined Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

double nig_cumulant_expected(double alpha, double beta, double location, double scale, std::int64_t order) {
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

double negbinom_cumulant_expected(double r, double p, std::int64_t order) {
  const double q = 1.0 - p;
  if (order == 1) {
    return r * q / p;
  }
  std::vector<long double> row(static_cast<std::size_t>(order), 0.0L);
  auto stirling = [&](std::int64_t n, std::int64_t k) {
    if (n < 0 || k < 0 || k > n) return 0.0L;
    std::vector<long double> s(static_cast<std::size_t>(k) + 1, 0.0L);
    s[0] = 1.0L;
    for (std::int64_t i = 1; i <= n; ++i) {
      const std::int64_t upper = std::min(i, k);
      for (std::int64_t j = upper; j >= 1; --j) {
        s[static_cast<std::size_t>(j)] =
            static_cast<long double>(j) * s[static_cast<std::size_t>(j)] + s[static_cast<std::size_t>(j - 1)];
      }
      s[0] = 0.0L;
    }
    return s[static_cast<std::size_t>(k)];
  };
  auto factorial = [](std::int64_t n) {
    long double out = 1.0L;
    for (std::int64_t i = 2; i <= n; ++i) out *= static_cast<long double>(i);
    return out;
  };
  long double total = 0.0L;
  for (std::int64_t k = 1; k <= order - 1; ++k) {
    total += stirling(order - 1, k) * factorial(k) *
             std::pow(static_cast<long double>(q), static_cast<long double>(k)) /
             std::pow(static_cast<long double>(p), static_cast<long double>(k + 1));
  }
  return static_cast<double>(static_cast<long double>(r) * total);
}

} // namespace

int main() {
  int failures = 0;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  expect(supports("norm"), "supports(norm) should be true", &failures);
  expect(supports("bernoulli"), "supports(bernoulli) should be true", &failures);
  expect(supports("gauss"), "supports(gauss) should be true", &failures);
  expect(supports("antilognormal"), "supports(antilognormal) should be true", &failures);
  expect(supports("doubleexp"), "supports(doubleexp) should be true", &failures);
  expect(supports("furry"), "supports(furry) should be true", &failures);
  expect(supports("chisq"), "supports(chisq) should be true", &failures);
  expect(supports("gig"), "supports(gig) should be true", &failures);
  expect(supports("nig"), "supports(nig) should be true", &failures);
  expect(pluss::distr::supports("alpha"), "pluss::distr alias should expose alpha", &failures);
  expect(pluss::distr::supports("planck"), "pluss::distr alias should expose planck", &failures);
  expect(pluss::distr::supports("vonmises"), "pluss::distr alias should expose vonmises", &failures);

  const Series zero = pluss::make_series(0.0, f64);
  expect(
      approx(as_f64_scalar(pdf("norm", zero, std::initializer_list<double>{})),
             1.0 / std::sqrt(2.0 * std::numbers::pi_v<double>),
             1e-10),
      "normal pdf at 0",
      &failures);
  expect(approx(as_f64_scalar(cdf("normal", zero, std::initializer_list<double>{})), 0.5, 1e-12),
         "normal cdf at 0",
         &failures);
  expect(approx(as_f64_scalar(logcdf("normal", zero, std::initializer_list<double>{})), std::log(0.5), 1e-12),
         "normal logcdf at 0",
         &failures);
  expect(
      approx(as_f64_scalar(zero.pdf()),
             1.0 / std::sqrt(2.0 * std::numbers::pi_v<double>),
             1e-10),
      "Series pdf() standard normal shortcut",
      &failures);
  expect(
      approx(as_f64_scalar(zero.logpdf()),
             std::log(1.0 / std::sqrt(2.0 * std::numbers::pi_v<double>)),
             1e-10),
      "Series logpdf() standard normal shortcut",
      &failures);
  expect(approx(as_f64_scalar(zero.logcdf()), std::log(0.5), 1e-12), "Series logcdf() standard normal shortcut", &failures);
  expect(approx(as_f64_scalar(zero.cdf()), 0.5, 1e-12), "Series cdf() standard normal shortcut", &failures);
  expect(
      approx(as_f64_scalar(zero.pdf(1.5)),
             as_f64_scalar(pdf("normal", zero, {0.0, 1.5})),
             1e-12),
      "Series pdf(stddev) shortcut",
      &failures);
  expect(
      approx(as_f64_scalar(zero.cdf(0.0, 2.0)),
             as_f64_scalar(cdf("normal", zero, {0.0, 2.0})),
             1e-12),
      "Series cdf(mean, stddev) shortcut",
      &failures);
  const Series one = pluss::make_series(1.0, f64);

  const pluss::distr::Normal normal_obj{0.0, 1.0};
  expect(
      approx(as_f64_scalar(normal_obj.pdf(zero)),
             1.0 / std::sqrt(2.0 * std::numbers::pi_v<double>),
             1e-10),
      "Normal object pdf delegates to functional API",
      &failures);
  expect(
      approx(as_f64_scalar(normal_obj.logpdf(zero)),
             std::log(1.0 / std::sqrt(2.0 * std::numbers::pi_v<double>)),
             1e-10),
      "Normal object logpdf delegates to functional API",
      &failures);
  expect(approx(as_f64_scalar(normal_obj.logcdf(zero)), std::log(0.5), 1e-12), "Normal object logcdf delegates to functional API", &failures);
  expect(approx(as_f64_scalar(normal_obj.cdf(zero)), 0.5, 1e-12), "Normal object cdf delegates to functional API", &failures);
  expect(approx(as_f64_scalar(normal_obj.cgf(pluss::make_series(1.0, f64))), 0.5, 1e-12), "Normal object cgf", &failures);
  expect(approx(as_f64_scalar(normal_obj.cumulant(1)), 0.0, 1e-12), "Normal object cumulant(1)", &failures);
  expect(approx(as_f64_scalar(normal_obj.cumulant(2)), 1.0, 1e-12), "Normal object cumulant(2)", &failures);
  expect(approx(as_f64_scalar(normal_obj.cumulant(3)), 0.0, 1e-12), "Normal object cumulant(3)", &failures);

  const pluss::distr::Bernoulli bernoulli_obj{0.25};
  expect(approx(as_f64_scalar(bernoulli_obj.pdf(pluss::make_series(1.0, f64))), 0.25, 1e-12), "Bernoulli object pdf", &failures);
  expect(approx(as_f64_scalar(bernoulli_obj.cgf(zero)), 0.0, 1e-12), "Bernoulli object cgf at 0", &failures);
  expect(approx(as_f64_scalar(bernoulli_obj.cumulant(2)), 0.25 * 0.75, 1e-12), "Bernoulli object cumulant(2)", &failures);

  const pluss::distr::Binomial binomial_obj{4, 0.25};
  expect(approx(as_f64_scalar(binomial_obj.pdf(pluss::make_series(1.0, f64))),
                4.0 * 0.25 * std::pow(0.75, 3.0),
                1e-12),
         "Binomial object pdf",
         &failures);
  expect(approx(as_f64_scalar(binomial_obj.cgf(zero)), 0.0, 1e-12), "Binomial object cgf at 0", &failures);
  expect(approx(as_f64_scalar(binomial_obj.cumulant(2)), 4.0 * 0.25 * 0.75, 1e-12), "Binomial object cumulant(2)", &failures);

  const pluss::distr::Gamma gamma_obj{2.0, 1.0, 0.0};
  expect(approx(as_f64_scalar(gamma_obj.pdf(one)), std::exp(-1.0), 1e-12), "Gamma object pdf", &failures);
  expect(approx(as_f64_scalar(gamma_obj.cgf(pluss::make_series(0.5, f64))), 2.0 * std::log(2.0), 1e-12), "Gamma object cgf", &failures);
  expect(approx(as_f64_scalar(gamma_obj.cumulant(3)), 4.0, 1e-12), "Gamma object cumulant(3)", &failures);

  const pluss::distr::Uniform uniform_obj{0.0, 1.0};
  expect(approx(as_f64_scalar(uniform_obj.pdf(pluss::make_series(0.25, f64))), 1.0, 1e-12), "Uniform object pdf", &failures);
  expect(approx(as_f64_scalar(uniform_obj.cgf(zero)), 0.0, 1e-12), "Uniform object cgf at 0", &failures);
  expect(approx(as_f64_scalar(uniform_obj.cumulant(4)), -1.0 / 120.0, 1e-12), "Uniform object cumulant(4)", &failures);

  const pluss::distr::Exponential exponential_obj{2.0};
  expect(approx(as_f64_scalar(exponential_obj.pdf(one)), 2.0 * std::exp(-2.0), 1e-12), "Exponential object pdf", &failures);
  expect(approx(as_f64_scalar(exponential_obj.cgf(pluss::make_series(1.0, f64))), std::log(2.0), 1e-12), "Exponential object cgf", &failures);
  expect(approx(as_f64_scalar(exponential_obj.cumulant(4)), 0.375, 1e-12), "Exponential object cumulant(4)", &failures);

  const Series normal_fit_x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const auto normal_fit = normal_obj.MLE(normal_fit_x);
  expect(approx(normal_fit.mean, 2.5, 1e-12), "Normal MLE mean", &failures);
  expect(approx(normal_fit.stddev, std::sqrt(1.25), 1e-12), "Normal MLE stddev", &failures);
  const auto normal_score = as_f64_vec(normal_fit.score(normal_fit_x));
  expect(normal_score.size() == 2 && approx(normal_score[0], 0.0, 1e-11) && approx(normal_score[1], 0.0, 1e-11),
         "Normal MLE score at optimum",
         &failures);
  const auto normal_fisher = as_f64_vec(normal_fit.fisher_inf(normal_fit_x));
  expect(normal_fisher.size() == 4 &&
             approx(normal_fisher[0], 3.2, 1e-10) &&
             approx(normal_fisher[1], 0.0, 1e-12) &&
             approx(normal_fisher[2], 0.0, 1e-12) &&
             approx(normal_fisher[3], 6.4, 1e-10),
         "Normal MLE Fisher information",
         &failures);
  const auto normal_observed = as_f64_vec(normal_fit.observed_inf(normal_fit_x));
  expect(normal_observed.size() == 4 &&
             approx(normal_observed[0], 3.2, 1e-10) &&
             approx(normal_observed[1], 0.0, 1e-11) &&
             approx(normal_observed[2], 0.0, 1e-11) &&
             approx(normal_observed[3], 6.4, 1e-10),
         "Normal MLE observed information",
         &failures);
  const auto normal_vcov = as_f64_vec(normal_fit.vcov(normal_fit_x));
  expect(normal_vcov.size() == 4 &&
             approx(normal_vcov[0], 0.3125, 1e-12) &&
             approx(normal_vcov[1], 0.0, 1e-12) &&
             approx(normal_vcov[2], 0.0, 1e-12) &&
             approx(normal_vcov[3], 0.15625, 1e-12),
         "Normal MLE vcov",
         &failures);
  const auto normal_stderr = as_f64_vec(normal_fit.stderr(normal_fit_x));
  expect(normal_stderr.size() == 2 &&
             approx(normal_stderr[0], std::sqrt(0.3125), 1e-12) &&
             approx(normal_stderr[1], std::sqrt(0.15625), 1e-12),
         "Normal MLE stderr",
         &failures);
  expect(as_f64_vec(normal_fit.loglik_per_obs(normal_fit_x)).size() == 4,
         "Normal MLE loglik_per_obs size",
         &failures);
  const auto normal_ci = as_f64_vec(normal_fit.confint(normal_fit_x));
  expect(normal_ci.size() == 4 &&
             normal_ci[0] < normal_fit.mean &&
             normal_ci[1] > normal_fit.mean &&
             normal_ci[2] >= 0.0 &&
             normal_ci[3] > normal_fit.stddev,
         "Normal MLE confint shape/order",
         &failures);
  expect(approx(as_f64_scalar(normal_fit.nll(normal_fit_x)),
                -as_f64_scalar(normal_fit.loglik(normal_fit_x)),
                1e-12),
         "Normal MLE nll/loglik consistency",
         &failures);

  const Series exponential_fit_x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0}, {4}, f64);
  const auto exponential_fit = exponential_obj.MLE(exponential_fit_x);
  expect(approx(exponential_fit.rate, 0.8, 1e-12), "Exponential MLE rate", &failures);
  expect(approx(as_f64_scalar(exponential_fit.score(exponential_fit_x)), 0.0, 1e-11),
         "Exponential MLE score at optimum",
         &failures);
  expect(approx(as_f64_scalar(exponential_fit.fisher_inf(exponential_fit_x)), 6.25, 1e-10),
         "Exponential MLE Fisher information",
         &failures);
  expect(approx(as_f64_scalar(exponential_fit.observed_inf(exponential_fit_x)), 6.25, 1e-10),
         "Exponential MLE observed information",
         &failures);
  expect(approx(as_f64_scalar(exponential_fit.vcov(exponential_fit_x)), 0.16, 1e-12),
         "Exponential MLE vcov",
         &failures);
  expect(approx(as_f64_scalar(exponential_fit.stderr(exponential_fit_x)), 0.4, 1e-12),
         "Exponential MLE stderr",
         &failures);
  expect(as_f64_vec(exponential_fit.loglik_per_obs(exponential_fit_x)).size() == 4,
         "Exponential MLE loglik_per_obs size",
         &failures);
  const auto exp_ci = as_f64_vec(exponential_fit.confint(exponential_fit_x));
  expect(exp_ci.size() == 2 && exp_ci[0] >= 0.0 && exp_ci[0] < exponential_fit.rate && exp_ci[1] > exponential_fit.rate,
         "Exponential MLE confint shape/order",
         &failures);

  const pluss::distr::Poisson poisson_mle_obj{1.0};
  const Series poisson_fit_x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0}, {5}, f64);
  const auto poisson_fit = poisson_mle_obj.MLE(poisson_fit_x);
  expect(approx(poisson_fit.lambda, 2.0, 1e-12), "Poisson MLE lambda", &failures);
  expect(approx(as_f64_scalar(poisson_fit.score(poisson_fit_x)), 0.0, 1e-11),
         "Poisson MLE score at optimum",
         &failures);
  expect(approx(as_f64_scalar(poisson_fit.fisher_inf(poisson_fit_x)), 2.5, 1e-10),
         "Poisson MLE Fisher information",
         &failures);
  expect(approx(as_f64_scalar(poisson_fit.observed_inf(poisson_fit_x)), 2.5, 1e-10),
         "Poisson MLE observed information",
         &failures);
  expect(approx(as_f64_scalar(poisson_fit.vcov(poisson_fit_x)), 0.4, 1e-12),
         "Poisson MLE vcov",
         &failures);
  expect(approx(as_f64_scalar(poisson_fit.stderr(poisson_fit_x)), std::sqrt(0.4), 1e-12),
         "Poisson MLE stderr",
         &failures);
  expect(as_f64_vec(poisson_fit.loglik_per_obs(poisson_fit_x)).size() == 5,
         "Poisson MLE loglik_per_obs size",
         &failures);
  const auto pois_ci = as_f64_vec(poisson_fit.confint(poisson_fit_x));
  expect(pois_ci.size() == 2 && pois_ci[0] >= 0.0 && pois_ci[0] < poisson_fit.lambda && pois_ci[1] > poisson_fit.lambda,
         "Poisson MLE confint shape/order",
         &failures);

  const Series bernoulli_fit_x = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0, 1.0}, {5}, f64);
  const auto bernoulli_fit = bernoulli_obj.MLE(bernoulli_fit_x);
  expect(approx(bernoulli_fit.p, 0.6, 1e-12), "Bernoulli MLE p", &failures);
  expect(approx(as_f64_scalar(bernoulli_fit.score(bernoulli_fit_x)), 0.0, 1e-11),
         "Bernoulli MLE score at optimum",
         &failures);
  expect(approx(as_f64_scalar(bernoulli_fit.fisher_inf(bernoulli_fit_x)), 5.0 / (0.6 * 0.4), 1e-10),
         "Bernoulli MLE fisher information",
         &failures);
  expect(approx(as_f64_scalar(bernoulli_fit.observed_inf(bernoulli_fit_x)), 5.0 / (0.6 * 0.4), 1e-10),
         "Bernoulli MLE observed information",
         &failures);
  expect(approx(as_f64_scalar(bernoulli_fit.vcov(bernoulli_fit_x)), 0.048, 1e-12),
         "Bernoulli MLE vcov",
         &failures);
  expect(as_f64_vec(bernoulli_fit.loglik_per_obs(bernoulli_fit_x)).size() == 5,
         "Bernoulli MLE loglik_per_obs size",
         &failures);

  const Series binomial_fit_x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0}, {5}, f64);
  const auto binomial_fit = binomial_obj.MLE(binomial_fit_x);
  expect(binomial_fit.count == 4, "Binomial MLE count", &failures);
  expect(approx(binomial_fit.p, 0.5, 1e-12), "Binomial MLE p", &failures);
  expect(approx(as_f64_scalar(binomial_fit.score(binomial_fit_x)), 0.0, 1e-11),
         "Binomial MLE score at optimum",
         &failures);
  expect(approx(as_f64_scalar(binomial_fit.fisher_inf(binomial_fit_x)), 80.0, 1e-10),
         "Binomial MLE fisher information",
         &failures);
  expect(approx(as_f64_scalar(binomial_fit.observed_inf(binomial_fit_x)), 80.0, 1e-10),
         "Binomial MLE observed information",
         &failures);
  expect(approx(as_f64_scalar(binomial_fit.vcov(binomial_fit_x)), 0.0125, 1e-12),
         "Binomial MLE vcov",
         &failures);
  expect(as_f64_vec(binomial_fit.loglik_per_obs(binomial_fit_x)).size() == 5,
         "Binomial MLE loglik_per_obs size",
         &failures);

  const Series geometric_fit_x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const auto geometric_fit = pluss::distr::Geometric{0.5}.MLE(geometric_fit_x);
  expect(approx(geometric_fit.p, 0.4, 1e-12), "Geometric MLE p", &failures);
  expect(approx(as_f64_scalar(geometric_fit.score(geometric_fit_x)), 0.0, 1e-11),
         "Geometric MLE score at optimum",
         &failures);
  expect(approx(as_f64_scalar(geometric_fit.fisher_inf(geometric_fit_x)), 4.0 / (0.4 * 0.4 * 0.6), 1e-10),
         "Geometric MLE fisher information",
         &failures);
  expect(approx(as_f64_scalar(geometric_fit.observed_inf(geometric_fit_x)), 4.0 / (0.4 * 0.4 * 0.6), 1e-10),
         "Geometric MLE observed information",
         &failures);
  expect(as_f64_vec(geometric_fit.loglik_per_obs(geometric_fit_x)).size() == 4,
         "Geometric MLE loglik_per_obs size",
         &failures);

  const pluss::distr::Uniform uniform_mle_obj{0.0, 1.0};
  const Series uniform_fit_x = pluss::make_series(std::vector<double>{2.0, 3.0, 5.0, 7.0}, {4}, f64);
  const auto uniform_fit = uniform_mle_obj.MLE(uniform_fit_x);
  expect(approx(uniform_fit.low, 2.0, 1e-12) && approx(uniform_fit.high, 7.0, 1e-12),
         "Uniform MLE low/high",
         &failures);
  const auto uniform_score = as_f64_vec(uniform_fit.score(uniform_fit_x));
  expect(uniform_score.size() == 2 && approx(uniform_score[0], 0.0, 1e-12) && approx(uniform_score[1], 0.0, 1e-12),
         "Uniform MLE score",
         &failures);
  const auto uniform_vcov = as_f64_vec(uniform_fit.vcov(uniform_fit_x));
  expect(uniform_vcov.size() == 4 &&
             approx(uniform_vcov[0], 2.0 / 3.0, 1e-12) &&
             approx(uniform_vcov[1], 1.0 / 6.0, 1e-12) &&
             approx(uniform_vcov[2], 1.0 / 6.0, 1e-12) &&
             approx(uniform_vcov[3], 2.0 / 3.0, 1e-12),
         "Uniform MLE vcov",
         &failures);
  expect(as_f64_vec(uniform_fit.loglik_per_obs(uniform_fit_x)).size() == 4,
         "Uniform MLE loglik_per_obs size",
         &failures);

  const pluss::distr::Lognormal lognormal_mle_obj{0.0, 1.0};
  const Series lognormal_fit_x = pluss::make_series(std::vector<double>{1.0, 2.0, 4.0, 8.0}, {4}, f64);
  const auto lognormal_fit = lognormal_mle_obj.MLE(lognormal_fit_x);
  const std::vector<double> lognormal_vals = {std::log(1.0), std::log(2.0), std::log(4.0), std::log(8.0)};
  const double lognormal_mean =
      std::accumulate(lognormal_vals.begin(), lognormal_vals.end(), 0.0) / static_cast<double>(lognormal_vals.size());
  double lognormal_sum_sq = 0.0;
  for (double lv : lognormal_vals) {
    const double r = lv - lognormal_mean;
    lognormal_sum_sq += r * r;
  }
  const double lognormal_std = std::sqrt(lognormal_sum_sq / static_cast<double>(lognormal_vals.size()));
  expect(approx(lognormal_fit.mean_log, lognormal_mean, 1e-12) &&
             approx(lognormal_fit.stddev_log, lognormal_std, 1e-12),
         "Lognormal MLE mean_log/stddev_log",
         &failures);
  const auto lognormal_score = as_f64_vec(lognormal_fit.score(lognormal_fit_x));
  expect(lognormal_score.size() == 2 && approx(lognormal_score[0], 0.0, 1e-11) && approx(lognormal_score[1], 0.0, 1e-11),
         "Lognormal MLE score at optimum",
         &failures);
  expect(as_f64_vec(lognormal_fit.loglik_per_obs(lognormal_fit_x)).size() == 4,
         "Lognormal MLE loglik_per_obs size",
         &failures);

  const pluss::distr::Rayleigh rayleigh_mle_obj{1.0};
  const Series rayleigh_fit_x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const auto rayleigh_fit = rayleigh_mle_obj.MLE(rayleigh_fit_x);
  expect(approx(rayleigh_fit.sigma, std::sqrt(3.75), 1e-12), "Rayleigh MLE sigma", &failures);
  expect(approx(as_f64_scalar(rayleigh_fit.score(rayleigh_fit_x)), 0.0, 1e-11),
         "Rayleigh MLE score at optimum",
         &failures);
  expect(approx(as_f64_scalar(rayleigh_fit.fisher_inf(rayleigh_fit_x)), 16.0 / 3.75, 1e-10),
         "Rayleigh MLE fisher information",
         &failures);
  expect(approx(as_f64_scalar(rayleigh_fit.vcov(rayleigh_fit_x)), 1.0 / (16.0 / 3.75), 1e-12),
         "Rayleigh MLE vcov",
         &failures);
  expect(as_f64_vec(rayleigh_fit.loglik_per_obs(rayleigh_fit_x)).size() == 4,
         "Rayleigh MLE loglik_per_obs size",
         &failures);
  const pluss::distr::Laplace laplace_obj{0.0, 0.5};
  expect(approx(as_f64_scalar(laplace_obj.pdf(zero)), 1.0, 1e-12), "Laplace object pdf", &failures);
  expect(approx(as_f64_scalar(laplace_obj.cumulant(6)), 3.75, 1e-12), "Laplace object cumulant(6)", &failures);
  const pluss::distr::Logistic logistic_obj{0.0, 0.5};
  expect(approx(as_f64_scalar(logistic_obj.pdf(zero)), 0.5, 1e-12), "Logistic object pdf", &failures);
  expect(approx(as_f64_scalar(logistic_obj.cumulant(6)), std::pow(std::numbers::pi_v<double>, 6.0) / 252.0, 1e-10),
         "Logistic object cumulant(6)",
         &failures);
  const pluss::distr::Rayleigh rayleigh_obj{1.25};
  expect(std::isfinite(as_f64_scalar(rayleigh_obj.logpdf(one))), "Rayleigh object logpdf finite", &failures);
  expect(approx(as_f64_scalar(rayleigh_obj.cumulant(2)),
                ((4.0 - std::numbers::pi_v<double>) / 2.0) * 1.25 * 1.25,
                1e-11),
         "Rayleigh object cumulant(2)",
         &failures);
  const pluss::distr::NormalTrunc normaltrunc_obj{-1.0, 1.0, 0.0, 1.0};
  expect(approx(as_f64_scalar(normaltrunc_obj.cumulant(1)), 0.0, 1e-12), "NormalTrunc object cumulant(1)", &failures);
  const pluss::distr::Poisson poisson_obj{2.5};
  expect(approx(as_f64_scalar(poisson_obj.pdf(pluss::make_series(2.0, f64))),
                std::exp(-2.5) * std::pow(2.5, 2.0) / 2.0,
                1e-12),
         "Poisson object pdf",
         &failures);
  expect(approx(as_f64_scalar(poisson_obj.cumulant(7)), 2.5, 1e-12), "Poisson object cumulant(7)", &failures);
  const pluss::distr::Geometric geometric_obj{0.25};
  expect(approx(as_f64_scalar(geometric_obj.cumulant(1)), 4.0, 1e-12), "Geometric object cumulant(1)", &failures);
  expect(approx(as_f64_scalar(geometric_obj.cumulant(2)), 12.0, 1e-12), "Geometric object cumulant(2)", &failures);
  const pluss::distr::NegativeBinomial negbin_obj{3.0, 0.4};
  expect(approx(as_f64_scalar(negbin_obj.cumulant(5)), negbinom_cumulant_expected(3.0, 0.4, 5), 1e-8),
         "NegativeBinomial object cumulant(5)",
         &failures);
  const pluss::distr::Skellam skellam_obj{2.0, 1.0};
  expect(std::isfinite(as_f64_scalar(skellam_obj.logpdf(zero))), "Skellam object logpdf finite", &failures);
  expect(approx(as_f64_scalar(skellam_obj.cgf(zero)), 0.0, 1e-12), "Skellam object cgf at 0", &failures);
  expect(approx(as_f64_scalar(skellam_obj.cumulant(4)), 3.0, 1e-12), "Skellam object cumulant(4)", &failures);
  expect(skellam_obj.sample({4}, Config{}.with_platform(pluss::pCPU), 20260301ULL).scalar_type() == ScalarType::Int64,
         "Skellam object sample natural dtype",
         &failures);
  const pluss::distr::Nig nig_obj{2.0, 0.5, 0.0, 1.0};
  expect(std::isfinite(as_f64_scalar(nig_obj.logpdf(zero))), "Nig object logpdf finite", &failures);
  expect(approx(as_f64_scalar(nig_obj.cgf(zero)), 0.0, 1e-12), "Nig object cgf at 0", &failures);
  const double nig_gamma = std::sqrt(2.0 * 2.0 - 0.5 * 0.5);
  expect(approx(as_f64_scalar(nig_obj.cumulant(2)), 4.0 / std::pow(nig_gamma, 3.0), 1e-12), "Nig object cumulant(2)", &failures);
  expect(approx(as_f64_scalar(nig_obj.cumulant(5)), nig_cumulant_expected(2.0, 0.5, 0.0, 1.0, 5), 1e-10),
         "Nig object cumulant(5)",
         &failures);

  const Series left_tail = pluss::make_series(-12.0, f64);
  expect(std::isfinite(as_f64_scalar(logcdf("normal", left_tail, std::initializer_list<double>{}))), "normal logcdf left tail finite", &failures);
  expect(std::isfinite(as_f64_scalar(normal_obj.logcdf(left_tail))), "Normal object logcdf left tail finite", &failures);
  expect(std::isfinite(as_f64_scalar(left_tail.logcdf())), "Series logcdf left tail finite", &failures);
  expect(std::isinf(as_f64_scalar(cdf("normal", left_tail, std::initializer_list<double>{}).log())), "normal cdf().log() underflows in left tail", &failures);
  expect(std::isfinite(as_f64_scalar(logcdf("logistic", pluss::make_series(-50.0, f64), {0.0, 1.0}))), "logistic logcdf left tail finite", &failures);
  expect(std::isfinite(as_f64_scalar(logcdf("laplace", pluss::make_series(-50.0, f64), {0.0, 1.0}))), "laplace logcdf left tail finite", &failures);
  expect(std::isfinite(as_f64_scalar(logcdf("rayleigh", pluss::make_series(10.0, f64), {1.0}))), "rayleigh logcdf finite", &failures);
  expect(std::isfinite(as_f64_scalar(logcdf("normaltrunc", pluss::make_series(-0.999, f64), {-1.0, 1.0, 0.0, 1.0}))), "normaltrunc logcdf finite near lower bound", &failures);
  try {
    static_cast<void>(laplace_obj.cgf(pluss::make_series(2.0, f64)));
    expect(false, "Laplace object cgf outside domain should throw", &failures);
  } catch (const std::domain_error&) {
  }
  try {
    static_cast<void>(normaltrunc_obj.cumulant(0));
    expect(false, "NormalTrunc object cumulant(0) should throw", &failures);
  } catch (const std::invalid_argument&) {
  }
  try {
    static_cast<void>(logistic_obj.cgf(pluss::make_series(2.0, f64)));
    expect(false, "Logistic object cgf outside domain should throw", &failures);
  } catch (const std::domain_error&) {
  }

  const Series x_exp = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}, f64);
  const auto exp_pdf = pdf("exp", x_exp, {2.0}).to_vec<double>();
  expect(approx(exp_pdf.at(0), 2.0, 1e-12), "exponential pdf x=0", &failures);
  expect(approx(exp_pdf.at(1), 2.0 * std::exp(-2.0), 1e-12), "exponential pdf x=1", &failures);

  const Series x_bern = pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0, 2.0}, {4}, f64);
  const auto bern_pdf = pdf("bern", x_bern, {0.25}).to_vec<double>();
  expect(approx(bern_pdf.at(0), 0.0), "bernoulli pdf invalid left", &failures);
  expect(approx(bern_pdf.at(1), 0.75), "bernoulli pdf at 0", &failures);
  expect(approx(bern_pdf.at(2), 0.25), "bernoulli pdf at 1", &failures);
  expect(approx(bern_pdf.at(3), 0.0), "bernoulli pdf invalid right", &failures);

  const Series x_geom = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4}, f64);
  const auto geom_cdf = cdf("geom", x_geom, {0.5}).to_vec<double>();
  expect(approx(geom_cdf.at(0), 0.0), "geometric cdf at 0", &failures);
  expect(approx(geom_cdf.at(1), 0.5), "geometric cdf at 1", &failures);
  expect(approx(geom_cdf.at(2), 0.75), "geometric cdf at 2", &failures);
  expect(approx(geom_cdf.at(3), 0.875), "geometric cdf at 3", &failures);

  const Series half = pluss::make_series(0.5, f64);
  expect(approx(as_f64_scalar(pdf("beta", half, {2.0, 2.0})), 1.5, 1e-12), "beta pdf at 0.5", &failures);
  expect(approx(as_f64_scalar(pdf("gamma", one, {2.0})), std::exp(-1.0), 1e-12), "gamma pdf at 1", &failures);
  expect(approx(as_f64_scalar(cdf("t", zero, {5.0})), 0.5, 1e-12), "student t cdf at 0", &failures);

  const Series x_count = pluss::make_series(1.0, f64);
  expect(approx(as_f64_scalar(pdf("binom", x_count, {4.0, 0.25})),
                4.0 * 0.25 * std::pow(0.75, 3.0),
                1e-12),
         "binomial pdf at 1",
         &failures);
  expect(approx(as_f64_scalar(cdf("poisson", x_count, {2.0})),
                std::exp(-2.0) * 3.0,
                1e-12),
         "poisson cdf at 1",
         &failures);
  expect(approx(as_f64_scalar(pdf("chisq", pluss::make_series(2.0, f64), {2.0})),
                0.5 * std::exp(-1.0),
                1e-12),
         "chisq pdf at 2",
         &failures);

  expect(approx(as_f64_scalar(cdf("triangular", half, {0.0, 0.5, 1.0})), 0.5, 1e-12),
         "triangular cdf at mode",
         &failures);
  expect(approx(as_f64_scalar(pdf("pareto", pluss::make_series(2.0, f64), {1.0, 3.0})),
                3.0 / 16.0,
                1e-12),
         "pareto pdf at 2",
         &failures);
  expect(approx(as_f64_scalar(cdf("lomax", one, {2.0, 3.0})),
                19.0 / 27.0,
                1e-12),
         "lomax cdf at 1",
         &failures);
  expect(approx(as_f64_scalar(cdf("normaltrunc", pluss::make_series(-1.0, f64), {-1.0, 1.0, 0.0, 1.0})),
                0.0,
                1e-12),
         "normaltrunc cdf at lower bound",
         &failures);
  expect(approx(as_f64_scalar(pdf("hypergeom", x_count, {10.0, 4.0, 3.0})), 0.5, 1e-12),
         "hypergeom pdf at 1",
         &failures);

  expect(approx(as_f64_scalar(pdf("zeta", one, {2.0})),
                6.0 / (std::numbers::pi_v<double> * std::numbers::pi_v<double>),
                1e-6),
         "zeta pdf at 1",
         &failures);
  expect(std::isfinite(as_f64_scalar(logpdf("skellam", zero, {1.0, 1.0}))), "skellam logpdf finite", &failures);
  expect(std::isfinite(as_f64_scalar(logpdf("gig", one, {1.0, 2.0, 3.0}))), "gig logpdf finite", &failures);
  expect(std::isfinite(as_f64_scalar(logpdf("gh", zero, {1.0, 2.0, 0.5, 0.0, 1.0}))), "gh logpdf finite", &failures);
  expect(std::isfinite(as_f64_scalar(logpdf("nig", zero, {2.0, 0.5, 0.0, 1.0}))), "nig logpdf finite", &failures);
  expect(approx(as_f64_scalar(cdf("u", zero, {})), 0.5, 1e-12), "u cdf at 0", &failures);
  expect(approx(as_f64_scalar(pdf("rademacher", pluss::make_series(1.0, f64), {})), 0.5, 1e-12),
         "rademacher pdf at 1",
         &failures);
  expect(std::isfinite(as_f64_scalar(logpdf("vonmises", zero, {0.0, 2.0}))), "vonmises logpdf finite", &failures);
  expect(approx(as_f64_scalar(pdf("frechet", pluss::make_series(2.0, f64), {0.0, 2.0, 1.0})),
                4.0 * std::exp(-4.0),
                1e-12),
         "frechet alias uses location weibull form",
         &failures);

  const Series normal_draws = sample("normal", {1.5, 2.0}, {4, 3}, f64, 20260301ULL);
  expect(normal_draws.scalar_type() == ScalarType::Float64, "normal sample dtype", &failures);
  expect(normal_draws.sizes().size() == 2 && normal_draws.sizes()[0] == 4 && normal_draws.sizes()[1] == 3,
         "normal sample shape",
         &failures);
  const Series sobol_u = pluss::rng::sobol({4, 3}, f64, 7ULL);
  const Series halton_u = pluss::rng::halton({4, 3}, f64, 7ULL);
  const Series torus_u = pluss::rng::torus({4, 3}, f64, 7ULL);
  expect(sobol_u.numel() == 12 && halton_u.numel() == 12 && torus_u.numel() == 12,
         "pluss::rng low-discrepancy generators preserve requested shape",
         &failures);
  expect(sobol_u.ge(0.0).logical_and(sobol_u.lt(1.0)).all().item<bool>(),
         "sobol values are in [0, 1)",
         &failures);
  expect(halton_u.ge(0.0).logical_and(halton_u.lt(1.0)).all().item<bool>(),
         "halton values are in [0, 1)",
         &failures);
  expect(torus_u.ge(0.0).logical_and(torus_u.lt(1.0)).all().item<bool>(),
         "torus values are in [0, 1)",
         &failures);
  expect(pluss::rng::sobol({3, 64}, f64, 0ULL).numel() == 192, "sobol supports dimension > 16", &failures);
  expect(pluss::rng::sobol({2, 1200}, f64, 0ULL).numel() == 2400, "sobol supports dimension > 1111 via fallback", &failures);

  pluss::rng::SobolOptions sob_opts;
  sob_opts.init = true;
  sob_opts.start = 1;
  const Series sob_a = pluss::rng::sobol({5}, sob_opts, f64);
  sob_opts.init = false;
  const Series sob_b = pluss::rng::sobol({5}, sob_opts, f64);
  const Series sob_full = pluss::rng::sobol({10}, pluss::rng::SobolOptions{}, f64);
  const auto sob_ab = as_f64_vec(sob_a.unite({sob_b}, 0));
  const auto sob_vec = as_f64_vec(sob_full);
  bool sob_concat_ok = sob_ab.size() == sob_vec.size();
  for (std::size_t i = 0; sob_concat_ok && i < sob_vec.size(); ++i) {
    sob_concat_ok = std::abs(sob_ab[i] - sob_vec[i]) < 1e-12;
  }
  expect(sob_concat_ok, "sobol init=false continues the sequence", &failures);

  pluss::rng::SobolOptions sob_scr;
  sob_scr.scrambling = 3;
  sob_scr.seed_set = true;
  sob_scr.seed = 12345ULL;
  const auto sob_scr_vec = as_f64_vec(pluss::rng::sobol({16, 3}, sob_scr, f64));
  const auto sob_plain_vec = as_f64_vec(pluss::rng::sobol({16, 3}, f64, 1ULL));
  bool sob_scr_differs = false;
  for (std::size_t i = 0; i < sob_scr_vec.size(); ++i) {
    if (std::abs(sob_scr_vec[i] - sob_plain_vec[i]) > 1e-15) {
      sob_scr_differs = true;
      break;
    }
  }
  expect(sob_scr_differs, "sobol scrambling option changes generated sequence", &failures);

  pluss::rng::HaltonOptions hal_opts;
  hal_opts.usetime = true;
  const Series hal_time = pluss::rng::halton({4, 3}, hal_opts, f64);
  expect(hal_time.ge(0.0).logical_and(hal_time.lt(1.0)).all().item<bool>(),
         "halton usetime option works",
         &failures);

  pluss::rng::TorusOptions tor_opts;
  tor_opts.prime = {7, 11, 13};
  const Series tor_custom = pluss::rng::torus({6}, tor_opts, f64);
  expect(tor_custom.sizes().size() == 2 && tor_custom.sizes()[0] == 6 && tor_custom.sizes()[1] == 3,
         "torus custom prime vector sets output dimension",
         &failures);

  pluss::rng::TorusOptions tor_mix;
  tor_mix.mixed = true;
  tor_mix.seed_set = true;
  tor_mix.seed = 42ULL;
  const Series tor_mixed = pluss::rng::torus({8, 2}, tor_mix, f64);
  expect(tor_mixed.ge(0.0).logical_and(tor_mixed.lt(1.0)).all().item<bool>(),
         "torus mixed option works",
         &failures);

  const Series normal_halton = sample("normal", {0.0, 1.0}, {8}, f64, 97ULL, pluss::rng::Method::Halton);
  const Series uniform_torus = sample("uniform", {0.0, 1.0}, {8}, f64, 101ULL, pluss::rng::Method::Torus);
  expect(normal_halton.numel() == 8, "sample(..., Method::Halton) works for normal", &failures);
  expect(uniform_torus.ge(0.0).logical_and(uniform_torus.le(1.0)).all().item<bool>(),
         "sample(..., Method::Torus) works for uniform",
         &failures);

  const Series bern_draws = sample("bern", {0.4}, {8}, Config{}.with_platform(pluss::pCPU), 20260301ULL);
  expect(bern_draws.scalar_type() == ScalarType::Bool, "bernoulli sample natural dtype", &failures);
  expect(normal_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Normal object sample shape", &failures);
  expect(bernoulli_obj.sample({4}, Config{}.with_platform(pluss::pCPU), 20260301ULL).scalar_type() == ScalarType::Bool,
         "Bernoulli object sample natural dtype",
         &failures);
  expect(binomial_obj.sample({4}, Config{}.with_platform(pluss::pCPU), 20260301ULL).numel() == 4, "Binomial object sample shape", &failures);
  expect(gamma_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Gamma object sample shape", &failures);
  expect(uniform_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Uniform object sample shape", &failures);
  expect(exponential_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Exponential object sample shape", &failures);
  expect(laplace_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Laplace object sample shape", &failures);
  expect(logistic_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Logistic object sample shape", &failures);
  expect(rayleigh_obj.sample({4}, f64, 20260301ULL).numel() == 4, "Rayleigh object sample shape", &failures);
  expect(poisson_obj.sample({4}, Config{}.with_platform(pluss::pCPU), 20260301ULL).scalar_type() == ScalarType::Int64,
         "Poisson object sample natural dtype",
         &failures);
  expect(geometric_obj.sample({4}, Config{}.with_platform(pluss::pCPU), 20260301ULL).scalar_type() == ScalarType::Int64,
         "Geometric object sample natural dtype",
         &failures);
  expect(negbin_obj.sample({4}, Config{}.with_platform(pluss::pCPU), 20260301ULL).scalar_type() == ScalarType::Int64,
         "NegativeBinomial object sample natural dtype",
         &failures);
  expect(sample("poisson", {2.0}, {6}, Config{}.with_platform(pluss::pCPU), 11ULL).scalar_type() == ScalarType::Int64,
         "poisson sample natural dtype",
         &failures);
  expect(sample("gig", {1.0, 2.0, 3.0}, {5}, f64, 13ULL).numel() == 5, "gig sample shape", &failures);
  expect(sample("nig", {2.0, 0.5, 0.0, 1.0}, {5}, f64, 17ULL).numel() == 5, "nig sample shape", &failures);
  expect(sample("vonmises", {0.0, 2.0}, {5}, f64, 19ULL).numel() == 5, "vonmises sample shape", &failures);
  expect(pluss::distr::sample("arcsin", {5}, f64, 23ULL).numel() == 5, "sample overload without params", &failures);

  const Series like = pluss::zeros(
      {2, 5},
      Config{}
          .with_dtype(ScalarType::Float32)
          .with_platform(pluss::pCPU)
          .with_pinned(true));
  expect(pluss::distr::sample_like("rademacher", like, 29ULL).numel() == like.numel(),
         "sample_like overload without params",
         &failures);
  const Series like_uniform_sobol =
      pluss::distr::sample_like("uniform", {0.0, 1.0}, like, 33ULL, pluss::rng::Method::Sobol);
  expect(like_uniform_sobol.ge(0.0).logical_and(like_uniform_sobol.le(1.0)).all().item<bool>(),
         "sample_like supports low-discrepancy method selection",
         &failures);
  const Series weibull_like = sample_like("weibull", {1.5, 2.0}, like, 7ULL);
  const Series normal_like = normal_obj.sample_like(like, 31ULL);
  const Series gamma_like = gamma_obj.sample_like(like, 37ULL);
  const Series nig_like = nig_obj.sample_like(like, 41ULL);
  const Series laplace_like = laplace_obj.sample_like(like, 43ULL);
  const Series logistic_like = logistic_obj.sample_like(like, 47ULL);
  const Series rayleigh_like = rayleigh_obj.sample_like(like, 53ULL);
  const Series poisson_like = poisson_obj.sample_like(like, 59ULL);
  const Series geometric_like = geometric_obj.sample_like(like, 61ULL);
  const Series negbin_like = negbin_obj.sample_like(like, 67ULL);
  const Series normaltrunc_like = normaltrunc_obj.sample_like(like, 71ULL);
  expect(weibull_like.scalar_type() == ScalarType::Float32, "sample_like preserves dtype", &failures);
  expect(normal_like.scalar_type() == ScalarType::Float32, "Normal object sample_like preserves dtype", &failures);
  expect(gamma_like.scalar_type() == ScalarType::Float32, "Gamma object sample_like preserves dtype", &failures);
  expect(nig_like.scalar_type() == ScalarType::Float32, "Nig object sample_like preserves dtype", &failures);
  expect(laplace_like.scalar_type() == ScalarType::Float32, "Laplace object sample_like preserves dtype", &failures);
  expect(logistic_like.scalar_type() == ScalarType::Float32, "Logistic object sample_like preserves dtype", &failures);
  expect(rayleigh_like.scalar_type() == ScalarType::Float32, "Rayleigh object sample_like preserves dtype", &failures);
  expect(poisson_like.scalar_type() == ScalarType::Float32, "Poisson object sample_like preserves dtype", &failures);
  expect(geometric_like.scalar_type() == ScalarType::Float32, "Geometric object sample_like preserves dtype", &failures);
  expect(negbin_like.scalar_type() == ScalarType::Float32, "NegativeBinomial object sample_like preserves dtype", &failures);
  expect(normaltrunc_like.scalar_type() == ScalarType::Float32, "NormalTrunc object sample_like preserves dtype", &failures);
  expect(weibull_like.platform() == pluss::pCPU, "sample_like preserves platform", &failures);
  expect(normal_like.platform() == pluss::pCPU, "Normal object sample_like preserves platform", &failures);
  expect(gamma_like.platform() == pluss::pCPU, "Gamma object sample_like preserves platform", &failures);
  expect(nig_like.platform() == pluss::pCPU, "Nig object sample_like preserves platform", &failures);
  expect(laplace_like.platform() == pluss::pCPU, "Laplace object sample_like preserves platform", &failures);
  expect(logistic_like.platform() == pluss::pCPU, "Logistic object sample_like preserves platform", &failures);
  expect(rayleigh_like.platform() == pluss::pCPU, "Rayleigh object sample_like preserves platform", &failures);
  expect(poisson_like.platform() == pluss::pCPU, "Poisson object sample_like preserves platform", &failures);
  expect(geometric_like.platform() == pluss::pCPU, "Geometric object sample_like preserves platform", &failures);
  expect(negbin_like.platform() == pluss::pCPU, "NegativeBinomial object sample_like preserves platform", &failures);
  expect(normaltrunc_like.platform() == pluss::pCPU, "NormalTrunc object sample_like preserves platform", &failures);
  expect(weibull_like.sizes().size() == 2 && weibull_like.sizes()[0] == 2 && weibull_like.sizes()[1] == 5,
         "sample_like preserves shape",
         &failures);
  expect(normal_like.sizes().size() == 2 && normal_like.sizes()[0] == 2 && normal_like.sizes()[1] == 5,
         "Normal object sample_like preserves shape",
         &failures);
  expect(gamma_like.sizes().size() == 2 && gamma_like.sizes()[0] == 2 && gamma_like.sizes()[1] == 5,
         "Gamma object sample_like preserves shape",
         &failures);
  expect(nig_like.sizes().size() == 2 && nig_like.sizes()[0] == 2 && nig_like.sizes()[1] == 5,
         "Nig object sample_like preserves shape",
         &failures);
  expect(laplace_like.sizes().size() == 2 && laplace_like.sizes()[0] == 2 && laplace_like.sizes()[1] == 5,
         "Laplace object sample_like preserves shape",
         &failures);
  expect(logistic_like.sizes().size() == 2 && logistic_like.sizes()[0] == 2 && logistic_like.sizes()[1] == 5,
         "Logistic object sample_like preserves shape",
         &failures);
  expect(rayleigh_like.sizes().size() == 2 && rayleigh_like.sizes()[0] == 2 && rayleigh_like.sizes()[1] == 5,
         "Rayleigh object sample_like preserves shape",
         &failures);
  expect(poisson_like.sizes().size() == 2 && poisson_like.sizes()[0] == 2 && poisson_like.sizes()[1] == 5,
         "Poisson object sample_like preserves shape",
         &failures);
  expect(geometric_like.sizes().size() == 2 && geometric_like.sizes()[0] == 2 && geometric_like.sizes()[1] == 5,
         "Geometric object sample_like preserves shape",
         &failures);
  expect(negbin_like.sizes().size() == 2 && negbin_like.sizes()[0] == 2 && negbin_like.sizes()[1] == 5,
         "NegativeBinomial object sample_like preserves shape",
         &failures);
  expect(normaltrunc_like.sizes().size() == 2 && normaltrunc_like.sizes()[0] == 2 && normaltrunc_like.sizes()[1] == 5,
         "NormalTrunc object sample_like preserves shape",
         &failures);

  const auto sobol_1d = pluss::rng::sobol({4}, f64, 0ULL).to_vec<double>();
  expect(approx(sobol_1d.at(0), 0.0), "sobol 1d first", &failures);
  expect(approx(sobol_1d.at(1), 0.5), "sobol 1d second", &failures);
  expect(approx(sobol_1d.at(2), 0.75), "sobol 1d third", &failures);
  expect(approx(sobol_1d.at(3), 0.25), "sobol 1d fourth", &failures);

  const auto sobol_2d = pluss::rng::sobol({4, 2}, f64, 0ULL).to_vec<double>();
  expect(approx(sobol_2d.at(0), 0.0) && approx(sobol_2d.at(1), 0.0), "sobol 2d point 0", &failures);
  expect(approx(sobol_2d.at(2), 0.5) && approx(sobol_2d.at(3), 0.5), "sobol 2d point 1", &failures);
  expect(approx(sobol_2d.at(4), 0.75) && approx(sobol_2d.at(5), 0.25), "sobol 2d point 2", &failures);
  expect(approx(sobol_2d.at(6), 0.25) && approx(sobol_2d.at(7), 0.75), "sobol 2d point 3", &failures);

  const Series mix_weights = pluss::make_series(std::vector<double>{1.0, 0.0}, {2}, f64);
  const Series mix_means = pluss::make_series(std::vector<double>{3.0, -4.0}, {2}, f64);
  const Series mix_stds = pluss::make_series(std::vector<double>{0.0, 5.0}, {2}, f64);
  const auto mix = mixture_normal(mix_weights, mix_means, mix_stds, {6}, f64, 19ULL, false).to_vec<double>();
  for (double v : mix) {
    expect(approx(v, 3.0), "mixture_normal deterministic component selection", &failures);
  }

  const auto mix_sobol = mixture_normal(mix_weights, mix_means, mix_stds, {4}, f64, 0ULL, true).to_vec<double>();
  for (double v : mix_sobol) {
    expect(approx(v, 3.0), "mixture_normal deterministic Sobol component selection", &failures);
  }

  const Series mix_pdf_x = pluss::make_series(std::vector<double>{0.0}, {1}, f64);
  const Series mix_pdf_weights = pluss::make_series(std::vector<double>{1.0, 0.0}, {2}, f64);
  const Series mix_pdf_means = pluss::make_series(std::vector<double>{0.0, 5.0}, {2}, f64);
  const Series mix_pdf_stds = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, f64);
  expect(approx(mixture_normal_pdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds).item<double>(),
                0.3989422804014327),
         "mixture_normal_pdf one-component parity", &failures);
  expect(approx(mixture_normal_cdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds).item<double>(), 0.5),
         "mixture_normal_cdf one-component parity", &failures);
  expect(approx(mixture_normal_logpdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds).item<double>(),
                std::log(0.3989422804014327)),
         "mixture_normal_logpdf one-component parity", &failures);
  expect(approx(mixture_normal_logcdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds).item<double>(),
                as_f64_scalar(logcdf("normal", mix_pdf_x, std::initializer_list<double>{})),
                1e-5),
         "mixture_normal_logcdf one-component parity", &failures);

  const pluss::distr::GaussianMixture gaussian_mixture_obj{mix_pdf_weights, mix_pdf_means, mix_pdf_stds};
  expect(approx(as_f64_scalar(gaussian_mixture_obj.pdf(mix_pdf_x)),
                as_f64_scalar(mixture_normal_pdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds)),
                1e-12),
         "GaussianMixture object pdf parity",
         &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.logpdf(mix_pdf_x)),
                as_f64_scalar(mixture_normal_logpdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds)),
                1e-12),
         "GaussianMixture object logpdf parity",
         &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.cdf(mix_pdf_x)),
                as_f64_scalar(mixture_normal_cdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds)),
                1e-12),
         "GaussianMixture object cdf parity",
         &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.logcdf(mix_pdf_x)),
                as_f64_scalar(mixture_normal_logcdf(mix_pdf_x, mix_pdf_weights, mix_pdf_means, mix_pdf_stds)),
                1e-12),
         "GaussianMixture object logcdf parity",
         &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.cgf(zero)), 0.0, 1e-12), "GaussianMixture object cgf at 0", &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.cumulant(1)), 0.0, 1e-12), "GaussianMixture object cumulant(1)", &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.cumulant(2)), 1.0, 1e-12), "GaussianMixture object cumulant(2)", &failures);
  expect(approx(as_f64_scalar(gaussian_mixture_obj.cumulant(3)), 0.0, 1e-10), "GaussianMixture object cumulant(3)", &failures);
  expect(gaussian_mixture_obj.sample({4}, f64, 43ULL).numel() == 4, "GaussianMixture object sample shape", &failures);

  const Series mix_left_tail = pluss::make_series(-12.0, f64);
  expect(std::isfinite(as_f64_scalar(mixture_normal_logcdf(mix_left_tail, mix_pdf_weights, mix_pdf_means, mix_pdf_stds))),
         "mixture_normal_logcdf left tail finite",
         &failures);
  expect(std::isfinite(as_f64_scalar(gaussian_mixture_obj.logcdf(mix_left_tail))),
         "GaussianMixture object logcdf left tail finite",
         &failures);
  expect(std::isinf(as_f64_scalar(mixture_normal_cdf(mix_left_tail, mix_pdf_weights, mix_pdf_means, mix_pdf_stds).log())),
         "mixture_normal_cdf().log() underflows in left tail",
         &failures);

  const Series deg_weights = pluss::make_series(std::vector<double>{1.0}, {1}, f64);
  const Series deg_means = pluss::make_series(std::vector<double>{3.0}, {1}, f64);
  const Series deg_stds = pluss::make_series(std::vector<double>{0.0}, {1}, f64);
  const auto deg_cdf = mixture_normal_cdf(
                           pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}, f64),
                           deg_weights,
                           deg_means,
                           deg_stds)
                           .to_vec<double>();
  expect(approx(deg_cdf[0], 0.0) && approx(deg_cdf[1], 1.0) && approx(deg_cdf[2], 1.0),
         "mixture_normal_cdf degenerate component step", &failures);
  const auto deg_pdf = mixture_normal_pdf(
                           pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}, f64),
                           deg_weights,
                           deg_means,
                           deg_stds)
                           .to_vec<double>();
  expect(approx(deg_pdf[0], 0.0) && std::isinf(deg_pdf[1]) && approx(deg_pdf[2], 0.0),
         "mixture_normal_pdf degenerate component mass", &failures);

  if (failures != 0) {
    std::cerr << "distr_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "distr_test: all checks passed\n";
  return 0;
}
