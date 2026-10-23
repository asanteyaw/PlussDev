<!-- generated from the_derivative_of_two_functionals.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The derivative of two functionals

[Back to Malliavin Calculus](../stochastic_05_malliavin.md)


`calc.diff.malliavin(F)` takes the increments and returns the derivative per increment. `malliavin_path(F)` takes the path itself, with `n + 1` samples starting at 0, and differentiates with respect to its increments so that `F` sees the path rebuilt from them.

```cpp
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
```

Output

```text
W_T = -0.86685, so D_i (W_T^2) = 2 W_T = -1.73370; computed: -1.73370 -1.73370 -1.73370 -1.73370 -1.73370 -1.73370 -1.73370 -1.73370
W at the grid: 0.00000 0.35760 0.07076 -0.12382 -0.11375 -0.52610 -0.77600 -0.91235 -0.86685
D_i max(W):  1.00000 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000
```
For `F = W_T²` the derivative is −1.73370 on every increment, equal to `2 W_T` with `W_T = −0.86685`. For `F = max(W)` the maximum of the sampled path is the second sample, 0.35760, so only the first increment moves it: the derivative is 1 there and 0 on the rest.
