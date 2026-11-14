// The Marcus differential of g(X) for dX = mu dt + sigma o dW + X- <> dJ keeps the ordinary chain rule. For
// g = log the multiplicative jump of size z moves log X by exactly z, so the jump sizes of the differential are
// the atoms themselves. The continuous part is the Stratonovich one: with mu and sigma given in Stratonovich
// form, the drift of log X is mu / x and the diffusion sigma / x, with no second-order term.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series x = pluss::make_series(std::vector<double>{1.5}, {1}).to(f64);
  const pluss::Series mu = x.mul(0.1), sigma = x.mul(0.3);
  pluss::Series::JumpMeasure jumps;
  jumps.sizes = {pluss::make_series(std::vector<double>{0.4, -0.7}, {2}).to(f64)};
  jumps.intensity = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}).to(f64);

  const auto d = x.calc.diff.marcus([](const pluss::Series& s) { return s.log(); }, mu, sigma,
                                    [](const pluss::Series& s) { return s; }, jumps);
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "continuous drift  " << d.drift.squeeze().item<double>() << "  (mu / x = " << 0.1 << ")\n";
  std::cout << "diffusion         " << d.diffusion.squeeze().item<double>() << "  (sigma / x = " << 0.3 << ")\n";
  const auto js = d.jump_sizes.host<double>();
  std::cout << "jump sizes of log X: " << js[0] << ", " << js[1] << "  (the atoms 0.4 and -0.7)\n";
  return 0;
}
