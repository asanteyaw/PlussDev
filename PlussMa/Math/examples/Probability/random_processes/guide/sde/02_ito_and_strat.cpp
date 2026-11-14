// The Ito and Stratonovich integrals of W against W are different numbers, and the difference is not a
// discretization error: it is half the quadratic variation of the path, exactly, for any number of steps.
// Ito evaluates the integrand at the start of every step (so it cannot see the increment it multiplies);
// Stratonovich averages the two ends. Both are computed here from the same path, and compared with the exact
// discrete identities and with the continuous-time limits (W_T^2 - T)/2 and W_T^2 / 2.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  auto w_of_t = [](double, const pluss::Series& w) { return w; };  // the integrand f(t, W_t) = W_t

  std::cout << std::fixed << std::setprecision(5);
  std::cout << "steps      Ito     Stratonovich   (W^2 - QV)/2   W^2/2     QV/2    Strat - Ito\n";
  for (const std::int64_t steps : {10, 100, 1000, 10000}) {
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, /*seed=*/7);
    const auto ito = sde::ito_integral(w_of_t, path);
    const auto strat = sde::strat_integral(w_of_t, path);
    const double wt = path.w.select(0, steps).squeeze().item<double>();
    const double qv = ito.realized_quadratic_variation.squeeze().item<double>();
    std::cout << std::setw(6) << steps << "  " << std::setw(8) << ito.value.squeeze().item<double>() << "   "
              << std::setw(8) << strat.value.squeeze().item<double>() << "     " << std::setw(8) << 0.5 * (wt * wt - qv)
              << "   " << std::setw(8) << 0.5 * wt * wt << "  " << std::setw(8) << 0.5 * qv << "  "
              << std::setw(8) << strat.value.squeeze().item<double>() - ito.value.squeeze().item<double>() << "\n";
  }

  const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 10000, /*seed=*/7);
  const double wt = path.w.select(0, 10000).squeeze().item<double>();
  const double ito = sde::ito_integral(w_of_t, path).value.squeeze().item<double>();
  std::cout << "\nat 10000 steps: Ito = " << ito << ", (W_T^2 - T)/2 = " << 0.5 * (wt * wt - 1.0)
            << "  (the continuous-time Ito value)\n";
  return 0;
}
