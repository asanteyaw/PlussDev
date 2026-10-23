// With m Brownian factors, sigma has an extra trailing axis of length m: sigma[i, f] is the loading of state i
// on factor f. The second-order term adds the variances of all factors, and the differential carries one
// diffusion coefficient per factor.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series x = pluss::make_series(std::vector<double>{0.5, 1.0, -0.3}, {3}).to(f64);
  const pluss::Series mu = pluss::make_series(std::vector<double>{0.1, 0.2, -0.1}, {3}).to(f64);
  // two factors: row i is (sigma_i1, sigma_i2)
  const pluss::Series sigma = pluss::make_series(std::vector<double>{0.3, 0.1, 0.5, 0.2, 0.2, 0.4}, {3, 2}).to(f64);

  const auto d = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma);
  const auto drift = d.drift.to_vec<double>();
  const auto diffusion = d.diffusion.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "diffusion has shape [" << d.diffusion.size(0) << ", " << d.diffusion.size(1) << "]\n";
  const std::vector<double> xv = {0.5, 1.0, -0.3}, mv = {0.1, 0.2, -0.1}, sg = {0.3, 0.1, 0.5, 0.2, 0.2, 0.4};
  for (int i = 0; i < 3; ++i) {
    const double variance = sg[2 * i] * sg[2 * i] + sg[2 * i + 1] * sg[2 * i + 1];
    std::cout << "state " << i << ": drift " << drift[i] << " = 2 x mu + (sigma_1^2 + sigma_2^2) = "
              << 2 * xv[i] * mv[i] + variance << ";  diffusion (" << diffusion[2 * i] << ", " << diffusion[2 * i + 1]
              << ") = 2 x sigma = (" << 2 * xv[i] * sg[2 * i] << ", " << 2 * xv[i] * sg[2 * i + 1] << ")\n";
  }
  return 0;
}
