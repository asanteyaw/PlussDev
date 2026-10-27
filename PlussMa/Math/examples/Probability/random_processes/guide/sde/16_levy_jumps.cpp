// When the driver also jumps, Ito's lemma gains a term that is not a derivative: a jump of size z moves g(X)
// by g(x + z) - g(x), whatever the smoothness of g. The jump part of a Levy process is described to
// calc.diff.ito as an atomic measure: atom k jumps by sizes[k] at rate intensity[k] per unit time. A compound
// Poisson process is a handful of atoms; a Levy measure with a density is its quadrature. Here a Kou
// double-exponential measure (up-jumps Exp(eta1) with probability p, down-jumps Exp(eta2) otherwise, total rate
// lambda) is discretized into 4000 atoms and compared with the exact integrals.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double lambda = 3.0, p = 0.4, q = 0.6, eta1 = 10.0, eta2 = 5.0;
  const int per_side = 2000;
  std::vector<double> z, nu;
  const double dz1 = 3.0 / per_side, dz2 = 6.0 / per_side;
  for (int k = 0; k < per_side; ++k) {  // midpoint rule on (0, 3] and [-6, 0)
    const double up = (k + 0.5) * dz1, down = (k + 0.5) * dz2;
    z.push_back(up);
    nu.push_back(lambda * p * eta1 * std::exp(-eta1 * up) * dz1);
    z.push_back(-down);
    nu.push_back(lambda * q * eta2 * std::exp(-eta2 * down) * dz2);
  }
  const auto K = static_cast<std::int64_t>(z.size());
  pluss::Series::JumpMeasure jumps;
  jumps.sizes = {pluss::make_series(z, {K}).to(f64)};
  jumps.intensity = pluss::make_series(nu, {K}).to(f64);

  const pluss::Series x = pluss::make_series(std::vector<double>{0.5}, {1}).to(f64);
  const pluss::Series mu = pluss::make_series(std::vector<double>{0.1}, {1}).to(f64);
  const pluss::Series sigma = pluss::make_series(std::vector<double>{0.3}, {1}).to(f64);
  const double base_x2 = 2 * 0.5 * 0.1 + 0.3 * 0.3;  // continuous part for g = x^2

  jumps.compensated = true;  // the jump part is a martingale: X carries the drift mu on top of it
  const auto sq = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma, jumps);
  const double mass = pluss::make_series(nu, {K}).to(f64).sum().item<double>();
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "total jump rate (sum of the atom rates): " << mass << " (lambda = " << lambda << ")\n";
  const double second_moment = lambda * (2 * p / (eta1 * eta1) + 2 * q / (eta2 * eta2));  // integral of z^2 nu(dz)
  std::cout << "g = x^2, compensated: drift " << sq.drift.item<double>() << " = continuous part " << base_x2
            << " + integral z^2 nu(dz) = " << base_x2 + second_moment << "\n";

  const auto ex = x.calc.diff.ito([](const pluss::Series& s) { return s.exp(); }, mu, sigma, jumps);
  const double ex_exact = std::exp(0.5) * (0.1 + 0.5 * 0.09) +
                          std::exp(0.5) * lambda * (p * (eta1 / (eta1 - 1) - 1 - 1 / eta1) + q * (eta2 / (eta2 + 1) - 1 + 1 / eta2));
  std::cout << "g = exp(x), compensated: drift " << ex.drift.item<double>() << " (exact " << ex_exact << ")\n";

  jumps.compensated = false;  // raw jumps: X = ... + the sum of the jumps, drift untouched
  const auto raw = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma, jumps);
  std::cout << "g = x^2, raw jumps:    drift " << raw.drift.item<double>() << " (continuous part only), "
            << raw.jump_sizes.size(0) << " jump sizes of g(x + z) - g(x), e.g. for z = " << z[0] << ": "
            << raw.jump_sizes.host<double>()[0] << " = 2 x z + z^2 = " << 2 * 0.5 * z[0] + z[0] * z[0] << "\n";
  return 0;
}
