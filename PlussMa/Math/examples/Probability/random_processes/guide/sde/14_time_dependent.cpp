// A function that depends on time explicitly, g(t, X), adds its partial time derivative to the drift:
// dg = (g_t + g' mu + 1/2 g'' sigma^2) dt + g' sigma dW. `ito_at(time, g, mu, sigma)` passes the time to g as its
// first argument. Discounting is the standard example: g(t, x) = exp(-r t) x has g_t = -r g, so the drift of the
// discounted process is exp(-r t)(mu - r x), and a martingale is exactly one whose discounted drift vanishes.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double r = 0.03, t = 0.8;
  const pluss::Series x = pluss::make_series(std::vector<double>{100.0, 80.0}, {2}).to(f64);
  // A traded asset with growth rate mu = r x (the risk-neutral drift) and one with a higher growth rate.
  const pluss::Series mu = pluss::make_series(std::vector<double>{r * 100.0, 0.07 * 80.0}, {2}).to(f64);
  const pluss::Series sigma = x.mul(0.2);

  const auto d = x.calc.diff.ito_at(pluss::make_series(t).to(f64),
                                    [r](const pluss::Series& time, const pluss::Series& s) { return s.mul(time.mul(-r).exp()); },
                                    mu, sigma);
  const auto drift = d.drift.host<double>();
  const auto diffusion = d.diffusion.host<double>();
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "discounted drift:  " << drift[0] << "   " << drift[1] << "\n";
  std::cout << "exp(-r t)(mu - r x): " << std::exp(-r * t) * (r * 100.0 - r * 100.0) << "   "
            << std::exp(-r * t) * (0.07 * 80.0 - r * 80.0) << "\n";
  std::cout << "discounted diffusion: " << diffusion[0] << "   " << diffusion[1] << "   (exp(-r t) sigma = "
            << std::exp(-r * t) * 20.0 << ", " << std::exp(-r * t) * 16.0 << ")\n";
  return 0;
}
