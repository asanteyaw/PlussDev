// Every stochastic-integral result carries a diagnosis of the path it was computed against. The values of Ito and
// Stratonovich integrals are only the standard ones when the integrator is a standard Wiener path, whose realized
// quadratic variation equals the elapsed time. `quadratic_variation_ratio` is that ratio (about 1 for a standard
// path) and `warnings` says in words when it is not: a path with a different volatility, a smooth path, or a path
// with jumps. Nothing is thrown, because such a path is a legitimate integrator; the warning is there so a wrong
// path is not mistaken for a standard one.
#include <pluss/ma.h>

#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  auto identity = [](double, const pluss::Series& w) { return w; };

  const auto standard = sde::simulate_wiener_path(like, 0.0, 1.0, 2000, /*seed=*/3);
  sde::WienerPath tripled = standard;
  tripled.w = standard.w.mul(3.0);                                  // volatility 3
  sde::WienerPath smooth = standard;
  smooth.w = pluss::Series::linspace(0.0, 1.0, 2001, f64).unsqueeze(1);  // a straight line

  for (const auto& item : {std::pair<const char*, sde::WienerPath>{"standard path", standard},
                           {"path times 3", tripled},
                           {"straight line", smooth}}) {
    const auto r = sde::ito_integral(identity, item.second);
    std::cout << item.first << ": quadratic_variation_ratio = " << r.quadratic_variation_ratio << ", warnings = "
              << r.warnings.size() << "\n";
    for (const auto& w : r.warnings) std::cout << "    " << w << "\n";
  }
  return 0;
}
