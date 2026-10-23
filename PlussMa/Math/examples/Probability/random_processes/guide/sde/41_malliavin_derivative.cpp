// The discrete Malliavin derivative D_i F = dF / d(dW_i) answers "how much does F change if the i-th Brownian
// increment changes?". For F = W_T^2 = (sum dW)^2 it is 2 W_T on every increment; for F = max(W) it is 1 on the
// increments up to the argmax of W and 0 after it. The derivative is computed by the autodiff engine, one reverse
// pass, however many increments there are.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t n = 8;
  const pluss::Series dw = pluss::series::randn({n}, pluss::Config{}.with_dtype(f64), 4).mul(std::sqrt(1.0 / n));
  const double wT = dw.sum().item<double>();

  const pluss::Series d_square = dw.calc.diff.malliavin([](const pluss::Series& inc) { return inc.sum().pow(2.0); });
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "W_T = " << wT << ", so D_i (W_T^2) = 2 W_T = " << 2 * wT << "; computed:";
  for (double v : d_square.to_vec<double>()) std::cout << " " << v;
  std::cout << "\n";

  // The path form takes the path W itself (n + 1 samples starting at 0) and differentiates with respect to its
  // increments, so F sees W rebuilt from them.
  const pluss::Series w = pluss::Series::zeros({1}, f64).unite({dw.cumsum(0)}, 0);
  const pluss::Series d_max = w.calc.diff.malliavin_path([](const pluss::Series& path) { return path.amax(0); });
  std::cout << "W at the grid:";
  for (double v : w.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nD_i max(W): ";
  for (double v : d_max.to_vec<double>()) std::cout << " " << v;
  std::cout << "\n";
  return 0;
}
