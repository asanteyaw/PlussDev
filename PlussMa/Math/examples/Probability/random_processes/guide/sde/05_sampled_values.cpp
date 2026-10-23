// When the integrand is not a function of (t, W_t) but a column of numbers that was computed from the path, the
// integrals are methods of that column: `f.antidiff(w).ito()` sums f_i (w_{i+1} - w_i). The receiver holds the
// integrand at every grid point and the peer is the integrator path. The same family gives the realized quadratic
// variation of a path, `w.antidiff().quadratic_variation()`, and the covariation of two paths. Quadratic
// variation is polarized exactly: [x + y] = [x] + [y] + 2 [x, y].
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t steps = 800;
  const double h = 1.0 / steps;
  auto make_path = [&](std::uint64_t seed) {
    const pluss::Series dw = pluss::series::randn({steps, 1}, pluss::Config{}.with_dtype(f64), seed).mul(std::sqrt(h));
    return pluss::Series::zeros({1, 1}, f64).unite({dw.cumsum(0)}, 0);
  };
  const pluss::Series x = make_path(1), y = make_path(2);

  // An integrand computed from the path: the square of the path so far.
  const pluss::Series f = x.mul(x);
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "Ito of x^2 dx:            " << f.antidiff(x).ito(0).item<double>() << "\n";
  std::cout << "Stratonovich of x^2 dx:   " << f.antidiff(x).strat(0).item<double>() << "   (x_T^3 / 3 = "
            << std::pow(x.select(0, steps).item<double>(), 3) / 3.0 << " in the limit)\n";
  // The Ito form needs only the left endpoints: n - 1 samples are enough.
  std::cout << "Ito from n - 1 samples:   " << f.slice(0, 0, steps, 1).antidiff(x).ito(0).item<double>() << "\n\n";

  const double qx = x.antidiff().quadratic_variation(0).item<double>();
  const double qy = y.antidiff().quadratic_variation(0).item<double>();
  const double cxy = x.antidiff(y).covariation(0).item<double>();
  const double qsum = x.add(y).antidiff().quadratic_variation(0).item<double>();
  std::cout << "[x] = " << qx << ", [y] = " << qy << ", [x, y] = " << cxy << "\n";
  std::cout << "[x + y] = " << qsum << " = [x] + [y] + 2 [x, y] = " << qx + qy + 2.0 * cxy << "\n";
  return 0;
}
