<!-- generated from the_family_in_between.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The family in between

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


The `θ` family reads `f` at `(1 − θ)·left + θ·right` of each step. The program fits each integral as `W_T²/2 + c · QV/2` and prints `c`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  // Two independent paths of 2000 steps on [0, 2].
  const std::int64_t steps = 2000;
  const double T = 2.0, h = T / steps;
  const pluss::Series dw = pluss::series::randn({steps, 2}, pluss::Config{}.with_dtype(f64), 11).mul(std::sqrt(h));
  const pluss::Series w = pluss::Series::zeros({1, 2}, f64).unite({dw.cumsum(0)}, 0);
  auto f = [](const pluss::Series& /*t*/, const pluss::Series& x) { return x; };  // f(t, w) = w

  const pluss::Series qv = w.antidiff().quadratic_variation(0);
  std::cout << std::fixed << std::setprecision(5);
  for (int path = 0; path < 2; ++path) {
    const double wt = w.select(0, steps).host<double>()[path];
    const double q = qv.host<double>()[path];
    std::cout << "path " << path << ": W_T^2/2 = " << 0.5 * wt * wt << ", QV/2 = " << 0.5 * q << "\n";
    for (const double theta : {0.0, 0.5, 1.0}) {
      const double value = w.calc.antidiff.theta(f, theta, 0.0, T).host<double>()[path];
      std::cout << "  theta = " << std::setprecision(1) << theta << std::setprecision(5) << "   integral = " << value
                << "   = W_T^2/2 " << std::showpos << (value - 0.5 * wt * wt) / (0.5 * q) << " QV/2" << std::noshowpos
                << "\n";
    }
  }
  return 0;
}
```

Output

```text
path 0: W_T^2/2 = 1.15198, QV/2 = 0.96094
  theta = 0.0   integral = 0.19104   = W_T^2/2 -1.00000 QV/2
  theta = 0.5   integral = 1.15198   = W_T^2/2 -0.00000 QV/2
  theta = 1.0   integral = 2.11292   = W_T^2/2 +1.00000 QV/2
path 1: W_T^2/2 = 0.36715, QV/2 = 1.02970
  theta = 0.0   integral = -0.66255   = W_T^2/2 -1.00000 QV/2
  theta = 0.5   integral = 0.36715   = W_T^2/2 +0.00000 QV/2
  theta = 1.0   integral = 1.39685   = W_T^2/2 +1.00000 QV/2
```
The coefficient is `−1`, `0` and `+1` for `θ = 0`, `½` and `1` on both paths, to the printed precision. That is the whole classification: Ito subtracts half the quadratic variation, Stratonovich subtracts none, and the backward integral (used in the Hänggi–Klimontovich convention) adds it.
