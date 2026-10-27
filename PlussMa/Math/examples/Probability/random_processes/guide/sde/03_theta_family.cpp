// The Ito and Stratonovich integrals are two members of a family indexed by where inside each step the
// integrand is read: (1 - theta) f(left) + theta f(right). theta = 0 is Ito, theta = 1/2 is Stratonovich, and
// theta = 1 reads the right end of the step (the backward, or Hanggi-Klimontovich, integral). Along a Wiener path
// the three differ by multiples of the quadratic variation, and they are available directly on the path Series.
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
