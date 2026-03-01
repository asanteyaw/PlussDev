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
  expect(!supports("chisq"), "supports(chisq) should be false in current surface", &failures);

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

  const Series normal_draws = sample("normal", {1.5, 2.0}, {4, 3}, f64, 20260301ULL);
  expect(normal_draws.scalar_type() == ScalarType::Float64, "normal sample dtype", &failures);
  expect(normal_draws.sizes().size() == 2 && normal_draws.sizes()[0] == 4 && normal_draws.sizes()[1] == 3,
         "normal sample shape",
         &failures);

  const Series bern_draws = sample("bern", {0.4}, {8}, Config{}.with_platform(pluss::pCPU), 20260301ULL);
  expect(bern_draws.scalar_type() == ScalarType::Bool, "bernoulli sample natural dtype", &failures);

  const Series like = pluss::zeros({2, 5}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
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
