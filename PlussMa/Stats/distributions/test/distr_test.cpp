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

  const auto sobol_1d = pluss::sobol({4}, ScalarType::Float64, 0).to_vec<double>();
  expect(approx(sobol_1d.at(0), 0.0), "sobol 1d first", &failures);
  expect(approx(sobol_1d.at(1), 0.5), "sobol 1d second", &failures);
  expect(approx(sobol_1d.at(2), 0.75), "sobol 1d third", &failures);
  expect(approx(sobol_1d.at(3), 0.25), "sobol 1d fourth", &failures);

  const auto sobol_2d = pluss::sobol({4, 2}, ScalarType::Float64, 0).to_vec<double>();
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
