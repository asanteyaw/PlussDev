// On a path with jumps the three integrals of f(X) = X^2 against dX differ, and only one obeys the ordinary
// chain rule. The Marcus integral integrates along the straight segment through each jump, so the jump steps are
// exact; what remains is the trapezoid error of the small Brownian steps, which shrinks with the grid. The
// Stratonovich average (trapezoid) is badly off on the large steps, and Ito's integral is off by the quadratic
// variation.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  const auto jp = sde::simulate_jump_diffusion_path(like, 0.0, 1.0, 4000, 11, 0.0, 0.2, 6.0, 0.0, 0.5);
  auto square = [](double, const pluss::Series& x) { return x.mul(x); };

  const double xT = jp.path.w.select(0, jp.path.steps).squeeze().item<double>();
  const double exact = xT * xT * xT / 3.0;
  const double marcus = sde::marcus_integral(square, jp.path, jp.jump_mask).value.squeeze().item<double>();
  const double strat = sde::strat_integral(square, jp.path).value.squeeze().item<double>();
  const double ito = sde::ito_integral(square, jp.path).value.squeeze().item<double>();

  std::cout << std::setprecision(6);
  std::cout << "X_T^3 / 3 (chain rule)  " << exact << "\n";
  std::cout << "Marcus                  " << marcus << "   error " << std::scientific << std::abs(marcus - exact) << std::fixed << "\n";
  std::cout << "Stratonovich (average)  " << strat << "   error " << std::scientific << std::abs(strat - exact) << std::fixed << "\n";
  std::cout << "Ito                     " << ito << "   error " << std::scientific << std::abs(ito - exact) << std::fixed << "\n";

  // Without a mask the jump steps are found from their size: |dX| > jump_sigma * sqrt(step). A jump smaller
  // than that threshold (here 8 * sqrt(1/4000) = 0.13) is treated as Brownian, which is why the number differs.
  const double automatic = sde::marcus_integral(square, jp.path, 8.0).value.squeeze().item<double>();
  std::cout << "Marcus, jumps found by size  " << automatic << "\n";
  return 0;
}
