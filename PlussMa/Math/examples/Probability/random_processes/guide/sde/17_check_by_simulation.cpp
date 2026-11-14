// Ito's lemma predicts more than a formula: it predicts what simulation should find. For geometric Brownian
// motion dX = mu X dt + sigma X dW the differential of g = X^2 has drift (2 mu + sigma^2) g, so E[X_T^2] =
// X_0^2 exp((2 mu + sigma^2) T). The program reads the factor 2 mu + sigma^2 off the computed drift (not from a
// formula), then compares the prediction with the mean of the squares of 60000 simulated paths.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double mu_c = 0.15, sg = 0.35, x0 = 1.2, T = 1.0;

  // The drift of g(X) = X^2 from Ito's lemma, divided by g: the exponential growth rate of E[g(X)].
  const pluss::Series point = pluss::make_series(std::vector<double>{x0}, {1}).to(f64);
  const auto d = point.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, point.mul(mu_c), point.mul(sg));
  const double rate = d.drift.squeeze().item<double>() / (x0 * x0);
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "growth rate of E[X^2] from Ito's lemma: " << rate << "  (2 mu + sigma^2 = " << 2 * mu_c + sg * sg << ")\n";

  const std::int64_t paths = 60000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::Milstein;
  options.steps = 100;
  options.seed = 99;
  const auto run = sde::fluctua([&](double, const pluss::Series& y, const pluss::Series&) { return y.mul(mu_c); },
                                [&](double, const pluss::Series& y, const pluss::Series&) { return y.mul(sg); }, y0, 0.0, T,
                                pluss::series::undefined(), options);
  const pluss::Series squares = run.y_final.mul(run.y_final);
  const double mean = squares.mean().item<double>();
  const double stderr_ = std::sqrt(squares.sub(mean).pow(2.0).mean().item<double>() / paths);
  std::cout << "predicted E[X_T^2] = " << x0 * x0 * std::exp(rate * T) << "\n";
  std::cout << "simulated mean     = " << mean << "  (standard error " << stderr_ << ")\n";
  return 0;
}
