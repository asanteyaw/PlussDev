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

  const Series one = pluss::make_series(1.0, f64);
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
  expect(sample("poisson", {2.0}, {6}, Config{}.with_platform(pluss::pCPU), 11ULL).scalar_type() == ScalarType::Int64,
         "poisson sample natural dtype",
         &failures);
  expect(sample("gig", {1.0, 2.0, 3.0}, {5}, f64, 13ULL).numel() == 5, "gig sample shape", &failures);
  expect(sample("nig", {2.0, 0.5, 0.0, 1.0}, {5}, f64, 17ULL).numel() == 5, "nig sample shape", &failures);
  expect(sample("vonmises", {0.0, 2.0}, {5}, f64, 19ULL).numel() == 5, "vonmises sample shape", &failures);
  expect(pluss::distr::sample("arcsin", {5}, f64, 23ULL).numel() == 5, "sample overload without params", &failures);

  const Series like = pluss::zeros({2, 5}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
  expect(pluss::distr::sample_like("rademacher", like, 29ULL).numel() == like.numel(),
         "sample_like overload without params",
         &failures);
  const Series weibull_like = sample_like("weibull", {1.5, 2.0}, like, 7ULL);
  expect(weibull_like.scalar_type() == ScalarType::Float32, "sample_like preserves dtype", &failures);
  expect(weibull_like.sizes().size() == 2 && weibull_like.sizes()[0] == 2 && weibull_like.sizes()[1] == 5,
         "sample_like preserves shape",
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

  if (failures != 0) {
    std::cerr << "distr_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "distr_test: all checks passed\n";
  return 0;
}
