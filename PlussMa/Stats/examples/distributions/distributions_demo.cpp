#include <pluss/ma.h>

#include <iostream>
#include <vector>

double calc_tip(const pluss::sign_list& list) {
  return list[0].item<double>() + list[1].item<double>();
}

int main() {
  using namespace pluss;
  using namespace pluss::subsp::stats::distr;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series draws = distr::sample(dst::Normal, {0.0, 1.0}, {8}, f64, 7ULL);
  const Series arcsin_draws = distr::sample(dst::Arcsin, {8}, f64, 11ULL);
  const Series circular = distr::sample(dst::VonMises, {0.0, 3.0}, {8}, f64, 13ULL);
  const Series grid = make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0}, {5}, f64);
  const distr::Normal normal{0.0, 1.0};
  const distr::Bernoulli bernoulli{0.35};

  const Series bell = distr::pdf(dst::Normal, grid, {0.0, 1.0});
  const Series bell_series_params =
      distr::logpdf(dst::Normal, grid, pluss::sign_list(pluss::zeros_like(grid), pluss::ones_like(grid)));
  const Series bell_series_braces =
      distr::logpdf(dst::Normal, grid, {pluss::zeros_like(grid), pluss::ones_like(grid)});
  const Series bell_log = distr::Normal{0.0, 1.0}.logpdf(grid);
  const Series bern_logcdf = bernoulli.logcdf(make_series(std::vector<double>{0.0, 1.0}, {2}, f64));
  const Series normal_cgf = normal.cgf(make_series(std::vector<double>{0.0, 1.0}, {2}, f64));
  const Series tail = distr::cdf(dst::StudentT, grid, {5.0});

  const Series like = zeros({2, 4}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
  const Series shaped = distr::sample_like(dst::Gamma, {2.0}, like, 19ULL);

  const Series weights = make_series(std::vector<double>{0.65, 0.35}, {2}, f64);
  const Series means = make_series(std::vector<double>{-1.0, 2.0}, {2}, f64);
  const Series stddevs = make_series(std::vector<double>{0.5, 1.25}, {2}, f64);
  const Series mixture = distr::mixture_normal(weights, means, stddevs, {10}, f64, 23ULL, true);

  std::cout << "normal sample: " << draws << "\n";
  std::cout << "arcsin sample: " << arcsin_draws << "\n";
  std::cout << "vonmises sample: " << circular << "\n";
  std::cout << "normal pdf grid: " << bell << "\n";
  std::cout << "normal logpdf grid (Series params): " << bell_series_params << "\n";
  std::cout << "normal logpdf grid (Series params braces): " << bell_series_braces << "\n";
  std::cout << "normal logpdf grid (object): " << bell_log << "\n";
  std::cout << "normal cgf (object): " << normal_cgf << "\n";
  std::cout << "bernoulli logcdf (object): " << bern_logcdf << "\n";
  std::cout << "student-t cdf grid: " << tail << "\n";
  std::cout << "sample_like gamma dtype=" << static_cast<int>(shaped.scalar_type()) << " sizes=" << shaped.sizes().size() << "\n";
  std::cout << "mixture sample: " << mixture << "\n";
  std::cout << "sign_list function example: " << calc_tip({pluss::make_series(1.0), pluss::make_series(2.0)}) << "\n";

  return 0;
}
